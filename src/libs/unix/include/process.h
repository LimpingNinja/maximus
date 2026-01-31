#ifndef _PROCESS_H
#define _PROCESS_H

#ifndef P_WAIT
#define P_WAIT          1
#endif
#ifndef P_NOWAIT
#define P_NOWAIT        2
#endif
#ifndef P_NOWAITO
#define P_NOWAITO       3
#endif
#ifndef P_OVERLAY
#define P_OVERLAY       4
#endif

int spawnvp(int mode, const char *file, char *const argv[]);

#endif
