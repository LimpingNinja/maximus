#define MAX_LANG_global
#define MAX_LANG_m_area
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <errno.h>
#include <signal.h>
#include <fcntl.h>
#include <sys/select.h>
#include <sys/ioctl.h>
#include <termios.h>
#include <sys/wait.h>
#include <sys/types.h>
#include <string.h>
#ifdef DARWIN
#include <util.h>
#else
#include <pty.h>
#endif
#include <prog.h>
#include "ntcomm.h"
#include "process.h"
#include "io.h"

 extern void cdecl logit(char *format, ...);

/* Forward declaration for Puts from max_out.c */
extern void Puts(char *s);

#define unixfd(hc)      FileHandle_fromCommHandle(ComGetHandle(hc))

static void noop(int sig)
{
  ;
}

static void set_nonblock(int fd)
{
  int flags;

  if (fd < 0)
    return;

  flags = fcntl(fd, F_GETFL, 0);
  if (flags < 0)
    return;

  (void)fcntl(fd, F_SETFL, flags | O_NONBLOCK);
}

extern HCOMM hcModem;

int xxspawnvp(int mode, const char *Cfile, char *const argv[], int is_door32)
{
  pid_t		pid;
  int i;
  char cmd[4096];

  int session_fd = -1;
  int master_fd = -1;
  int slave_fd = -1;

  void (*old_sigchld)(int);

  old_sigchld = signal(SIGCHLD, noop);

  if (!argv || !argv[0] || !*argv[0])
  {
    errno = EINVAL;
    return -1;
  }

  if (hcModem)
    session_fd = unixfd(hcModem);

  /* Door32 doors use the session fd directly - no PTY needed */
  if (mode == P_WAIT && session_fd >= 0 && !is_door32)
  {
    if (openpty(&master_fd, &slave_fd, NULL, NULL, NULL) == 0)
    {
      set_nonblock(master_fd);
      set_nonblock(session_fd);
    }
    else
    {
      master_fd = -1;
      slave_fd = -1;
    }
  }

  if (mode != P_OVERLAY)
    pid = fork();
  else
    pid = 0; /* fake being a child */

  if (pid < 0)
  {
    logit("!xxspawnvp: fork() failed for '%s': %s", Cfile ? Cfile : (argv && argv[0] ? argv[0] : "(null)"), strerror(errno));
    signal(SIGCHLD, old_sigchld);
    return -1;
  }

  if (pid) /* Parent */
  {
    logit("@xxspawnvp: started pid=%ld mode=%d cmd='%s'", (long)pid, mode, Cfile ? Cfile : argv[0]);

    if (slave_fd >= 0)
    {
      close(slave_fd);
      slave_fd = -1;
    }

    if ((mode == P_NOWAIT) || (mode == P_NOWAITO))
    {
      if (master_fd >= 0)
        close(master_fd);
      signal(SIGCHLD, old_sigchld);
      return 0;
    }

    if (mode == P_WAIT)
    {
      int status;
      pid_t dead_kid;

      for (;;)
      {
        struct timeval tv;
        fd_set rfds;
        int maxfd;

        errno = 0;
        dead_kid = waitpid(pid, &status, WNOHANG);
        if (dead_kid == pid)
          break;

        if (dead_kid < 0)
        {
          if (errno == EINTR)
            continue;
          signal(SIGCHLD, old_sigchld);
          return -1;
        }

        if (hcModem && !ComIsOnline(hcModem))
        {
          if (pid > 0)
          {
            logit("@xxspawnvp: carrier lost; terminating pid=%ld cmd='%s'", (long)pid, Cfile ? Cfile : argv[0]);
            kill(pid, SIGTERM);
            usleep(250000);
            kill(pid, SIGKILL);
          }
        }

        FD_ZERO(&rfds);

        maxfd = -1;
        if (session_fd >= 0)
        {
          FD_SET(session_fd, &rfds);
          maxfd = session_fd;
        }

        if (master_fd >= 0)
        {
          FD_SET(master_fd, &rfds);
          if (master_fd > maxfd)
            maxfd = master_fd;
        }

        tv.tv_sec = 0;
        tv.tv_usec = 250000;

        if (maxfd < 0)
        {
          usleep(250000);
          continue;
        }

        if (select(maxfd + 1, &rfds, NULL, NULL, &tv) <= 0)
          continue;

        if (session_fd >= 0 && master_fd >= 0 && FD_ISSET(session_fd, &rfds))
        {
          char buf[4096];
          ssize_t n = read(session_fd, buf, sizeof(buf));

          if (n > 0)
          {
            ssize_t off = 0;
            while (off < n)
            {
              ssize_t w = write(master_fd, buf + off, (size_t)(n - off));
              if (w > 0)
                off += w;
              else if (w < 0 && (errno == EINTR))
                continue;
              else if (w < 0 && (errno == EAGAIN || errno == EWOULDBLOCK))
                break;
              else
                break;
            }
          }
        }

        if (session_fd >= 0 && master_fd >= 0 && FD_ISSET(master_fd, &rfds))
        {
          char buf[4096];
          ssize_t n = read(master_fd, buf, sizeof(buf));

          if (n > 0)
          {
            ssize_t off = 0;
            while (off < n)
            {
              ssize_t w = write(session_fd, buf + off, (size_t)(n - off));
              if (w > 0)
                off += w;
              else if (w < 0 && (errno == EINTR))
                continue;
              else if (w < 0 && (errno == EAGAIN || errno == EWOULDBLOCK))
                break;
              else
                break;
            }
          }
        }
      }

      if (WIFEXITED(status))
      {
        logit("@xxspawnvp: pid=%ld exited status=%d cmd='%s'", (long)pid, WEXITSTATUS(status), Cfile ? Cfile : argv[0]);
        if (master_fd >= 0)
          close(master_fd);
        signal(SIGCHLD, old_sigchld);
        return WEXITSTATUS(status);
      }

      if (WIFSIGNALED(status))
      {
        fprintf(stderr, "%s: Child (%s) exited due to signal %i!\n", __FUNCTION__, Cfile ? Cfile : argv[0], WTERMSIG(status));
        logit("@xxspawnvp: pid=%ld signaled sig=%d cmd='%s'", (long)pid, WTERMSIG(status), Cfile ? Cfile : argv[0]);
      }

      if (master_fd >= 0)
        close(master_fd);

      signal(SIGCHLD, old_sigchld);
      return -1;
    }

    signal(SIGCHLD, old_sigchld);
    return 0;
  }

  else /* child */
  {
    if (mode == P_NOWAITO)
    {
	/* Parent will not reap -- use double fork trick to avoid zombies */

	pid = getpid();
	signal(SIGCHLD, SIG_IGN);
	(void)setpgid(pid, pid); 
	if (fork())
        _exit(0);
    }

    if (is_door32)
    {
      /* Door32: preserve session_fd, door will use it directly via door32.sys */
      /* Do NOT redirect stdio, do NOT close session_fd */
      /* The door reads the fd number from door32.sys and uses it for I/O */
      logit("@xxspawnvp: Door32 mode - preserving session_fd=%d", session_fd);
    }
    else if (slave_fd >= 0)
    {
      (void)setsid();
      (void)ioctl(slave_fd, TIOCSCTTY, 0);

      dup2(slave_fd, 0);
      dup2(slave_fd, 1);
      dup2(slave_fd, 2);

      if (slave_fd > 2)
        close(slave_fd);

      if (master_fd >= 0)
        close(master_fd);

      if (session_fd > 2)
        close(session_fd);
    }
    else if (session_fd >= 0)
    {
      dup2(session_fd, 0);
      dup2(session_fd, 1);
      dup2(session_fd, 2);
      if (session_fd > 2)
        close(session_fd);
    }

do_exec:

    memset(cmd, 0, sizeof(cmd));

    for (i = 0; argv[i]; i++)
    {
      if (strlen(cmd) + strlen(argv[i]) + 2 >= sizeof(cmd))
        break;

      strcat(cmd, argv[i]);
      if (argv[i + 1])
        strcat(cmd, " ");
    }

    logit("@xxspawnvp: exec sh -c '%s'", cmd);

    execl("/bin/sh", "sh", "-c", cmd, (char *)NULL);

    logit("!xxspawnvp: exec failed for '%s': %s", cmd, strerror(errno));
    _exit(127);
  }
}




