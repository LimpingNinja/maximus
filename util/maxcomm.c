/*
 * maxcomm - Telnet to UNIX socket bridge for Maximus BBS
 *
 * Modifications Copyright (C) 2025 Kevin Morgan (Limping Ninja)
 * https://github.com/LimpingNinja
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version 2
 * of the License, or (at your option) any later version.
 */

#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include <string.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <sys/stat.h>
#include <sys/ioctl.h>
#include <dirent.h>
#include <sys/un.h>
#include <unistd.h>
#include <sys/types.h>
#include <fcntl.h>
#include <termios.h>

#include "../comdll/telnet.h"

static int transmit_binary = 0;
static int telnet_mode = 0;     /* Client supports telnet negotiation */
static int ansi_mode = 0;       /* Client supports ANSI escape codes */

int to_read (int fd, char* buf, int count, int timeout)
{
    fd_set rfds;
    struct timeval tv;
    int i;
    
    tv.tv_sec = 0;
    tv.tv_usec = 500;
   
    do
    {
    /* Note: on some unices, tv will be decremented to report the 
     * amount slept; on others it will not. That means that some 
     * unices will wait longer than others in the case that a 
     * signal has been received. 
     */
       i = select(fd + 1, &rfds, NULL, NULL, &tv);
    } while(i < 0 && (errno == EINTR));
  
    if (i > 0)
      return read(fd, buf, count);

    if (i == 0)
      return 0;
        
    return -1;
	 
}

int telnetInterpret(unsigned char* buf, int bytesRead)
{
  /* output buffer and counter */
  int oi; 
  /* arguments */
  unsigned char arg, arg2;
  
  unsigned char obuf[bytesRead];
  /* counter */
  int i, j, found, start=0;
  
  memset(obuf, '\0', bytesRead);
  oi = 0;

  for(i=start; i < bytesRead; i++)
  {

    if(buf[i] == cmd_IAC)
    {
	/* if the argument is avaible */
        if((i+1) < bytesRead)
        {
	    i++;
	    arg = buf[i];
	}
	/* if it's not IAC xxx */
	else
	{
	    /* we read one */
	    if(to_read(0, &arg, 1, 200) != 1)
	    {
	        sleep(1);
		/* if we didn't get anything we tries agian, 
		   and hope it goes. */
	        if(to_read(0, &arg, 1, 200) != 1)
	        {
		    return oi;
		}
	    }
	}

	/* find out which argument it's */    
	switch(arg)
	{
	    /* a real IAC */
	    case cmd_IAC:
	        obuf[oi] = cmd_IAC;
	        oi++;
		break;
	    /* a erase charecter */	
	    case cmd_EC:
	        obuf[oi] = 0x08;
	        oi++;
	        break;    
	    /* IAC WILL/WONT/DO/DONT <some option> */
	    case cmd_WILL:				
	    case cmd_WONT: 
    	    case cmd_DO:
    	    case cmd_DONT:
		/* if the second argument is aviable */
		if((i+1) < bytesRead )	    
		{
		    i++;
		    arg2 = buf[i];
		}
		/* else we read one */
		else
		{
		    if(to_read(0, &arg2, 1, 200) != 1)
		    {
		        sleep(1);
		        if(to_read(0, &arg2, 1, 200) != 1)
		        {
		    	    return oi;
			}
		    }
		}
		if(arg == cmd_WILL && arg2 == mopt_TRANSMIT_BINARY)
		    transmit_binary = 1;
		else if(arg == cmd_WONT && arg2 == mopt_TRANSMIT_BINARY)
		    transmit_binary = 0;

		break;
	    /* no argument just continue the loop */
	    case cmd_SE: 
	    case cmd_NOP:
	    case cmd_DM:
	    case cmd_BRK:
	    case cmd_IP:
	    case cmd_AO:
	    case cmd_AYT:
	    case cmd_GA:
	    case cmd_EL:
	        break;
		
	    case cmd_SB:
	        found = 0;
		    
		for(j=i; j < bytesRead; j++)
		{
		    if(buf[j] == cmd_SE)
		    {
		        found = 1;
		        break;
		    }
		}	        

		if(found)
		{
		    i = j;
		}
		break;
	    default:
		obuf[oi] = arg;
		oi++;
		break;
	}
	
    }    
    else
    {
        obuf[oi] = buf[i];
        oi++;
    }
  }
  
  memcpy(buf, obuf, oi);
  
  
  return oi;
}

