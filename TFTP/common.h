/* common.h: helper macros.

   Copyright (C) 2016 Tao Zhao (alick@tamu.edu)

   This file is part of the tftp-server project.

   This program is free software; you can redistribute it and/or modify
   it under the terms of the GNU General Public License as published by
   the Free Software Foundation; either version 3, or (at your option)
   any later version.

   This program is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
   GNU General Public License for more details.

   You should have received a copy of the GNU General Public License
   along with this program.  If not, see <http://www.gnu.org/licenses/>.  */

#ifndef COMMON_H
#define COMMON_H

#include <stdio.h>       // for fprintf()
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netdb.h>
#include <arpa/inet.h>

// Errors related to basic socket functions.
#define ERR_SOCKET 1
#define ERR_CONN 2
#define ERR_RECV 3
#define ERR_SELECT 4
#define ERR_SOCKOPT 5

#define ERR_SERVER 10	// server hung up

#define ERR_INVOC 100
#define ERR_INVALID_PORT 101
#define ERR_USERNAME 102
#define ERR_REJECTED 103

// Lengths in bytes.
#define MAX_FILE_SIZE (32 * 1024 * 1024) // Max: 32 MB
#define MAXDATASIZE 1000
#define CLIENT_COUNT_LEN 2
#define OPCODE_LEN 2
#define BLOCK_NUM_LEN 2
#define BLOCK_LEN 512
#define ERROR_NUM_LEN 2

// Error messages.
#define FILE_TOO_LARGE "File too large!"

#ifndef DEBUG
#define DEBUG 1
#endif

#define debug(...) \
    do { if (DEBUG) fprintf(stderr, __VA_ARGS__); } while (0)


// Should be within 16 bits.
typedef enum tftp_opcode_type {
    RRQ   = 1,
    WRQ   = 2,
    DATA  = 3,
    ACK   = 4,
    ERROR = 5,
} opcode_type;

#ifdef __cplusplus
extern "C" { 	/* Assume C declarations for C++   */
#endif  /* __cplusplus */

int sendall(int s, char *buf, int *len);
void *get_in_addr(struct sockaddr *sa);
unsigned short get_in_port(struct sockaddr *sa);
unsigned short strtou16(const char*str);

void print_hex(const char *s, unsigned n);

#ifdef __cplusplus
}		/* End of extern "C" { */
#endif  /* __cplusplus */

#endif /* COMMON_H */
// vim: set et sw=4 ai tw=80:
