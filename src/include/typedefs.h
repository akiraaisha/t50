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

#ifndef __TYPEDEFS_INCLUDED__
#define __TYPEDEFS_INCLUDED__

struct config_options;

/* Data types */
typedef uint32_t in_addr_t;
typedef int socket_t;
typedef int threshold_t;  /* FIX: If we need more than 2147483648 packets sent,
                                  this type can be changed to int64_t. */

typedef void (*module_func_ptr_t)(const struct config_options * const __restrict__, size_t *);

/* This will ease the buffers pointers manipulations. */
typedef union {
  void    *ptr;
  uint8_t *byte_ptr;
  uint16_t *word_ptr;
  uint32_t *dword_ptr;
  in_addr_t *inaddr_ptr;
  uint64_t *qword_ptr;
} mptr_t;

/* Common protocol structures used by code */
/*
 * User Datagram Protocol (RFC 768)
 *
 * Checksum is the 16-bit one's complement of the one's complement sum of a
 * pseudo header of information from the IP header, the UDP header, and the
 * data,  padded  with zero octets  at the end (if  necessary)  to  make  a
 * multiple of two octets.
 *
 * The pseudo  header  conceptually prefixed to the UDP header contains the
 * source  address,  the destination  address,  the protocol,  and the  UDP
 * length.   This information gives protection against misrouted datagrams.
 * This checksum procedure is the same as is used in TCP.
 *
 *                   0      7 8     15 16    23 24    31
 *                  +--------+--------+--------+--------+
 *                  |          source address           |
 *                  +--------+--------+--------+--------+
 *                  |        destination address        |
 *                  +--------+--------+--------+--------+
 *                  |  zero  |protocol|   UDP length    |
 *                  +--------+--------+--------+--------+
 *
 * If the computed  checksum  is zero,  it is transmitted  as all ones (the
 * equivalent  in one's complement  arithmetic).   An all zero  transmitted
 * checksum  value means that the transmitter  generated  no checksum  (for
 * debugging or for higher level protocols that don't care).
 */
struct psdhdr
{
  in_addr_t saddr;                  /* source address              */
  in_addr_t daddr;                  /* destination address         */
  uint8_t   zero;                   /* must be zero                */
  uint8_t   protocol;               /* protocol                    */
  uint16_t  len;                    /* header length               */
};

#endif
