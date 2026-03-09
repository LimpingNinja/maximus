/*
 * mex_tls.c — OpenSSL TLS wrapper for MEX socket intrinsics
 *
 * Copyright 2026 by Kevin Morgan.  All rights reserved.
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version 2
 * of the License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.
 */

#include "mex_tls.h"

#include <openssl/ssl.h>
#include <openssl/err.h>
#include <openssl/x509.h>

#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/select.h>
#include <sys/time.h>

/*------------------------------------------------------------------------*
 * Internal state                                                         *
 *------------------------------------------------------------------------*/

/** Opaque TLS connection — wraps an OpenSSL SSL object. */
struct mex_tls_conn {
    SSL *ssl;
    int  fd;   /**< Borrowed fd — we do NOT close it */
};

/** Global SSL context, shared across all connections. */
static SSL_CTX *g_ssl_ctx = NULL;
static int       g_tls_inited = 0;

/** Last error buffer for diagnostics. */
static char g_tls_errbuf[512] = "(no error)";

/**
 * @brief Return a human-readable string describing the last TLS error.
 *
 * @return Pointer to a static buffer with the last error message.
 */
const char *mex_tls_last_error(void)
{
    return g_tls_errbuf;
}

/** @brief Capture the current OpenSSL error queue into g_tls_errbuf. */
static void capture_ssl_error(const char *context, int ssl_err)
{
    unsigned long e = ERR_peek_last_error();
    const char *reason = e ? ERR_reason_error_string(e) : NULL;
    snprintf(g_tls_errbuf, sizeof(g_tls_errbuf),
             "%s: ssl_err=%d, errno=%d (%s), openssl=%s",
             context, ssl_err, errno, strerror(errno),
             reason ? reason : "(none)");
    ERR_clear_error();
}

/*------------------------------------------------------------------------*
 * Helpers                                                                *
 *------------------------------------------------------------------------*/

/**
 * @brief Set a file descriptor to non-blocking mode.
 * @return 0 on success, -1 on error.
 */
static int set_nonblock(int fd)
{
    int flags = fcntl(fd, F_GETFL, 0);
    if (flags == -1) return -1;
    return fcntl(fd, F_SETFL, flags | O_NONBLOCK);
}

/**
 * @brief Restore a file descriptor to blocking mode.
 * @return 0 on success, -1 on error.
 */
static int set_blocking(int fd)
{
    int flags = fcntl(fd, F_GETFL, 0);
    if (flags == -1) return -1;
    return fcntl(fd, F_SETFL, flags & ~O_NONBLOCK);
}

/**
 * @brief Wait for fd readiness with a timeout.
 * @param fd        File descriptor.
 * @param want_read 1 to wait for read, 0 for write.
 * @param ms        Timeout in milliseconds.
 * @return 1 if ready, 0 on timeout, -1 on error.
 */
static int wait_ready(int fd, int want_read, int ms)
{
    fd_set fds;
    struct timeval tv;

    FD_ZERO(&fds);
    FD_SET(fd, &fds);
    tv.tv_sec  = ms / 1000;
    tv.tv_usec = (ms % 1000) * 1000;

    if (want_read)
        return select(fd + 1, &fds, NULL, NULL, &tv);
    else
        return select(fd + 1, NULL, &fds, NULL, &tv);
}

/**
 * @brief Compute remaining milliseconds from a start time.
 */
static int remaining_ms(struct timeval *start, int budget_ms)
{
    struct timeval now;
    gettimeofday(&now, NULL);
    long elapsed = (now.tv_sec - start->tv_sec) * 1000
                 + (now.tv_usec - start->tv_usec) / 1000;
    int remain = budget_ms - (int)elapsed;
    return (remain > 0) ? remain : 0;
}

/*------------------------------------------------------------------------*
 * Public API                                                             *
 *------------------------------------------------------------------------*/

