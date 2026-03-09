/*
 * mex_tls.h — TLS wrapper header for MEX socket intrinsics
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

#ifndef MEX_TLS_H
#define MEX_TLS_H

#ifdef __cplusplus
extern "C" {
#endif

/** Opaque handle to a TLS connection. */
typedef struct mex_tls_conn mex_tls_conn;

/**
 * @brief One-time global TLS library init.
 * Safe to call multiple times; only the first call does work.
 */
void mex_tls_global_init(void);

/**
 * @brief Global TLS library cleanup. Call at program shutdown.
 */
void mex_tls_global_cleanup(void);

/**
 * @brief Create a TLS connection on an already-connected socket fd.
 *
 * Performs the full TLS handshake with the given timeout. The socket
 * should be in blocking mode on entry; this function manages non-blocking
 * internally as needed.
 *
 * @param fd          Connected TCP socket (caller retains ownership).
 * @param hostname    Server hostname for SNI.
 * @param timeout_ms  Handshake timeout in milliseconds.
 * @return Opaque TLS handle on success, NULL on failure.
 */
mex_tls_conn *mex_tls_connect(int fd, const char *hostname, int timeout_ms);

/**
 * @brief Send data over a TLS connection.
 * @param conn       TLS handle from mex_tls_connect().
 * @param data       Buffer to send.
 * @param len        Number of bytes to send.
 * @param timeout_ms Write timeout in milliseconds.
 * @return Bytes sent (> 0) on success, -1 on error.
 */
int mex_tls_send(mex_tls_conn *conn, const char *data, int len, int timeout_ms);

/**
 * @brief Receive data from a TLS connection.
 * @param conn       TLS handle from mex_tls_connect().
 * @param buf        Buffer to receive into.
 * @param max_len    Maximum bytes to read.
 * @param timeout_ms Read timeout in milliseconds.
 * @return Bytes read (> 0) on success, 0 on timeout/EOF, -1 on error.
 */
int mex_tls_recv(mex_tls_conn *conn, char *buf, int max_len, int timeout_ms);

/**
 * @brief Return a human-readable string describing the last TLS error.
 *
 * The returned pointer is valid until the next mex_tls_* call.
 * Returns "(no error)" if no error has occurred.
 */
const char *mex_tls_last_error(void);

/**
 * @brief Close a TLS connection and free all resources.
 *
 * Does NOT close the underlying fd — caller is responsible for that.
 * Safe to call with NULL.
 *
 * @param conn  TLS handle (set to NULL after this call).
 */
void mex_tls_close(mex_tls_conn *conn);

#ifdef __cplusplus
}
#endif

#endif /* MEX_TLS_H */
