/* common.c -- helper routines.

   Copyright (C) 2016 Tao Zhao (alick@tamu.edu)

   This file is part of the tftp-server project.

   This program is free software: you can redistribute it and/or modify
   it under the terms of the GNU General Public License as published by
   the Free Software Foundation, either version 3 of the License, or
   (at your option) any later version.

   This program is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
   GNU General Public License for more details.

   You should have received a copy of the GNU General Public License
   along with this program.  If not, see <http://www.gnu.org/licenses/>.  */

#include <stdio.h>
#include <stdlib.h>
#include <limits.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netdb.h>
#include <arpa/inet.h>
#include <errno.h>		// defines perror(), herror()

#include "common.h"

// From Beej's Guide.
int sendall(int s, char *buf, int *len)
{
    int total = 0;        // how many bytes we've sent
    int bytesleft = *len; // how many we have left to send
    int n;

    while(total < *len) {
        n = send(s, buf+total, bytesleft, 0);
        if (n == -1) { break; }
        total += n;
        bytesleft -= n;
    }

    *len = total; // return number actually sent here

    return n==-1?-1:0; // return -1 on failure, 0 on success
}


// get sockaddr, IPv4 or IPv6:
void *get_in_addr(struct sockaddr *sa)
{
    if (sa->sa_family == AF_INET) {
        return &(((struct sockaddr_in *) sa)->sin_addr);
    }
    return &(((struct sockaddr_in6 *) sa)->sin6_addr);
}

// get port number, IPv4 or IPv6:
unsigned short get_in_port(struct sockaddr *sa)
{
    if (sa->sa_family == AF_INET) {
        return ((struct sockaddr_in *) sa)->sin_port;
    }
    return ((struct sockaddr_in6 *) sa)->sin6_port;
}

// Parse unsigned short (16 bit) integer from a string.
unsigned short strtou16(const char*str)
{
    char *tmp;
    unsigned long val;
    unsigned short res;

    errno = 0;

    val = strtol(str, &tmp, 10);

    if (tmp == str || *tmp != '\0' ||
        (val == ULONG_MAX && errno == ERANGE) ||
        val > USHRT_MAX) {
        fprintf(stderr, "Invalid port number!\n");
        exit(ERR_INVALID_PORT);
    }

    res = (unsigned short)val;
    return res;
}

void print_hex(const char *s, unsigned n)
{
    int i;
    for (i = 0; i < n; i++) {
        debug("%02x", (unsigned int) *s++);
    }
    debug("\n");
}
// vim: set et sw=4 ai tw=80:
