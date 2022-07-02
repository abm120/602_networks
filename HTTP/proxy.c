/* proxy.c -- The simple HTTP proxy.

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
#include <unistd.h>
#include <errno.h>
#include <string.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netdb.h>
#include <arpa/inet.h>
#include <sys/wait.h>
#include <signal.h>
#include <time.h>
#include <sys/time.h>

#include "common.h"
#include "list.h"
#include "cache.h"

#define BACKLOG 10              // how many pending connections queue will hold

int main(int argc, char *argv[])
{
    fd_set read_fds;            // read file descriptor list for select()
    fd_set read_fds_tmp;        // temp read file descriptor list for select()
    int fd_max;                 // maximum file descriptor number
    int i;                   // loop counter

    int sockfd, new_fd;         // listen on sock_fd, new connection on new_fd
    int servfd;         // for connecting remote server
    int clientfd;         // for reply to a connected server
    struct sockaddr_in proxy_addr;        // proxy's address information
    struct sockaddr_storage their_addr; // connector's address information
    socklen_t sin_size;
    const char *proxy_ip;
    unsigned short proxy_port;
    int yes = 1;
    char s[INET6_ADDRSTRLEN];
    char req_buf[MAX_HTTP_REQUEST_LEN + 1];
    char res_buf[MAX_HTTP_RESPONSE_LEN + 1];
    char host[MAX_HOST_LEN + 1];
    char rel_url[MAX_REL_URL_LEN + 1];
    unsigned n_clients = 0;
    int numbytes;
    int len;
    int numbytes_cum;         // cumulative numbytes for read (recv) in a loop
    unsigned short port;        // client port
    char *p;    // tmp storage for locating substr
    char *q;    // tmp storage for locating substr
    int status;
    struct addrinfo hints;
    struct addrinfo *servinfo;
    struct addrinfo *servp;
    client *clients = new_list();
    cache cache = new_cache();
    struct timeval tv;
    client *clientp;    // pointer to a client node
    entry *entryp;      // pointer to a cache entry
    bool cache_entry_good = false;

    // Initialize file descriptor sets.
    FD_ZERO(&read_fds);
    FD_ZERO(&read_fds_tmp);

    // Initialize the hints struct.
    memset(&hints, 0, sizeof hints);
    hints.ai_family = AF_INET;
    hints.ai_socktype = SOCK_STREAM;

    // Handle command line arguments.
    if (argc != 3) {
        fprintf(stderr, "Invalid command invocation!\n");
        fprintf(stderr, "Usage: ./proxy proxy_ip proxy_port\n");
        exit(ERR_INVOC);
    }
    proxy_ip = argv[1];
    proxy_port = strtou16(argv[2]);

    if ((sockfd = socket(AF_INET, SOCK_STREAM, 0)) == -1) {
        perror("proxy: socket");
        exit(ERR_SOCKET);
    }
    if (setsockopt(sockfd, SOL_SOCKET, SO_REUSEADDR, &yes,
                sizeof(int)) == -1) {
        perror("setsockopt");
        exit(ERR_SOCKOPT);
    }

    memset(&proxy_addr, '\0', sizeof(proxy_addr));
    proxy_addr.sin_family = AF_INET;
    inet_pton(AF_INET, proxy_ip, &(proxy_addr.sin_addr));
    proxy_addr.sin_port = htons(proxy_port);

    if (bind(sockfd, (struct sockaddr *)&proxy_addr, sizeof proxy_addr) == -1) {
        close(sockfd);
        perror("bind");
    }

    if (listen(sockfd, BACKLOG) == -1) {
        perror("listen");
        exit(1);
    }

    // Add the proxy socket fd to the read_fds set.
    FD_SET(sockfd, &read_fds);
    fd_max = sockfd;

    debug("proxy: waiting for connections...\n");
    while (1) {                 // main event loop
        // Use tmp since they will be modified by select().
        read_fds_tmp = read_fds;
        if (select(fd_max + 1, &read_fds_tmp, NULL, NULL, NULL) == -1) {
            perror("select");
            exit(ERR_SELECT);
        }

        for (i = 0; i <= fd_max; i++) {
            if (FD_ISSET(i, &read_fds_tmp)) {
                if (i == sockfd) { // new connection
                    sin_size = sizeof their_addr;
                    new_fd =
                        accept(sockfd, (struct sockaddr *) &their_addr, &sin_size);
                    if (new_fd == -1) {
                        perror("accept");
                    } else {
                        inet_ntop(their_addr.ss_family,
                                get_in_addr((struct sockaddr *) &their_addr),
                                s, sizeof s);
                        port = get_in_port((struct sockaddr *)&their_addr);
                        debug("proxy: got connection from %s:%d (fd=%d)\n", s, port, new_fd);
                        FD_SET(new_fd, &read_fds);
                        if (new_fd > fd_max) {
                            fd_max = new_fd;
                        }
                        //TODO: Mark new_fd as a client.
                        add_client(&clients, new_fd);
                        n_clients++;
                        /*numbytes = writeall(new_fd, "Hello, world!\n", 14);*/
                        /*debug("%d bytes hello msg sent!\n", numbytes);*/
                        /*close(new_fd);*/
                    }
                } else { // new client GET request or server response
                    if ((numbytes = read(i, req_buf, MAX_HTTP_REQUEST_LEN)) > 0) {
                        req_buf[numbytes] = '\0';
                        debug("proxy: got data '%s' from fd=%d\n", req_buf, i);
                        //TODO: decide i is a client or server.
                        if (find_client(clients, i)) {        // client
                            // Forward request to the remote server.
                            // Make sure this is a GET request.
                            if (0 != strncasecmp(req_buf, "GET ", 4)) {
                                fprintf(stderr, "Bad request: not GET!\n");
                                continue;
                            }
                            // First, parse out the requested page.
                            p = req_buf + 4;    // skip "GET ".
                            q = strstr(p, " ");
                            len = q - p;        // length of rel_url (page)
                            if (len > MAX_REL_URL_LEN) {
                                fprintf(stderr, "Bad request: rel_url too long!\n");
                                continue;
                            }
                            strlcpy(rel_url, p, len + 1);
                            // Parse out domain of remote server.
                            p = strstr(req_buf, "Host: ");
                            if (!p) {
                                fprintf(stderr, "Bad request: No Host header\n");
                                continue;
                            }
                            q = strstr(p, "\r\n");
                            if (!q) {
                                fprintf(stderr, "Bad request: Host value not terminated\n");
                                continue;
                            }
                            len = q - p - 6;
                            if (len > MAX_HOST_LEN) {
                                fprintf(stderr, "Bad request: Host too long!\n");
                                continue;
                            }
                            strlcpy(host, p + 6, len + 1);
                            debug("parsed host: %s\n", host);
                            debug("parsed page: %s\n", rel_url);
                            update_client(clients, i, -1, host, rel_url);

                            // Check if in cache and not expired.
                            entryp = find_entry(cache, host, rel_url);
                            if (entryp) {
                                //TODO: check if expired.
                                if (entryp->may_expire) {
                                    if (entryp->expiry_time < time(NULL)) {
                                        cache_entry_good = false;
                                        debug("Cache entry expired!\n");
                                        remove_entry(&cache, host, rel_url);
                                    } else {
                                        cache_entry_good = true;
                                    }
                                } else {
                                    cache_entry_good = true;
                                }
                            } else {
                                cache_entry_good = false;
                            }
                            if (cache_entry_good) {
                                // Use the cached response.
                                numbytes = writeall(i, entryp->response, strlen(entryp->response));
                                debug("CACHE HIT: %d bytes '%s' sent to client %d!\n", numbytes, entryp->response, i);
                                // Update the entry's access time.
                                gettimeofday(&tv, NULL);
                                update_entry(cache, host, rel_url, tv);
                                remove_client(&clients, i);
                                n_clients--;
                                FD_CLR(i, &read_fds);
                                close(i);
                                continue;
                            }
                            // Connect to remote server.
                            status = getaddrinfo(host, "http", &hints, &servinfo);
                            if (status != 0) {
                                fprintf(stderr, "getaddrinfo: %s\n", gai_strerror(status));
                                exit(ERR_GETADDRINFO);
                            }
                            // Connect to the first we can.
                            for (servp = servinfo; servp != NULL; servp = servp->ai_next) {
                                if ((servfd = socket(servp->ai_family, servp->ai_socktype, servp->ai_protocol)) < 0) {
                                    perror("socket");
                                    continue;
                                }
                                if (connect(servfd, servp->ai_addr, servp->ai_addrlen) < 0) {
                                    close(servfd);
                                    perror("connect");
                                    continue;
                                }
                                break;
                            }
                            if (!servp) {
                                debug("Connect to %s failed!\n", host);
                                continue;
                            }
                            debug("Connected to %s\n", host);
                            FD_SET(servfd, &read_fds);
                            if (servfd > fd_max) {
                                fd_max = servfd;
                            }
                            update_client(clients, i, servfd, NULL, NULL);
                            freeaddrinfo(servinfo);
                            // Send out the request.
                            numbytes = writeall(servfd, req_buf, numbytes);
                            debug("%d bytes '%s' sent to remote server %d!\n", numbytes, req_buf, servfd);
                        } else if ((clientfd = find_client_by_servfd(clients, i)) >= 0) { // server response.
                            debug("HTTP response from servfd=%d for clientfd=%d\n", i, clientfd);
                            // This is in fact response so copy to res_buf first.
                            memcpy(res_buf, req_buf, numbytes);
                            numbytes_cum = numbytes;
                            numbytes = recv(i, res_buf + numbytes_cum, MAX_HTTP_RESPONSE_LEN - numbytes_cum, MSG_WAITALL);
                            numbytes_cum += numbytes;
                            debug("numbytes=%d, numbytes_cum=%d\n", numbytes, numbytes_cum);
                            res_buf[numbytes_cum] = '\0';
                            numbytes = writeall(clientfd, res_buf, numbytes_cum);
                            debug("%d bytes '%s' sent back to clientfd=%d!\n", numbytes, res_buf, clientfd);
                            // Add info into our cache.
                            clientp = find_client(clients, clientfd);
                            gettimeofday(&tv, NULL);
                            add_entry(&cache, clientp->domain, clientp->page, res_buf, tv);

                            remove_client(&clients, clientfd);
                            n_clients--;
                            FD_CLR(clientfd, &read_fds);
                            FD_CLR(i, &read_fds);
                            close(clientfd);
                            close(i);
                        } else {
                            debug("Unknown socket %d\n", i);
                        }
                    }
                }
            } // end handling data/connections from clients
        } // end loop through file descriptors
    } // end main loop
    return 0;
}
// vim: set et sw=4 ai tw=80:
