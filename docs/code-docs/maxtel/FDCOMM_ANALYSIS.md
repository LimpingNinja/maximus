# fdcomm.c Analysis: TCP Communications Driver

## Executive Summary

`fdcomm.c` is a **partially implemented** TCP/IP communications driver that was never integrated into the Maximus build. It contains the correct `AF_INET` socket code for real TCP listening but has significant incomplete sections and architectural issues that would need to be addressed before use.

**Verdict: Partially workable, requires significant cleanup before production use.**

## File Overview

| Property | Value |
|----------|-------|
| File | `comdll/fdcomm.c` |
| Author | Wes Garland |
| Era | 2003-2004 |
| Socket Type | `AF_INET` (TCP/IP) |
| Build Status | **Not included in Makefile** |
| Completion | ~60% functional |

## Code Analysis

### What Works

#### 1. TCP Socket Creation (Lines 278-298)
```c
if ((h = socket(AF_INET, SOCK_STREAM, 0)) < 0)
    return FALSE;

serv_addr.sin_family=AF_INET;
serv_addr.sin_addr.s_addr = INADDR_ANY;
serv_addr.sin_port=htons((short)port);

setsockopt(h, SOL_SOCKET, SO_REUSEADDR, (char *)&junk, sizeof(junk));

if (bind(h, (struct sockaddr *) &serv_addr, sizeof(serv_addr)))
    // error handling

if (listen(h, 1))
    // error handling
```
**Assessment: Correct and functional.**

#### 2. Connection Accept (Lines 399-434)
```c
if (!hc->fDCD && hc->saddr_p) {
    fd_set rfds;
    struct timeval tv;
    
    FD_ZERO(&rfds);
    FD_SET(hc->h, &rfds);
    tv.tv_sec = 0;
    tv.tv_usec = (0.25 * 1000000);

    if (select(hc->h + 1, &rfds, NULL, NULL, &tv) > 0) {
        fd = accept(hc->h, (struct sockaddr *)&hc->saddr_p, &addrSize);
        if (fd >= 0) {
            hc->listenfd = hc->h;
            hc->h = fd;
            hc->fDCD = 1;
            setsockopt(hc->h, IPPROTO_TCP, TCP_NODELAY, ...);
        }
    }
}
```
**Assessment: Correct pattern, properly disables Nagle algorithm.**

#### 3. Basic Read/Write (Lines 442-612)
```c
BOOL COMMAPI ComWrite(HCOMM hc, PVOID pvBuf, DWORD dwCount)
{
    do {
        bytesWritten = write(hc->h, (char *)pvBuf + totalBytesWritten, 
                            dwCount - totalBytesWritten);
        if ((bytesWritten < 0) && (errno != EINTR))
            return FALSE;
        totalBytesWritten += bytesWritten;
    } while(totalBytesWritten < dwCount);
    return TRUE;
}
```
**Assessment: Functional but simplistic. No buffering.**

### What's Incomplete

#### 1. Threading System (Lines 169-216)
```c
#if 0 /* later */
  if ((hc->hevTx=CreateEvent(NULL, FALSE, FALSE, NULL))==NULL ||
      (hc->hevRxDone=CreateEvent(NULL, FALSE, TRUE, NULL))==NULL ||
      ...
  
  if ((hc->hTx=CreateThread(NULL, 0, TxThread, (LPVOID)hc, CREATE_SUSPENDED, &dwJunk))==NULL ||
      (hc->hRx=CreateThread(NULL, 0, RxThread, (LPVOID)hc, CREATE_SUSPENDED, &dwJunk))==NULL ||
      ...
#endif
```
**Status: Completely disabled. Uses Win32 threading API that doesn't exist on UNIX.**

#### 2. Ring Buffers (Lines 474-543)
```c
#if 0 /* not yet */
BOOL COMMAPI ComWrite(HCOMM hc, PVOID pvBuf, DWORD dwCount)
{
    while (dwCount) {
        dwMaxBytes = QueueGetFreeContig(&hc->cqTx);
        // Ring buffer management
        memmove(hc->cqTx.pbTail, pvBuf, dwMaxBytes);
        QueueInsertContig(&hc->cqTx, dwMaxBytes);
        SetEvent(hc->hevTx);
    }
}
#endif
```
**Status: Disabled. The `Queue*` functions are referenced but not defined.**

#### 3. Buffer Initialization (Lines 119-147)
```c
static BOOL _InitBuffers(HCOMM hc, DWORD dwRxBuf, DWORD dwTxBuf)
{
    if ((hc->cqTx.pbBuf=malloc(dwRxBuf))==NULL ||  // Note: bug - should be dwTxBuf
        (hc->cqRx.pbBuf=malloc(dwTxBuf))==NULL)    // Note: bug - should be dwRxBuf
        return FALSE;
    
    hc->cqTx.pbHead = hc->cqTx.pbTail = hc->cqTx.pbBuf;
    // ...
}
```
**Status: Has bugs (buffer sizes swapped), and `cqTx`/`cqRx` members don't exist in the struct.**

### Structural Issues

#### 1. Missing Struct Members

The code references struct members that don't exist in `comstruct.h`:

```c
// Referenced in fdcomm.c but not in struct _hcomm:
hc->cqTx         // Circular queue for transmit
hc->cqRx         // Circular queue for receive
hc->hevTx        // Event handle
hc->hevRxDone    // Event handle
hc->hTx          // Thread handle
hc->hRx          // Thread handle
hc->hMn          // Monitor thread handle
hc->fDie         // Kill flag
hc->cThreads     // Thread count
hc->listenfd     // Listen file descriptor (EXISTS in comstruct.h)
```

#### 2. Win32 API Dependencies

fdcomm.c uses Win32 threading/event APIs that need UNIX equivalents:

| Win32 API | UNIX Equivalent | Status |
|-----------|-----------------|--------|
| `CreateThread` | `pthread_create` | Not implemented |
| `CreateEvent` | `pthread_cond_t` | Not implemented |
| `SetEvent` | `pthread_cond_signal` | Not implemented |
| `WaitForSingleObject` | `pthread_cond_wait` | Not implemented |
| `ResumeThread` | N/A (start running) | Not implemented |

#### 3. Inconsistent Handle Types

```c
// fdcomm.c line 152
BOOL COMMAPI ComOpenHandle(int hfComm, HCOMM *phc, ...)
                          ^^^
// vs ipcomm.c
BOOL COMMAPI ComOpenHandle(COMMHANDLE h, HCOMM *phc, ...)
                          ^^^^^^^^^^
```

### Platform Compatibility Analysis

| Platform | Status | Notes |
|----------|--------|-------|
| Linux x86_64 | ⚠️ Partial | TCP code works, threading disabled |
| Linux arm64 | ⚠️ Partial | Same as x86_64 |
| macOS arm64 | ⚠️ Partial | Same, may need `SO_NOSIGPIPE` |
| macOS x86_64 | ⚠️ Partial | Same as arm64 |
| FreeBSD | ⚠️ Partial | Likely works, untested |
| Windows | ❌ No | `#error UNIX only!` at line 34 |

### Security Considerations

1. **No address binding restriction** - Listens on `INADDR_ANY`
2. **No connection authentication** - Relies on Maximus login
3. **No TLS/SSL** - Plain text only (standard for era)
4. **Potential DoS** - `listen(h, 1)` has backlog of only 1

## Comparison with ipcomm.c

| Feature | ipcomm.c | fdcomm.c |
|---------|----------|----------|
| Socket Type | `AF_UNIX` | `AF_INET` |
| Threading | None (async I/O) | Disabled Win32 |
| Buffering | Direct I/O | Ring buffers (disabled) |
| Fork Support | Yes (respawn trick) | No |
| Telnet NVT | Optional (`-DTELNET`) | No |
| Build Status | ✅ Active | ❌ Not built |
| Completeness | ~90% | ~60% |

## Recommendations

### Option A: Fix fdcomm.c (High Effort)

1. **Add missing struct members** to `comstruct.h`
2. **Replace Win32 threading** with pthreads
3. **Fix buffer size bug** in `_InitBuffers`
4. **Add to Makefile** as alternative driver
5. **Implement driver selection** mechanism
6. **Add Telnet NVT support** (IAC sequences)
7. **Test extensively**

Estimated effort: 40-80 hours

### Option B: Modify ipcomm.c (Medium Effort)

1. **Change `AF_UNIX` to `AF_INET`** in socket creation
2. **Replace `sockaddr_un` with `sockaddr_in`**
3. **Remove UNIX socket path logic**
4. **Keep existing async I/O model**
5. **Maintain fork/respawn support**

Estimated effort: 8-16 hours

### Option C: External Wrapper (Low Effort)

Use inetd/xinetd/socat as documented in `TELNET_DEPLOYMENT.md`.

Estimated effort: 1-2 hours

## Code to Enable fdcomm.c in Build

If you want to experiment, modify `comdll/Makefile`:

```makefile
# Current:
libcomm.so: ipcomm.o common.o modemcom.o wincomm.o

# To test fdcomm (replaces ipcomm):
libcomm.so: fdcomm.o common.o modemcom.o wincomm.o

# Or to build both (requires driver selection logic):
libcomm.so: ipcomm.o fdcomm.o common.o modemcom.o wincomm.o
```

**Warning:** This will likely fail to link due to missing symbols and duplicate definitions. Significant code changes required.

## Historical Context

From CVS logs and code comments:

- **May 2003**: Wes Garland created initial fdcomm.c
- **Purpose**: "hackish way to use a pair of file descriptors to replace comm.dll"
- **Intent**: Support inetd-style operation and direct TCP
- **Status**: Never completed, focus shifted to ipcomm.c
- **Last modified**: 2004 (based on `$Id` tag)

The threading model was designed for Windows NT but never ported to UNIX pthreads. The `#if 0 /* later */` comments indicate planned future work that never happened.

## Conclusion

fdcomm.c contains the **correct TCP socket code** but is wrapped in an **incomplete threading framework**. The simplest path forward is either:

1. **Modify ipcomm.c** to use TCP (preserving its working async model)
2. **Use external tools** (inetd/socat) as originally intended

Completing fdcomm.c would require significant effort for minimal gain over these alternatives.

---

*Analysis performed for Maximus 3.04a - November 2025*