int writeWIAC(int fd, unsigned char* buf, int count)
{
    int i=0;
    static char iac = 255;
    char* tp;
    
    for(i=0; i < count; i++)
    {
        write(fd, &(buf[i]), 1);
	
	if(buf[i] == cmd_IAC)
	    write(fd, &iac, 1);
    }

    return 0;    
}
						    
    



/*
 * detectAndNegotiate - Auto-detect client capabilities and negotiate accordingly
 *
 * Sends two probes:
 *   1. ESC[6n (ANSI Device Status Report - cursor position query)
 *   2. IAC DO SGA (Telnet Suppress Go Ahead request)
 *
 * Waits 200ms for responses:
 *   - IAC response -> telnet client, full negotiation
 *   - ESC[...R response -> ANSI terminal, skip telnet
 *   - Neither -> dumb terminal, raw mode
 *
 * All probe responses are consumed and discarded to prevent them
 * from leaking to Maximus.
 */
void detectAndNegotiate(int preferBinarySession)
{
  unsigned char probe[8];
  unsigned char buf[256];
  int buflen = 0;
  fd_set rfds;
  struct timeval tv;
  int n, i;
  int got_iac = 0;
  int got_ansi = 0;
  
  /* Print detection message */
  write(1, "\r\nDetecting terminal...", 24);
  
  /* Send Telnet probe first: IAC DO SGA */
  probe[0] = cmd_IAC;
  probe[1] = cmd_DO;
  probe[2] = opt_SGA;
  write(1, probe, 3);
  
  /* Wait briefly for telnet response */
  tv.tv_sec = 0;
  tv.tv_usec = 150000;  /* 150ms */
  
  FD_ZERO(&rfds);
  FD_SET(0, &rfds);
  
  while (select(1, &rfds, NULL, NULL, &tv) > 0)
  {
    n = read(0, buf + buflen, sizeof(buf) - buflen - 1);
    if (n <= 0) break;
    buflen += n;
    
    /* Reset for next select iteration - wait for more data */
    FD_ZERO(&rfds);
    FD_SET(0, &rfds);
    tv.tv_sec = 0;
    tv.tv_usec = 50000;  /* 50ms more for additional chunks */
  }
  
  /* Check if we got telnet response */
  for (i = 0; i < buflen; i++)
  {
    if (buf[i] == cmd_IAC)
    {
      got_iac = 1;
      break;
    }
  }
  
  /* If telnet detected, assume ANSI (telnet clients almost always support ANSI) */
  if (got_iac)
  {
    got_ansi = 1;  /* Assume ANSI for telnet clients */
  }
  else
  {
    /* No telnet - send ANSI probe to check for ANSI support */
    probe[0] = 0x1B;  /* ESC */
    probe[1] = '[';
    probe[2] = '6';
    probe[3] = 'n';
    write(1, probe, 4);
    
    /* Wait for ANSI response */
    buflen = 0;
    tv.tv_sec = 0;
    tv.tv_usec = 200000;  /* 200ms */
    FD_ZERO(&rfds);
    FD_SET(0, &rfds);
    
    while (select(1, &rfds, NULL, NULL, &tv) > 0)
    {
      n = read(0, buf + buflen, sizeof(buf) - buflen - 1);
      if (n <= 0) break;
      buflen += n;
      FD_ZERO(&rfds);
      FD_SET(0, &rfds);
      tv.tv_sec = 0;
      tv.tv_usec = 50000;
    }
    
    /* Check for ANSI response (ESC[...R) */
    for (i = 0; i < buflen; i++)
    {
      if (buf[i] == 0x1B && i + 1 < buflen && buf[i+1] == '[')
      {
        got_ansi = 1;
        break;
      }
    }
  }
  
  /* Set global mode flags */
  telnet_mode = got_iac;
  ansi_mode = got_ansi;
  
  /* Clear any garbage on the line, then report detection results */
  /* ESC[2K = clear entire line, \r = return to start of line */
  write(1, "\x1B[2K\rDetecting terminal...", 26);
  
  if (got_iac && got_ansi)
    write(1, " Telnet+ANSI\r\n", 14);
  else if (got_iac)
    write(1, " Telnet\r\n", 9);
  else if (got_ansi)
    write(1, " ANSI\r\n", 7);
  else
    write(1, " Raw\r\n", 6);
  
  /* Drain any remaining input that might have arrived late */
  tv.tv_sec = 0;
  tv.tv_usec = 50000;  /* 50ms */
  FD_ZERO(&rfds);
  FD_SET(0, &rfds);
  while (select(1, &rfds, NULL, NULL, &tv) > 0)
  {
    n = read(0, buf, sizeof(buf));
    if (n <= 0) break;
    FD_ZERO(&rfds);
    FD_SET(0, &rfds);
    tv.tv_sec = 0;
    tv.tv_usec = 50000;
  }
  
  /* Only send full telnet negotiation if client responded to IAC */
  if (telnet_mode)
  {
    unsigned char command[4];
    
    command[0] = cmd_IAC; command[1] = cmd_DONT; command[2] = opt_ENVIRON;
    write(1, command, 3);

    command[0] = cmd_IAC; command[1] = cmd_WILL; command[2] = opt_ECHO;
    write(1, command, 3);

    command[0] = cmd_IAC; command[1] = cmd_WILL; command[2] = opt_SGA;
    write(1, command, 3);

    command[0] = cmd_IAC; command[1] = cmd_DONT; command[2] = opt_NAWS;
    write(1, command, 3);

    if (preferBinarySession)
    {
      command[0] = cmd_IAC; command[1] = cmd_DO; command[2] = opt_TRANSMIT_BINARY;
      write(1, command, 3);

      command[0] = cmd_IAC; command[1] = cmd_WILL; command[2] = opt_TRANSMIT_BINARY;
      write(1, command, 3);
    }
  }
}

int fexist(char* filename)
{
    struct stat s;

    if(stat(filename, &s))
        return 0;
    else
        return 1;
}

int main(void)
{
    int s, t, nt, len, retval;
    struct sockaddr_un remote;
    struct timeval tv;
    fd_set rfds, wfds;
    char tmptext[128];
    char lockpath[128];
    unsigned char str[512];
    struct dirent * dirp = NULL;
    DIR* dir = NULL;


    if ((s = socket(AF_UNIX, SOCK_STREAM, 0)) == -1) 
    {
        perror("socket");
        exit(1);
    }

    if (getcwd(tmptext, 128) == NULL)
    {
        perror("getcwd");
        exit(1);
    }
    
    dir = opendir(tmptext);
    if (dir == NULL)
    {
        perror("opendir");
        exit(1);
    }

    remote.sun_path[0] = '\0';  /* Initialize to empty */
    
    while((dirp = readdir(dir)))
    {
        if(strstr(dirp->d_name, "maxipc") 
           && !strstr(dirp->d_name, ".lck"))
        {
    	    sprintf(lockpath, "%s/%s.lck", tmptext, dirp->d_name);
	    if(fexist(lockpath))
	        continue;        
	    
	    /* Use absolute path for socket */
	    snprintf(remote.sun_path, sizeof(remote.sun_path), "%s/%s", tmptext, dirp->d_name);
    	    remote.sun_family = AF_UNIX;
	    break;
	}
    }
    closedir(dir);
    
    if (remote.sun_path[0] == '\0')
    {
        fprintf(stderr, "No available Maximus nodes found in %s\n", tmptext);
        exit(1);
    }

    len = sizeof(remote);
    if (connect(s, (struct sockaddr *)&remote, len) == -1) 
    {
	perror("Sorry no more free nodes!");
	exit(1);
    }
    
    detectAndNegotiate(1);    

    for(;;)
    {
	tv.tv_usec = 5;
	tv.tv_sec = 0;

	FD_ZERO(&wfds);
	FD_SET(s, &wfds);

	if(select(s+1, &wfds, 0, 0, &tv) > 0)
	{
    	    if ((t=read(s, str, 512)) > 0) 
	    {
		writeWIAC(1, str, t);
    	    } 
	    else 
	    {
                if (t < 0) perror("recv");
                else fprintf(stdout, "Server closed connection\n");
	        exit(1);
    	    }
	}
	    
	FD_ZERO(&rfds);
	FD_SET(0, &rfds);

	tv.tv_usec = 5;
	tv.tv_sec = 0;
	    
	if((retval = select(0 + 1, &rfds, 0, 0, &tv)) > 0)
	{
	    if((t = read(0, str, 512))) 
	    {
		t = telnetInterpret(str, t);
    		write(s, str, t);
	    }
	    else
	    {
	        exit(1);		
	    }
	}
    }

    close(s);

    return 0;
}
