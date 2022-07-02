/* common.h: common macros shared by server and client.

   Copyright (C) 2016 Alick Zhao (alick9188@gmail.com)

   This file is part of the sbcp project.

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
#define MAXDATASIZE 1000
#define MAX_USERNAME_LEN 16
#define MAX_REASON_LEN 32
#define MAX_MESSAGE_LEN 512
#define CLIENT_COUNT_LEN 2

#define NAK_MAX_CLIENTS "Chat room is already full!"
#define NAK_USERNAME "Username already used!"

#ifndef DEBUG
#define DEBUG 0
#endif

#define debug(...) \
    do { if (DEBUG) fprintf(stderr, __VA_ARGS__); } while (0)


typedef unsigned int sbcp_hdr;

// Should be within 16 bits.
typedef enum sbcp_hdr_type {
    JOIN = 2,
    SEND = 4,
    FWD  = 3,
    ACK  = 7,
    NAK  = 5,
    IDLE = 9,
    ONLINE  = 8,
    OFFLINE = 6,
} hdr_type;

typedef unsigned int sbcp_attr;

// Should be within 16 bits.
typedef enum sbcp_attr_type {
    Username = 2,
    Message  = 4,
    Reason   = 1,
    ClientCount = 3,
} attr_type;

// The client object used by the server.
typedef struct {
    int sockfd;
    char username[MAX_USERNAME_LEN + 1];
} client;

#ifdef __cplusplus
extern "C" { 	/* Assume C declarations for C++   */
#endif  /* __cplusplus */

int sendall(int s, char *buf, int *len);
void *get_in_addr(struct sockaddr *sa);
unsigned short get_in_port(struct sockaddr *sa);
unsigned short strtou16(const char*str);

sbcp_hdr new_hdr(hdr_type type);
void set_msg_len(sbcp_hdr *hdr, unsigned length);
unsigned get_msg_len(sbcp_hdr *hdr);
hdr_type get_hdr_type(sbcp_hdr *hdr);
unsigned get_protocol_version(sbcp_hdr *hdr);
sbcp_attr new_attr(attr_type type);
void set_attr_len(sbcp_attr *attr, unsigned length);
unsigned get_attr_len(sbcp_attr *attr);
attr_type get_attr_type(sbcp_attr *attr);
void print_hex(const char *s, unsigned n);

#ifdef __cplusplus
}		/* End of extern "C" { */
#endif  /* __cplusplus */

#endif /* COMMON_H */
// vim: set et sw=4 ai tw=80:
