/* common.h: common macros and declarations shared by proxy and client.

   Copyright (C) 2016 Tao Zhao (alick@tamu.edu)

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

#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <sys/socket.h>

// Errors related to basic socket functions.
#define ERR_SOCKET 1
#define ERR_CONN 2
#define ERR_READ 3
#define ERR_SELECT 4
#define ERR_SOCKOPT 5
#define ERR_WRITE 6
#define ERR_CREAT 7
#define ERR_GETADDRINFO 8

#define ERR_SERVER 10	// server hung up

#define ERR_INVOC 100
#define ERR_INVALID_PORT 101
#define ERR_URL 102
#define ERR_PROTO 103

#define ERR_BUG 200	// bug in code

// Lenght limits in bytes.
#define MAX_HTTP_REQUEST_LEN 200
#define MAX_HTTP_RESPONSE_LEN (1024 * 1024)	// Should be larger than MAX_HTTP_REQUEST_LEN
#define MAX_URL_LEN 50
#define MAX_HOST_LEN 30	// might include trailing port number (`:port`)
#define MAX_REL_URL_LEN ((MAX_URL_LEN) - (MAX_HOST_LEN)) // Need to ensure at least 1

#define HTTP_PROTO_STR "http://"
#define HTTP_PROTO_STR_LEN 7

#ifndef DEBUG
#define DEBUG 1
#endif

#define debug(...) \
	do { if (DEBUG) fprintf(stderr, __VA_ARGS__); } while (0)

#ifdef __cplusplus
extern "C" { 	/* Assume C declarations for C++   */
#endif  /* __cplusplus */

int writeall(int s, char *buf, int len);
int readall(int s, char *buf, int len);
void *get_in_addr(struct sockaddr *sa);
unsigned short get_in_port(struct sockaddr *sa);
unsigned short strtou16(const char*str);
size_t strlcpy(char *dst, const char *src, size_t dsize);
bool streqi(const char *s1, const char *s2);

#ifdef __cplusplus
}		/* End of extern "C" { */
#endif  /* __cplusplus */

#endif /* COMMON_H */