/**
 * @brief One-time global TLS library initialization.
 *
 * Creates the shared SSL_CTX, sets minimum TLS 1.2, disables certificate
 * verification, and advertises HTTP/1.1 via ALPN. Safe to call multiple
 * times; only the first call performs work.
 */
void mex_tls_global_init(void)
{
    if (g_tls_inited)
        return;

    /* OpenSSL 1.1+ / 3.x auto-inits, but explicit init is harmless */
    OPENSSL_init_ssl(OPENSSL_INIT_LOAD_SSL_STRINGS
                   | OPENSSL_INIT_LOAD_CRYPTO_STRINGS, NULL);

    g_ssl_ctx = SSL_CTX_new(TLS_client_method());
    if (!g_ssl_ctx)
        return;

    /* Skip cert verification — BBS making outgoing API requests.
     * Users can override via config in the future. */
    SSL_CTX_set_verify(g_ssl_ctx, SSL_VERIFY_NONE, NULL);

    /* Let OpenSSL pick the best available ciphers and TLS version.
     * This matches what curl does by default. */
    SSL_CTX_set_min_proto_version(g_ssl_ctx, TLS1_2_VERSION);

    /* Advertise HTTP/1.1 only via ALPN — we don't implement HTTP/2.
     * Without this, some servers may attempt H2 negotiation. */
    {
        static const unsigned char alpn[] = { 8, 'h','t','t','p','/','1','.','1' };
        SSL_CTX_set_alpn_protos(g_ssl_ctx, alpn, sizeof(alpn));
    }

    g_tls_inited = 1;
}

/**
 * @brief Tear down the global TLS context. Call at program shutdown.
 */
void mex_tls_global_cleanup(void)
{
    if (!g_tls_inited)
        return;

    if (g_ssl_ctx)
    {
        SSL_CTX_free(g_ssl_ctx);
        g_ssl_ctx = NULL;
    }

    g_tls_inited = 0;
}

/**
 * @brief Create a TLS connection on an already-connected socket fd.
 *
 * Performs a non-blocking TLS handshake with select()-based timeout.
 * On success the socket is restored to blocking mode.
 *
 * @param fd          Connected TCP socket (caller retains ownership).
 * @param hostname    Server hostname for SNI.
 * @param timeout_ms  Handshake timeout in milliseconds.
 * @return Opaque TLS handle on success, NULL on failure.
 */
mex_tls_conn *mex_tls_connect(int fd, const char *hostname, int timeout_ms)
{
    if (!g_tls_inited || !g_ssl_ctx)
    {
        mex_tls_global_init();
        if (!g_ssl_ctx)
            return NULL;
    }

    SSL *ssl = SSL_new(g_ssl_ctx);
    if (!ssl)
        return NULL;

    SSL_set_fd(ssl, fd);
    SSL_set_tlsext_host_name(ssl, hostname);   /* SNI */

    /* Non-blocking handshake with select()-based waiting */
    set_nonblock(fd);

    struct timeval start;
    gettimeofday(&start, NULL);

    for (;;)
    {
        int ret = SSL_connect(ssl);
        if (ret == 1)
        {
            /* Handshake complete — restore blocking for I/O */
            set_blocking(fd);

            mex_tls_conn *conn = calloc(1, sizeof(*conn));
            if (!conn)
            {
                SSL_free(ssl);
                return NULL;
            }
            conn->ssl = ssl;
            conn->fd  = fd;
            return conn;
        }

        int err = SSL_get_error(ssl, ret);
        if (err != SSL_ERROR_WANT_READ && err != SSL_ERROR_WANT_WRITE)
        {
            /* Real error — capture details before cleanup */
            capture_ssl_error("handshake", err);
            SSL_free(ssl);
            set_blocking(fd);
            return NULL;
        }

        int remain = remaining_ms(&start, timeout_ms);
        if (remain <= 0)
        {
            SSL_free(ssl);
            set_blocking(fd);
            return NULL;
        }

        int rdy = wait_ready(fd, (err == SSL_ERROR_WANT_READ), remain);
        if (rdy <= 0)
        {
            snprintf(g_tls_errbuf, sizeof(g_tls_errbuf),
                     "handshake: select timeout/error (remain=%dms, rdy=%d)",
                     remain, rdy);
            SSL_free(ssl);
            set_blocking(fd);
            return NULL;
        }
    }
}

/**
 * @brief Send data over a TLS connection.
 *
 * @param conn       TLS handle from mex_tls_connect().
 * @param data       Buffer to send.
 * @param len        Number of bytes to send.
 * @param timeout_ms Write timeout in milliseconds.
 * @return Bytes sent (> 0) on success, -1 on error.
 */
int mex_tls_send(mex_tls_conn *conn, const char *data, int len, int timeout_ms)
{
    if (!conn || !conn->ssl)
        return -1;

    struct timeval start;
    gettimeofday(&start, NULL);
    int sent = 0;

    while (sent < len)
    {
        int ret = SSL_write(conn->ssl, data + sent, len - sent);
        if (ret > 0)
        {
            sent += ret;
            continue;
        }

        int err = SSL_get_error(conn->ssl, ret);
        if (err != SSL_ERROR_WANT_READ && err != SSL_ERROR_WANT_WRITE)
            return (sent > 0) ? sent : -1;

        int remain = remaining_ms(&start, timeout_ms);
        if (remain <= 0)
            return (sent > 0) ? sent : -1;

        if (wait_ready(conn->fd, (err == SSL_ERROR_WANT_READ), remain) <= 0)
            return (sent > 0) ? sent : -1;
    }

    return sent;
}

/**
 * @brief Receive data from a TLS connection.
 *
 * @param conn       TLS handle from mex_tls_connect().
 * @param buf        Buffer to receive into.
 * @param max_len    Maximum bytes to read.
 * @param timeout_ms Read timeout in milliseconds.
 * @return Bytes read (> 0) on success, 0 on timeout/EOF, -1 on error.
 */
int mex_tls_recv(mex_tls_conn *conn, char *buf, int max_len, int timeout_ms)
{
    if (!conn || !conn->ssl)
        return -1;

    struct timeval start;
    gettimeofday(&start, NULL);

    for (;;)
    {
        /* Check for buffered data in OpenSSL first */
        if (SSL_pending(conn->ssl) == 0)
        {
            int remain = remaining_ms(&start, timeout_ms);
            if (remain <= 0)
                return 0;   /* timeout */

            int rdy = wait_ready(conn->fd, 1, remain);
            if (rdy < 0)  return -1;
            if (rdy == 0) return 0;  /* timeout */
        }

        int ret = SSL_read(conn->ssl, (unsigned char *)buf, max_len);
        if (ret > 0)
            return ret;

        int err = SSL_get_error(conn->ssl, ret);

        if (err == SSL_ERROR_ZERO_RETURN)
            return 0;  /* clean EOF */

        if (err != SSL_ERROR_WANT_READ && err != SSL_ERROR_WANT_WRITE)
            return -1;

        int remain = remaining_ms(&start, timeout_ms);
        if (remain <= 0)
            return 0;

        if (wait_ready(conn->fd, (err == SSL_ERROR_WANT_READ), remain) <= 0)
            return 0;
    }
}

/**
 * @brief Close a TLS connection and free all associated resources.
 *
 * Does NOT close the underlying fd — caller is responsible for that.
 * Safe to call with NULL.
 *
 * @param conn  TLS handle to close.
 */
void mex_tls_close(mex_tls_conn *conn)
{
    if (!conn)
        return;

    if (conn->ssl)
    {
        SSL_shutdown(conn->ssl);
        SSL_free(conn->ssl);
    }

    /* Do NOT close conn->fd — caller owns it */
    free(conn);
}
