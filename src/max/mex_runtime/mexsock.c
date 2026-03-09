/*
 * mexsock.c — MEX socket intrinsics (TCP/HTTP)
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

#define MAX_LANG_m_area
#include "mexall.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <fcntl.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <sys/select.h>
#include <sys/time.h>
#include <sys/ioctl.h>
#include <netinet/in.h>
#include <netdb.h>
#include <arpa/inet.h>

/* TLS support via isolated OpenSSL wrapper (see mex_tls.c / mex_tls.h) */
#include "mex_tls.h"

#ifdef MEX

/*------------------------------------------------------------------------*
 * Constants                                                              *
 *------------------------------------------------------------------------*/

#define MAX_MEXSOCK        8    /**< Maximum concurrent socket handles     */
#define SOCK_DEFAULT_TMO   5000 /**< Default timeout in ms if script passes 0 */
#define SOCK_MAX_TMO      30000 /**< Hard cap on timeout in ms             */
#define HTTP_MAX_RESPONSE  131072 /**< Max HTTP response body size (128 KB) */
#define HTTP_MAX_HEADER    4096  /**< Max HTTP header block size           */
#define MAX_REDIRECTS      5     /**< Max HTTP redirects to follow         */
#define TLS_HANDSHAKE_TMO  500   /**< Default TLS handshake timeout (ms)   */

/* Status constants — must match socket.mh */
#define MEX_SOCK_CLOSED     0
#define MEX_SOCK_CONNECTED  1
#define MEX_SOCK_ERROR     -1

/*------------------------------------------------------------------------*
 * Handle table                                                           *
 *------------------------------------------------------------------------*/

/** Per-handle state for an open socket. */
typedef struct _mex_sock {
    int   fd;           /**< OS socket fd, -1 = unused slot              */
    int   connected;    /**< 1 = connected, 0 = closed, -1 = error       */
    char  host[256];    /**< Remote host (for logging)                   */
    int   port;         /**< Remote port                                 */
} MEX_SOCK;

static MEX_SOCK g_mex_socks[MAX_MEXSOCK];
static int g_mex_socks_initialized = 0;

/** @brief Ensure the socket table is initialized (fd = -1 for all slots). */
static void mex_sock_ensure_init(void)
{
    if (g_mex_socks_initialized)
        return;
    for (int i = 0; i < MAX_MEXSOCK; i++)
    {
        g_mex_socks[i].fd = -1;
        g_mex_socks[i].connected = MEX_SOCK_CLOSED;
        g_mex_socks[i].host[0] = '\0';
        g_mex_socks[i].port = 0;
    }
    g_mex_socks_initialized = 1;
}

/* Forward declarations for plain-TCP I/O helpers */
static int mex_sock_send_timeout(int fd, const char *data, int len, int timeout_ms);
static int mex_sock_recv_timeout(int fd, char *buf, int max_len, int timeout_ms);

/*------------------------------------------------------------------------*
 * TLS support (HTTPS) — via isolated OpenSSL wrapper (mex_tls.c)        *
 *------------------------------------------------------------------------*/

/**
 * @brief Connection wrapper for HTTP helper — plain fd or fd + TLS.
 *
 * Only used internally by mex_http_request(). Raw socket intrinsics
 * (sock_open/close/send/recv) remain plain TCP.
 */
typedef struct _mex_http_conn {
    int            fd;
    mex_tls_conn  *tls;   /**< Non-NULL for HTTPS connections */
} MEX_HTTP_CONN;

/** @brief Send data over an HTTP connection (plain or TLS). */
static int mex_http_send(MEX_HTTP_CONN *c, const char *data, int len, int timeout_ms)
{
    if (c->tls)
        return mex_tls_send(c->tls, data, len, timeout_ms);
    return mex_sock_send_timeout(c->fd, data, len, timeout_ms);
}

/** @brief Receive data from an HTTP connection (plain or TLS) with timeout. */
static int mex_http_recv(MEX_HTTP_CONN *c, char *buf, int max_len, int timeout_ms)
{
    if (c->tls)
    {
        int rc = mex_tls_recv(c->tls, buf, max_len, timeout_ms);
        if (rc > 0)
            buf[rc] = '\0';
        return rc;
    }
    return mex_sock_recv_timeout(c->fd, buf, max_len, timeout_ms);
}

