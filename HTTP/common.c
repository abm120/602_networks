/* common.c -- common routines shared by proxy and client.

   Copyright (C) 2016 Tao Zhao (alick@tamu.edu)

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
#include <string.h>
#include <strings.h>
#include <unistd.h>
#include <limits.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netdb.h>
#include <arpa/inet.h>
#include <errno.h>		// defines perror(), herror()

#include "common.h"

// cf. Beej's Guide.
int writeall(int s, char *buf, int len)
{
    int total = 0;        // how many bytes we've sent
    int bytesleft = len; // how many we have left to send
    int n;

    while (total < len) {
        n = write(s, buf + total, bytesleft);
        if (n == -1) {
            perror("write");
            exit(ERR_WRITE);
        }
        total += n;
        bytesleft -= n;
    }

    return total;
}

int readall(int s, char *buf, int len)
{
    int total = 0;        // how many bytes we've received
    int bytesleft = len;
    int n;

    while (total < len) {
        n = read(s, buf + total, bytesleft);
        debug("read: got %d bytes\n", n);
        if (n == -1) {
            perror("read");
            exit(ERR_READ);
        } else if (n == 0) {
            break;
        } else {
            total += n;
            bytesleft -= n;
        }
    }

    return total;
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

// Test if two strings are equal ingoring the case.
bool streqi(const char *s1, const char *s2)
{
    bool res = ((strlen(s1) == strlen(s2)) && (strcasecmp(s1, s2) == 0));
    if (res) {
        debug("%s == %s\n", s1, s2);
    } else {
        debug("%s != %s\n", s1, s2);
    }
    return res;
}
