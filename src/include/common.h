/*
 *  T50 - Experimental Mixed Packet Injector
 *
 *  Copyright (C) 2010 - 2014 - T50 developers
 *
 *  This program is free software: you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation, either version 2 of the License, or
 *  (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program.  If not, see <http://www.gnu.org/licenses/>.
*/

#ifndef __COMMON_H
#define __COMMON_H

#if !(linux) || !(__linux__)
# error "Sorry! The T50 was only tested under Linux!"
#endif  /* __linux__ */

#define _GNU_SOURCE

#include <assert.h> /* for debugging purposes only */
#include <time.h>
#include <stdio.h>
#include <stdint.h>
#include <netdb.h>
#include <errno.h>
#include <fcntl.h>
#include <unistd.h>
#include <stdlib.h>
#include <string.h>
#include <signal.h>
#include <getopt.h>
#include <sys/time.h>
#include <sys/socket.h>
#include <sys/resource.h>
#include <netinet/in.h>
#include <arpa/inet.h>

/* This code prefers to use Linux headers rather than BSD favored */
#include <linux/ip.h>
#include <linux/tcp.h>
#include <linux/udp.h>
#include <linux/icmp.h>
#include <linux/igmp.h>
#include <linux/dccp.h>
#include <linux/if_ether.h>

#include <debug.h>

#include <typedefs.h>
#include <defines.h>

#include <config.h>
#include <help.h>
#include <modules.h>

/* NOTE: Protocols and modules definitions are on modules.h now. */

/* The packet buffer. Reallocated as needed! */
extern void *packet;
extern size_t current_packet_size; /* available if necessary! updated by alloc_packet(). */

/* NOTE: Since this is not a macro, it's here insted of defines.h. */
extern uint32_t NETMASK_RND(uint32_t);

/* Realloc packet as needed. Used on module functions. */
extern void alloc_packet(size_t);

/* Common routines used by code */
extern struct cidr *config_cidr(uint32_t, in_addr_t);
extern uint16_t cksum(void *, size_t);  /* Checksum calc. */
extern in_addr_t resolv(char *);  /* Resolve name to ip address. */
extern int createSocket(void); /* Creates the sending socket */
extern void closeSocket(void);  /* Close the previously created socket */
/* Send the actual packet from buffer, with size bytes, using config options. */
extern int sendPacket(const void * const, size_t, const struct config_options * const __restrict__);
extern void show_version(void); /* Prints version info. */
extern void usage(void);        /* Prints usage message */

#ifdef __HAVE_RDRAND__
extern uint32_t readrand(void);
#endif

#endif /* __COMMON_H */
