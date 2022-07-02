/* client.c -- The simple HTTP proxy client application.

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
#include <errno.h>		// defines perror(), herror()
#include <fcntl.h>		// set socket to non-blocking with fcntrl()
#include <unistd.h>
#include <string.h>
#include <strings.h>
#include <assert.h>		//add diagnostics to a program

#include <netinet/in.h>		//defines in_addr and sockaddr_in structures
#include <arpa/inet.h>		//external definitions for inet functions
#include <netdb.h>		//getaddrinfo() & gethostbyname()

#include <sys/socket.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <sys/uio.h>
#include <sys/wait.h>
#include <sys/select.h>		// for select() system call only
#include <sys/time.h>		// time() & clock_gettime()

#include "common.h"

int main(int argc, char *argv[])
{
    fd_set read_fds;            // read file descriptor list for select()
    fd_set read_fds_tmp;        // temp read file descriptor list for select()
    int i;
    int fd_max;                 // maximum file descriptor number
    struct sockaddr_in saddr;
    int sockfd;
    int f;      // file to store HTML page
    unsigned short proxy_port;
    const char *proxy_ip;
    const char *url;
    char host[MAX_HOST_LEN + 1]; // might include trailing port number
    char rel_url[MAX_REL_URL_LEN + 1];
    char *p;
    char tx_buf[MAX_HTTP_REQUEST_LEN + 1];
    char rx_buf[MAX_HTTP_RESPONSE_LEN + 1];
    int numbytes = 0;
    int len = 0;
    int offset = 0;
    char *filename = "response";

    // Initialize file descriptor sets.
    FD_ZERO(&read_fds);
    FD_ZERO(&read_fds_tmp);

    memset(tx_buf, '\0', sizeof(tx_buf));
    memset(rx_buf, '\0', sizeof(rx_buf));

    // Handle command line arguments.
    if (argc != 4) {
        fprintf(stderr, "Invalid command invocation!\n");
        fprintf(stderr, "Usage: ./client proxy_ip proxy_port url\n");
        exit(ERR_INVOC);
    }
    proxy_ip = argv[1];
    proxy_port = strtou16(argv[2]);
    url = argv[3];
    len = strlen(url);
    if (len > MAX_URL_LEN) {
        fprintf(stderr, "URL too long!\n");
        exit(ERR_URL);
    }
    debug("url: %s\n", url);
    // Skip beginning protocol prefix.
    if (0 == strncasecmp(url, HTTP_PROTO_STR, HTTP_PROTO_STR_LEN)) {
        offset = HTTP_PROTO_STR_LEN;
    } else if (NULL != strstr(url, "://")) {
        fprintf(stderr, "Unsupported protocol!\n");
        exit(ERR_PROTO);
    } else {
        offset = 0;
    }
    debug("url w/o protocol prefix: %s\n", url + offset);
    p = strchr(url + offset, '/');
    if (p) {
        len = p - url - offset; // length of host in url
        if (len > MAX_HOST_LEN) {
            fprintf(stderr, "Host length too long!\n");
            exit(ERR_INVOC);
        }
        strlcpy(host, url + offset, len + 1);
        len = strlcpy(rel_url, p, sizeof(rel_url)); // length of rel_url in url
        if (len > MAX_REL_URL_LEN) {
            fprintf(stderr, "Relative URL too long!\n");
            exit(ERR_INVOC);
        }
    } else { // Only host name is specified.
        len = strlcpy(host, url + offset, sizeof(host)); // length of host in url
        if (len > MAX_HOST_LEN) {
            fprintf(stderr, "Host length too long!\n");
            exit(ERR_INVOC);
        }
        rel_url[0] = '/';
        rel_url[1] = '\0';
    }
    debug("Host: %s\n", host);
    debug("Relative URL: %s\n", rel_url);

    if ((sockfd = socket(AF_INET, SOCK_STREAM, 0)) < 0) {
        perror("socket");
        exit(ERR_SOCKET);
    }
    memset(&saddr, '\0', sizeof(saddr));
    saddr.sin_family = AF_INET;
    inet_pton(AF_INET, proxy_ip, &(saddr.sin_addr));
    saddr.sin_port = htons(proxy_port);

    if (connect(sockfd, (struct sockaddr *)&saddr, sizeof(saddr)) < 0) {
        close(sockfd);
        perror("connect");
        exit(ERR_CONN);
    }
    debug("Client connected!\n");
    // Add the server socket fd to the read_fds set.
    FD_SET(sockfd, &read_fds);
    fd_max = sockfd;

    len = snprintf(tx_buf, sizeof(tx_buf), "GET %s HTTP/1.0\r\nHost: %s\r\nUser-Agent: HTMLGET/1.0\r\n\r\n", rel_url, host);
    if (len >= sizeof(tx_buf)) {
        debug("tx_buf too small: need %d (w/o NULL byte) \n", len);
        exit(ERR_BUG);
    }

    numbytes = writeall(sockfd, tx_buf, strlen(tx_buf));
    debug("%d bytes '%s' sent!\n", numbytes, tx_buf);

    read_fds_tmp = read_fds;
    if (select(fd_max + 1, &read_fds_tmp, NULL, NULL, NULL) == -1) {
        perror("select");
        exit(ERR_SELECT);
    }

    if (FD_ISSET(sockfd, &read_fds_tmp)) {
        debug("ready to read\n");
        numbytes = recv(sockfd, rx_buf, MAX_HTTP_RESPONSE_LEN, MSG_WAITALL);
        rx_buf[numbytes] = '\0';
        debug("Client received %d bytes.\n", numbytes);

        if ((f = creat(filename, S_IRUSR | S_IWUSR)) < 0) {
            perror("creat");
            exit(ERR_CREAT);
        }
        numbytes = writeall(f, rx_buf, numbytes);
        debug("%d bytes written to file '%s'!\n", numbytes, filename);
    }

    close(sockfd);
    return 0;
}
// vim: set et sw=4 ai tw=80:
