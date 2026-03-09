/*
 * telnet.h — Telnet protocol definitions for utilities
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

#ifndef TELNET_H
#define TELNET_H

/* Telnet commands */
#define cmd_SE      240  /* End of subnegotiation parameters */
#define cmd_NOP     241  /* No operation */
#define cmd_DM      242  /* Data mark */
#define cmd_BRK     243  /* Break */
#define cmd_IP      244  /* Interrupt process */
#define cmd_AO      245  /* Abort output */
#define cmd_AYT     246  /* Are you there */
#define cmd_EC      247  /* Erase character */
#define cmd_EL      248  /* Erase line */
#define cmd_GA      249  /* Go ahead */
#define cmd_SB      250  /* Subnegotiation begin */
#define cmd_WILL    251  /* Will */
#define cmd_WONT    252  /* Won't */
#define cmd_DO      253  /* Do */
#define cmd_DONT    254  /* Don't */
#define cmd_IAC     255  /* Interpret as command */

/* Telnet options */
#define opt_ECHO              1   /* Echo */
#define opt_SGA               3   /* Suppress go ahead */
#define opt_TRANSMIT_BINARY   0   /* Binary transmission */
#define opt_NAWS             31   /* Negotiate about window size */
#define opt_ENVIRON          36   /* Environment variables */

/* Maximus-specific option aliases */
#define mopt_TRANSMIT_BINARY  opt_TRANSMIT_BINARY

#endif /* TELNET_H */