/** @brief Close an HTTP connection, freeing TLS state if present. */
static void mex_http_conn_close(MEX_HTTP_CONN *c)
{
    if (c->tls)
    {
        mex_tls_close(c->tls);
        c->tls = NULL;
    }
    if (c->fd >= 0)
    {
        close(c->fd);
        c->fd = -1;
    }
}

/*------------------------------------------------------------------------*
 * Internal helpers                                                       *
 *------------------------------------------------------------------------*/

/**
 * @brief Clamp a timeout value to sane bounds.
 * @param timeout_ms Requested timeout from script (0 = use default).
 * @return Clamped timeout in milliseconds.
 */
static int mex_sock_clamp_timeout(int timeout_ms)
{
    if (timeout_ms <= 0)
        return SOCK_DEFAULT_TMO;
    if (timeout_ms > SOCK_MAX_TMO)
        return SOCK_MAX_TMO;
    return timeout_ms;
}

/**
 * @brief Find a free slot in the socket handle table.
 * @return Slot index 0..MAX_MEXSOCK-1 or -1 if full.
 */
static int mex_sock_alloc_slot(void)
{
    for (int i = 0; i < MAX_MEXSOCK; i++)
    {
        if (g_mex_socks[i].fd == -1)
            return i;
    }
    return -1;
}

/**
 * @brief Free a socket slot — closes the fd and resets state.
 * @param slot Slot index.
 */
static void mex_sock_free_slot(int slot)
{
    if (slot < 0 || slot >= MAX_MEXSOCK)
        return;

    if (g_mex_socks[slot].fd >= 0)
        close(g_mex_socks[slot].fd);

    g_mex_socks[slot].fd = -1;
    g_mex_socks[slot].connected = MEX_SOCK_CLOSED;
    g_mex_socks[slot].host[0] = '\0';
    g_mex_socks[slot].port = 0;
}

/**
 * @brief Validate a socket handle from MEX.
 * @param sh Handle value from script.
 * @return Pointer to MEX_SOCK or NULL if invalid.
 */
static MEX_SOCK *mex_sock_validate(int sh)
{
    if (sh < 0 || sh >= MAX_MEXSOCK)
        return NULL;
    if (g_mex_socks[sh].fd == -1)
        return NULL;
    return &g_mex_socks[sh];
}

/**
 * @brief Set a socket to non-blocking mode.
 * @param fd Socket file descriptor.
 * @return 0 on success, -1 on error.
 */
static int mex_sock_set_nonblock(int fd)
{
    int flags = fcntl(fd, F_GETFL, 0);
    if (flags == -1)
        return -1;
    return fcntl(fd, F_SETFL, flags | O_NONBLOCK);
}

/**
 * @brief Set a socket back to blocking mode.
 * @param fd Socket file descriptor.
 * @return 0 on success, -1 on error.
 */
static int mex_sock_set_blocking(int fd)
{
    int flags = fcntl(fd, F_GETFL, 0);
    if (flags == -1)
        return -1;
    return fcntl(fd, F_SETFL, flags & ~O_NONBLOCK);
}

/**
 * @brief Connect to a remote host:port with timeout.
 *
 * Uses non-blocking connect + select() to enforce the timeout. On success,
 * restores the socket to blocking mode for subsequent I/O.
 *
 * @param host      Hostname or IP address.
 * @param port      TCP port number.
 * @param timeout_ms Timeout in milliseconds (already clamped).
 * @return Socket fd on success, -1 on error.
 */
static int mex_sock_connect(const char *host, int port, int timeout_ms)
{
    struct addrinfo hints, *res, *rp;
    char port_str[16];
    int fd = -1;
    int rc;
    int addr_count;

    memset(&hints, 0, sizeof(hints));
    hints.ai_family = AF_INET;       /* IPv4 only — avoids dual-stack delays */
    hints.ai_socktype = SOCK_STREAM;

    snprintf(port_str, sizeof(port_str), "%d", port);

    rc = getaddrinfo(host, port_str, &hints, &res);
    if (rc != 0)
    {
        logit("!MEX sock_connect: getaddrinfo(%s:%d) failed: %s",
              host, port, gai_strerror(rc));
        return -1;
    }

    /* Count addresses so we can split the timeout across them.
     * This avoids the dual-stack penalty: if IPv6 is unreachable,
     * we don't burn the entire timeout before trying IPv4. */
    addr_count = 0;
    for (rp = res; rp != NULL; rp = rp->ai_next)
        addr_count++;

    /* Per-address timeout: split evenly, minimum 500ms, max 2s.
     * This gives fast failover without starving any address. */
    int per_addr_tmo = (addr_count > 0) ? timeout_ms / addr_count : timeout_ms;
    if (per_addr_tmo < 500)
        per_addr_tmo = 500;
    if (per_addr_tmo > 2000)
        per_addr_tmo = 2000;

    for (rp = res; rp != NULL; rp = rp->ai_next)
    {
        fd = socket(rp->ai_family, rp->ai_socktype, rp->ai_protocol);
        if (fd < 0)
            continue;

        /* Set non-blocking for timeout-controlled connect */
        if (mex_sock_set_nonblock(fd) < 0)
        {
            close(fd);
            fd = -1;
            continue;
        }

        rc = connect(fd, rp->ai_addr, rp->ai_addrlen);
        if (rc == 0)
        {
            /* Immediate connection (rare but possible on localhost) */
            mex_sock_set_blocking(fd);
            break;
        }

        if (errno != EINPROGRESS)
        {
            close(fd);
            fd = -1;
            continue;
        }

        /* Wait for connect to complete — per-address timeout */
        fd_set wfds;
        struct timeval tv;
        FD_ZERO(&wfds);
        FD_SET(fd, &wfds);
        tv.tv_sec = per_addr_tmo / 1000;
        tv.tv_usec = (per_addr_tmo % 1000) * 1000;

        rc = select(fd + 1, NULL, &wfds, NULL, &tv);
        if (rc <= 0)
        {
            /* Timeout or error — try next address quickly */
            close(fd);
            fd = -1;
            continue;
        }

        /* Check if connect actually succeeded */
        int err = 0;
        socklen_t len = sizeof(err);
        getsockopt(fd, SOL_SOCKET, SO_ERROR, &err, &len);
        if (err != 0)
        {
            close(fd);
            fd = -1;
            continue;
        }

        /* Connected! Restore blocking mode */
        mex_sock_set_blocking(fd);
        break;
    }

    freeaddrinfo(res);
    return fd;
}

/**
 * @brief Receive data with timeout via select().
 *
 * @param fd        Socket file descriptor.
 * @param buf       Output buffer.
 * @param max_len   Maximum bytes to read.
 * @param timeout_ms Timeout in milliseconds.
 * @return Bytes received, 0 on timeout, -1 on error/disconnect.
 */
static int mex_sock_recv_timeout(int fd, char *buf, int max_len, int timeout_ms)
{
    fd_set rfds;
    struct timeval tv;
    int rc;

    FD_ZERO(&rfds);
    FD_SET(fd, &rfds);
    tv.tv_sec = timeout_ms / 1000;
    tv.tv_usec = (timeout_ms % 1000) * 1000;

    rc = select(fd + 1, &rfds, NULL, NULL, &tv);
    if (rc < 0)
        return -1;   /* select error */
    if (rc == 0)
        return 0;    /* timeout */

    rc = recv(fd, buf, max_len, 0);
    if (rc <= 0)
        return -1;   /* disconnect or error */

    buf[rc] = '\0';  /* null-terminate for MEX string safety */
    return rc;
}

/**
 * @brief Send data with timeout via select().
 *
 * @param fd        Socket file descriptor.
 * @param data      Data buffer to send.
 * @param len       Number of bytes to send.
 * @param timeout_ms Timeout in milliseconds.
 * @return Bytes sent, or -1 on error.
 */
static int mex_sock_send_timeout(int fd, const char *data, int len, int timeout_ms)
{
    fd_set wfds;
    struct timeval tv;
    int rc;

    FD_ZERO(&wfds);
    FD_SET(fd, &wfds);
    tv.tv_sec = timeout_ms / 1000;
    tv.tv_usec = (timeout_ms % 1000) * 1000;

    rc = select(fd + 1, NULL, &wfds, NULL, &tv);
    if (rc <= 0)
        return -1;

    rc = send(fd, data, len, 0);
    return rc;
}

/*------------------------------------------------------------------------*
 * URL parser (for http_request)                                          *
 *------------------------------------------------------------------------*/

/** Parsed URL components. */
typedef struct _parsed_url {
    char host[256];
    int  port;
    char path[512];
    int  use_tls;   /**< 1 for https://, 0 for http:// */
} PARSED_URL;

/**
 * @brief Parse an http:// or https:// URL into host, port, path, and TLS flag.
 *
 * @param url   Input URL string.
 * @param out   Output parsed components.
 * @return 0 on success, -1 on error.
 */
static int mex_parse_url(const char *url, PARSED_URL *out)
{
    const char *p;

    memset(out, 0, sizeof(*out));
    strcpy(out->path, "/");

    /* Detect scheme */
    if (strncasecmp(url, "https://", 8) == 0)
    {
        out->use_tls = 1;
        out->port = 443;
        p = url + 8;
    }
    else if (strncasecmp(url, "http://", 7) == 0)
    {
        out->use_tls = 0;
        out->port = 80;
        p = url + 7;
    }
    else
    {
        logit("!MEX http_request: unsupported URL scheme");
        return -1;
    }

    /* Extract host (and optional :port) */
    const char *host_end = strpbrk(p, ":/?");
    if (!host_end)
    {
        /* URL is just scheme://hostname */
        strncpy(out->host, p, sizeof(out->host) - 1);
        return 0;
    }

    size_t host_len = host_end - p;
    if (host_len >= sizeof(out->host))
        host_len = sizeof(out->host) - 1;
    strncpy(out->host, p, host_len);
    out->host[host_len] = '\0';

    p = host_end;

    if (*p == ':')
    {
        p++;
        out->port = atoi(p);
        while (*p >= '0' && *p <= '9')
            p++;
    }

    if (*p == '/' || *p == '?')
        strncpy(out->path, p, sizeof(out->path) - 1);

    return 0;
}

/*------------------------------------------------------------------------*
 * HTTP helper                                                            *
 *------------------------------------------------------------------------*/

/**
 * @brief Perform a complete HTTP/1.1 request and return the response body.
 *
 * Supports both http:// and https:// URLs. Follows 301/302/307/308
 * redirects up to MAX_REDIRECTS times. Uses MEX_HTTP_CONN for TLS-aware
 * I/O via OpenSSL when available.
 *
 * @param url        Full URL (http:// or https://).
 * @param method     HTTP method ("GET", "POST", etc.).
 * @param headers    Extra headers (\r\n separated) or empty string.
 * @param body       Request body (for POST/PUT) or empty string.
 * @param response   Output buffer for response body (caller frees).
 * @param resp_len   Output: length of response body.
 * @param timeout_ms Timeout per I/O operation.
 * @return HTTP status code (200, 404, etc.) or -1 on error.
 */
static int mex_http_request(const char *url, const char *method,
                            const char *headers, const char *body,
                            char **response, int *resp_len, int timeout_ms)
{
    char current_url[2048];
    int redir;

    *response = NULL;
    *resp_len = 0;

    strncpy(current_url, url, sizeof(current_url) - 1);
    current_url[sizeof(current_url) - 1] = '\0';

    for (redir = 0; redir <= MAX_REDIRECTS; redir++)
    {
        PARSED_URL pu;
        int status_code = -1;
        int content_length = -1;

        if (mex_parse_url(current_url, &pu) < 0)
            return -1;

        /* TCP connect */
        int fd = mex_sock_connect(pu.host, pu.port, timeout_ms);
        if (fd < 0)
        {
            logit("!MEX http_request: connect to %s:%d failed", pu.host, pu.port);
            return -1;
        }

        /* Set up connection wrapper */
        MEX_HTTP_CONN conn;
        conn.fd = fd;
        conn.tls = NULL;

        /* TLS handshake for HTTPS via OpenSSL (isolated in mex_tls.c) */
        if (pu.use_tls)
        {
            int hs_tmo = ngcfg_get_int("mex.sockets.tls_handshake_timeout_ms");
            if (hs_tmo <= 0)
                hs_tmo = TLS_HANDSHAKE_TMO;

            conn.tls = mex_tls_connect(fd, pu.host, hs_tmo);
            if (!conn.tls)
            {
                logit("!MEX http_request: TLS failed for %s:%d (cap %dms): %s",
                      pu.host, pu.port, hs_tmo, mex_tls_last_error());
                close(fd);
                return -1;
            }
        }

        /* Build request */
        char req[2048];
        int body_len = body ? (int)strlen(body) : 0;
        int req_len;

        if (body_len > 0)
        {
            req_len = snprintf(req, sizeof(req),
                "%s %s HTTP/1.1\r\n"
                "Host: %s\r\n"
                "Connection: close\r\n"
                "Content-Length: %d\r\n"
                "%s"
                "\r\n",
                method, pu.path, pu.host, body_len,
                headers ? headers : "");
        }
        else
        {
            req_len = snprintf(req, sizeof(req),
                "%s %s HTTP/1.1\r\n"
                "Host: %s\r\n"
                "Connection: close\r\n"
                "%s"
                "\r\n",
                method, pu.path, pu.host,
                headers ? headers : "");
        }

        /* Send request */
        if (mex_http_send(&conn, req, req_len, timeout_ms) < 0)
        {
            logit("!MEX http_request: send failed");
            mex_http_conn_close(&conn);
            return -1;
        }

        /* Send body if present */
        if (body_len > 0)
        {
            if (mex_http_send(&conn, body, body_len, timeout_ms) < 0)
            {
                logit("!MEX http_request: send body failed");
                mex_http_conn_close(&conn);
                return -1;
            }
        }

        /* Read response — accumulate until EOF */
        char *raw = malloc(HTTP_MAX_RESPONSE + 1);
        if (!raw)
        {
            mex_http_conn_close(&conn);
            return -1;
        }

        int raw_len = 0;
        int done = 0;

        while (!done && raw_len < HTTP_MAX_RESPONSE)
        {
            int got = mex_http_recv(&conn, raw + raw_len,
                                    HTTP_MAX_RESPONSE - raw_len,
                                    timeout_ms);
            if (got <= 0)
                done = 1;
            else
                raw_len += got;
        }

        mex_http_conn_close(&conn);

        if (raw_len <= 0)
        {
            free(raw);
            return -1;
        }

        raw[raw_len] = '\0';

        /* Parse status line: "HTTP/1.x NNN reason\r\n" */
        char *status_start = strstr(raw, "HTTP/");
        if (status_start)
        {
            char *sp = strchr(status_start, ' ');
            if (sp)
                status_code = atoi(sp + 1);
        }

        /* Find header/body separator */
        char *hdr_end = strstr(raw, "\r\n\r\n");

        /* Handle redirects (301, 302, 307, 308) */
        if (status_code >= 301 && status_code <= 308 &&
            status_code != 304 && status_code != 305 && hdr_end)
        {
            char *loc = strcasestr(raw, "\r\nLocation:");
            if (loc && loc < hdr_end)
            {
                loc += 11;  /* skip "\r\nLocation:" */
                while (*loc == ' ' || *loc == '\t') loc++;

                char *loc_end = strpbrk(loc, "\r\n");
                if (loc_end)
                {
                    size_t loc_len = loc_end - loc;
                    if (loc_len > 0 && loc_len < sizeof(current_url))
                    {
                        /* Handle relative URLs */
                        if (loc[0] == '/')
                        {
                            char abs_url[2048];
                            const char *scheme = pu.use_tls ? "https" : "http";
                            int default_port = pu.use_tls ? 443 : 80;

                            if (pu.port != default_port)
                                snprintf(abs_url, sizeof(abs_url), "%s://%s:%d%.*s",
                                         scheme, pu.host, pu.port,
                                         (int)loc_len, loc);
                            else
                                snprintf(abs_url, sizeof(abs_url), "%s://%s%.*s",
                                         scheme, pu.host,
                                         (int)loc_len, loc);

                            strncpy(current_url, abs_url, sizeof(current_url) - 1);
                            current_url[sizeof(current_url) - 1] = '\0';
                        }
                        else
                        {
                            /* Absolute URL from Location header */
                            memcpy(current_url, loc, loc_len);
                            current_url[loc_len] = '\0';
                        }

                        logit("MEX http_request: %d redirect -> %s",
                              status_code, current_url);
                        free(raw);
                        continue;  /* Follow the redirect */
                    }
                }
            }
        }

        /* Not a redirect — extract body */
        if (hdr_end)
        {
            char *body_start = hdr_end + 4;
            int body_available = raw_len - (int)(body_start - raw);

            /* Check for chunked transfer-encoding */
            int is_chunked = 0;
            {
                char *te = strcasestr(raw, "\r\nTransfer-Encoding:");
                if (te && te < hdr_end && strcasestr(te, "chunked"))
                    is_chunked = 1;
            }

            if (is_chunked)
            {
                /* Decode chunked body in-place:
                 *   <hex-size>\r\n<data>\r\n ... 0\r\n\r\n */
                char *out = malloc(body_available + 1);
                if (out)
                {
                    int out_len = 0;
                    char *p = body_start;
                    char *end = body_start + body_available;

                    while (p < end)
                    {
                        /* Parse hex chunk size */
                        long chunk_sz = strtol(p, NULL, 16);
                        if (chunk_sz <= 0)
                            break;  /* Final 0-length chunk or parse error */

                        /* Skip past the size line (\r\n) */
                        char *data = strstr(p, "\r\n");
                        if (!data)
                            break;
                        data += 2;

                        /* Copy chunk data */
                        int avail = (int)(end - data);
                        int to_copy = (chunk_sz < avail) ? (int)chunk_sz : avail;
                        if (out_len + to_copy > HTTP_MAX_RESPONSE)
                            to_copy = HTTP_MAX_RESPONSE - out_len;

                        memcpy(out + out_len, data, to_copy);
                        out_len += to_copy;

                        /* Advance past chunk data + trailing \r\n */
                        p = data + chunk_sz;
                        if (p + 2 <= end && p[0] == '\r' && p[1] == '\n')
                            p += 2;
                    }

                    out[out_len] = '\0';
                    *response = out;
                    *resp_len = out_len;
                }
            }
            else
            {
                /* Non-chunked: use Content-Length or raw body size */
                char *cl = strcasestr(raw, "Content-Length:");
                if (cl && cl < hdr_end)
                {
                    content_length = atoi(cl + 15);
                    if (content_length < 0)
                        content_length = -1;
                }

                if (content_length >= 0 && content_length < body_available)
                    body_available = content_length;

                if (body_available > HTTP_MAX_RESPONSE)
                    body_available = HTTP_MAX_RESPONSE;

                char *resp_buf = malloc(body_available + 1);
                if (resp_buf)
                {
                    memcpy(resp_buf, body_start, body_available);
                    resp_buf[body_available] = '\0';
                    *response = resp_buf;
                    *resp_len = body_available;
                }
            }
        }

        free(raw);

        if (status_code <= 0)
            status_code = -1;

        return status_code;
    }

    logit("!MEX http_request: too many redirects (%d)", MAX_REDIRECTS);
    return -1;
}

/*------------------------------------------------------------------------*
 * Intrinsic implementations                                              *
 *------------------------------------------------------------------------*/

/**
 * @brief MEX: sock_open(host, port, timeout_ms) -> handle or -1
 *
 * Connect to a remote host:port with timeout. Returns a socket handle
 * (0..MAX_MEXSOCK-1) or -1 on failure.
 */
word EXPENTRY intrin_sock_open(void)
{
    MA ma;
    char *host;
    int port, timeout_ms;

    MexArgBegin(&ma);
    host       = MexArgGetString(&ma, FALSE);
    port       = (int)MexArgGetWord(&ma);
    timeout_ms = (int)MexArgGetWord(&ma);

    regs_2[0] = (word)-1;

    mex_sock_ensure_init();

    if (!host || !*host || port <= 0 || port > 65535)
    {
        if (host) free(host);
        return MexArgEnd(&ma);
    }

    timeout_ms = mex_sock_clamp_timeout(timeout_ms);

    /* TODO: Check sysop allow/deny list from TOML config before connecting */

    int slot = mex_sock_alloc_slot();
    if (slot == -1)
    {
        logit("!MEX sock_open: no free slots (max %d)", MAX_MEXSOCK);
        free(host);
        return MexArgEnd(&ma);
    }

    int fd = mex_sock_connect(host, port, timeout_ms);
    if (fd >= 0)
    {
        g_mex_socks[slot].fd = fd;
        g_mex_socks[slot].connected = MEX_SOCK_CONNECTED;
        g_mex_socks[slot].port = port;
        strncpy(g_mex_socks[slot].host, host, sizeof(g_mex_socks[slot].host) - 1);
        g_mex_socks[slot].host[sizeof(g_mex_socks[slot].host) - 1] = '\0';
        regs_2[0] = (word)slot;
        logit("MEX sock_open: connected to %s:%d (slot %d)", host, port, slot);
    }
    else
    {
        logit("!MEX sock_open: connect to %s:%d failed", host, port);
    }

    free(host);
    return MexArgEnd(&ma);
}

/**
 * @brief MEX: sock_close(sh) -> 0 or -1
 *
 * Close a socket handle and free the slot.
 */
word EXPENTRY intrin_sock_close(void)
{
    MA ma;
    int sh;

    MexArgBegin(&ma);
    sh = (int)MexArgGetWord(&ma);

    regs_2[0] = (word)-1;

    MEX_SOCK *s = mex_sock_validate(sh);
    if (!s)
        return MexArgEnd(&ma);

    logit("MEX sock_close: closing %s:%d (slot %d)", s->host, s->port, sh);
    mex_sock_free_slot(sh);
    regs_2[0] = 0;
    return MexArgEnd(&ma);
}

/**
 * @brief MEX: sock_send(sh, data, len) -> bytes sent or -1
 *
 * Send raw bytes over an open socket. The len parameter specifies how many
 * bytes of data to send (0 = send entire string length).
 */
word EXPENTRY intrin_sock_send(void)
{
    MA ma;
    int sh, len;
    char *data;

    MexArgBegin(&ma);
    sh   = (int)MexArgGetWord(&ma);
    data = MexArgGetString(&ma, FALSE);
    len  = (int)MexArgGetWord(&ma);

    regs_2[0] = (word)-1;

    MEX_SOCK *s = mex_sock_validate(sh);
    if (!s || !data)
    {
        if (data) free(data);
        return MexArgEnd(&ma);
    }

    int data_len = (int)strlen(data);
    if (len <= 0 || len > data_len)
        len = data_len;

    int sent = mex_sock_send_timeout(s->fd, data, len, SOCK_DEFAULT_TMO);
    free(data);

    if (sent < 0)
        s->connected = MEX_SOCK_ERROR;

    regs_2[0] = (word)sent;
    return MexArgEnd(&ma);
}

/**
 * @brief MEX: sock_recv(sh, ref buf, max_len, timeout_ms) -> bytes or -1
 *
 * Receive up to max_len bytes with timeout. Returns bytes received,
 * 0 on timeout, -1 on error/disconnect. The buffer is passed by reference.
 */
word EXPENTRY intrin_sock_recv(void)
{
    MA ma;
    int sh, max_len, timeout_ms;
    IADDR where;
    word wLen;

    MexArgBegin(&ma);
    sh         = (int)MexArgGetWord(&ma);
    MexArgGetRefString(&ma, &where, &wLen);
    max_len    = (int)MexArgGetWord(&ma);
    timeout_ms = (int)MexArgGetWord(&ma);

    regs_2[0] = (word)-1;

    MEX_SOCK *s = mex_sock_validate(sh);
    if (!s)
        return MexArgEnd(&ma);

    timeout_ms = mex_sock_clamp_timeout(timeout_ms);

    /* Cap recv size to prevent excessive allocation */
    if (max_len <= 0)
        max_len = 512;
    if (max_len > HTTP_MAX_RESPONSE)
        max_len = HTTP_MAX_RESPONSE;

    char *buf = malloc(max_len + 1);
    if (!buf)
        return MexArgEnd(&ma);

    int got = mex_sock_recv_timeout(s->fd, buf, max_len, timeout_ms);

    /* Store result into the ref string */
    MexKillString(&where);
    if (got > 0)
        MexStoreByteStringAt(MexIaddrToVM(&where), buf, got);
    else
        MexStoreByteStringAt(MexIaddrToVM(&where), "", 0);

    free(buf);

    if (got < 0)
        s->connected = MEX_SOCK_ERROR;

    regs_2[0] = (word)got;
    return MexArgEnd(&ma);
}

/**
 * @brief MEX: sock_status(sh) -> SOCK_CONNECTED, SOCK_CLOSED, or SOCK_ERROR
 *
 * Query the connection state of a socket handle.
 */
word EXPENTRY intrin_sock_status(void)
{
    MA ma;
    int sh;

    MexArgBegin(&ma);
    sh = (int)MexArgGetWord(&ma);

    MEX_SOCK *s = mex_sock_validate(sh);
    if (!s)
    {
        regs_2[0] = (word)MEX_SOCK_CLOSED;
        return MexArgEnd(&ma);
    }

    /* Quick liveness check: peek with zero timeout */
    if (s->connected == MEX_SOCK_CONNECTED)
    {
        fd_set rfds;
        struct timeval tv;
        char peek;

        FD_ZERO(&rfds);
        FD_SET(s->fd, &rfds);
        tv.tv_sec = 0;
        tv.tv_usec = 0;

        int rc = select(s->fd + 1, &rfds, NULL, NULL, &tv);
        if (rc > 0)
        {
            /* Data or EOF available — peek to distinguish */
            rc = recv(s->fd, &peek, 1, MSG_PEEK);
            if (rc == 0)
            {
                /* Peer closed connection */
                s->connected = MEX_SOCK_CLOSED;
            }
            else if (rc < 0 && errno != EAGAIN && errno != EWOULDBLOCK)
            {
                s->connected = MEX_SOCK_ERROR;
            }
            /* rc > 0 means data waiting, still connected */
        }
    }

    regs_2[0] = (word)s->connected;
    return MexArgEnd(&ma);
}

/**
 * @brief MEX: sock_avail(sh) -> bytes available or -1
 *
 * Return number of bytes available to read without blocking.
 */
word EXPENTRY intrin_sock_avail(void)
{
    MA ma;
    int sh;

    MexArgBegin(&ma);
    sh = (int)MexArgGetWord(&ma);

    regs_2[0] = (word)-1;

    MEX_SOCK *s = mex_sock_validate(sh);
    if (!s)
        return MexArgEnd(&ma);

    int avail = 0;
    if (ioctl(s->fd, FIONREAD, &avail) < 0)
        avail = -1;

    regs_2[0] = (word)avail;
    return MexArgEnd(&ma);
}

/**
 * @brief MEX: http_request(url, method, headers, body, ref response, timeout_ms)
 *        -> HTTP status code or -1
 *
 * Convenience function that performs a complete HTTP/1.1 request and returns
 * the response body in a ref string. Handles connect, framing, send, recv,
 * status parsing, and cleanup.
 */
word EXPENTRY intrin_http_request(void)
{
    MA ma;
    char *url, *method, *headers, *body;
    int timeout_ms;
    IADDR resp_where;
    word resp_wLen;

    MexArgBegin(&ma);
    url        = MexArgGetString(&ma, FALSE);
    method     = MexArgGetString(&ma, FALSE);
    headers    = MexArgGetString(&ma, FALSE);
    body       = MexArgGetString(&ma, FALSE);
    MexArgGetRefString(&ma, &resp_where, &resp_wLen);
    timeout_ms = (int)MexArgGetWord(&ma);

    regs_2[0] = (word)-1;

    if (!url || !method)
    {
        if (url)     free(url);
        if (method)  free(method);
        if (headers) free(headers);
        if (body)    free(body);
        return MexArgEnd(&ma);
    }

    timeout_ms = mex_sock_clamp_timeout(timeout_ms);

    mex_sock_ensure_init();

    /* TODO: Check sysop allow/deny list from TOML config before connecting */

    logit("MEX http_request: %s %s (timeout %dms)", method, url, timeout_ms);

    char *response = NULL;
    int resp_len = 0;
    int status = mex_http_request(url, method,
                                  headers ? headers : "",
                                  body ? body : "",
                                  &response, &resp_len, timeout_ms);

    /* Store response body into the ref string */
    MexKillString(&resp_where);
    if (response && resp_len > 0)
        MexStoreByteStringAt(MexIaddrToVM(&resp_where), response, resp_len);
    else
        MexStoreByteStringAt(MexIaddrToVM(&resp_where), "", 0);

    if (response)
        free(response);

    free(url);
    free(method);
    if (headers) free(headers);
    if (body)    free(body);

    if (status > 0)
        logit("MEX http_request: status %d, body %d bytes", status, resp_len);
    else
        logit("!MEX http_request: failed (status %d)", status);

    regs_2[0] = (word)status;
    return MexArgEnd(&ma);
}

/*------------------------------------------------------------------------*
 * Cleanup — called from intrin_term()                                    *
 *------------------------------------------------------------------------*/

/**
 * @brief Close all open socket handles. Called on MEX session teardown.
 */
void MexSockCleanup(void)
{
    if (!g_mex_socks_initialized)
        return;  /* Nothing to clean — table was never initialized */

    for (int i = 0; i < MAX_MEXSOCK; i++)
    {
        if (g_mex_socks[i].fd > 0)  /* >0: never close stdin/stdout/stderr */
        {
            logit("MEX sock cleanup: closing leaked slot %d (%s:%d)",
                  i, g_mex_socks[i].host, g_mex_socks[i].port);
            mex_sock_free_slot(i);
        }
    }
    g_mex_socks_initialized = 0;  /* Re-init on next session */

    mex_tls_global_cleanup();
}

/*------------------------------------------------------------------------*
 * Init — called once at startup to zero the handle table                 *
 *------------------------------------------------------------------------*/

/**
 * @brief Initialize the socket handle table. Call once before first use.
 */
void MexSockInit(void)
{
    for (int i = 0; i < MAX_MEXSOCK; i++)
    {
        g_mex_socks[i].fd = -1;
        g_mex_socks[i].connected = MEX_SOCK_CLOSED;
        g_mex_socks[i].host[0] = '\0';
        g_mex_socks[i].port = 0;
    }
}

#endif /* MEX */
