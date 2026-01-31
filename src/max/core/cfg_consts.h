#ifndef __CFG_CONSTS_H_DEFINED
#define __CFG_CONSTS_H_DEFINED

#ifndef CTL_VER
#define CTL_VER             9
#endif

#ifndef LOG_TERSE
#define LOG_TERSE       0x02
#endif
#ifndef LOG_VERBOSE
#define LOG_VERBOSE     0x04
#endif
#ifndef LOG_TRACE
#define LOG_TRACE       0x06
#endif

#ifndef LOG_terse
#define LOG_terse       LOG_TERSE
#endif
#ifndef LOG_verbose
#define LOG_verbose     LOG_VERBOSE
#endif
#ifndef LOG_trace
#define LOG_trace       LOG_TRACE
#endif

#ifndef CHARSET_SWEDISH
#define CHARSET_SWEDISH   0x01
#endif
#ifndef CHARSET_CHINESE
#define CHARSET_CHINESE   0x02
#endif

#ifndef XTERNEXIT
#define XTERNEXIT       0x40
#endif
#ifndef XTERNBATCH
#define XTERNBATCH      0x80
#endif

#ifndef NLVER_5
#define NLVER_5         5
#endif
#ifndef NLVER_6
#define NLVER_6         6
#endif
#ifndef NLVER_7
#define NLVER_7         7
#endif
#ifndef NLVER_FD
#define NLVER_FD       32
#endif

#ifndef MULTITASKER_AUTO
#define MULTITASKER_AUTO        -1
#endif
#ifndef MULTITASKER_NONE
#define MULTITASKER_NONE        0
#endif
#ifndef MULTITASKER_DOUBLEDOS
#define MULTITASKER_DOUBLEDOS   1
#endif
#ifndef MULTITASKER_DESQVIEW
#define MULTITASKER_DESQVIEW    2
#endif
#ifndef MULTITASKER_TOPVIEW
#define MULTITASKER_TOPVIEW     3
#endif
#ifndef MULTITASKER_MLINK
#define MULTITASKER_MLINK       4
#endif
#ifndef MULTITASKER_MSWINDOWS
#define MULTITASKER_MSWINDOWS   5
#endif
#ifndef MULTITASKER_OS2
#define MULTITASKER_OS2         6
#endif
#ifndef MULTITASKER_PCMOS
#define MULTITASKER_PCMOS       7
#endif
#ifndef MULTITASKER_NT
#define MULTITASKER_NT          8
#endif
#ifndef MULTITASKER_UNIX
#define MULTITASKER_UNIX        9
#endif

#ifndef MULTITASKER_auto
#define MULTITASKER_auto        MULTITASKER_AUTO
#endif
#ifndef MULTITASKER_none
#define MULTITASKER_none        MULTITASKER_NONE
#endif
#ifndef MULTITASKER_doubledos
#define MULTITASKER_doubledos   MULTITASKER_DOUBLEDOS
#endif
#ifndef MULTITASKER_desqview
#define MULTITASKER_desqview    MULTITASKER_DESQVIEW
#endif
#ifndef MULTITASKER_topview
#define MULTITASKER_topview     MULTITASKER_TOPVIEW
#endif
#ifndef MULTITASKER_mlink
#define MULTITASKER_mlink       MULTITASKER_MLINK
#endif
#ifndef MULTITASKER_mswindows
#define MULTITASKER_mswindows   MULTITASKER_MSWINDOWS
#endif
#ifndef MULTITASKER_os2
#define MULTITASKER_os2         MULTITASKER_OS2
#endif
#ifndef MULTITASKER_pcmos
#define MULTITASKER_pcmos       MULTITASKER_PCMOS
#endif
#ifndef MULTITASKER_nt
#define MULTITASKER_nt          MULTITASKER_NT
#endif
#ifndef MULTITASKER_unix
#define MULTITASKER_unix        MULTITASKER_UNIX
#endif

#endif
