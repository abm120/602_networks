/* server.c -- main server code.

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
#include <sys/stat.h>
#include <fcntl.h>
#include <sys/time.h>

#include "common.h"
#include "list.h"

int main(int argc, char *argv[])
{
    fd_set read_fds;            // read file descriptor list for select()
    fd_set read_fds_tmp;        // temp read file descriptor list for select()
    int fd_max;                 // maximum file descriptor number
    unsigned short i;                   // loop counter

    int sockfd, new_fd;         // listen on sock_fd, new connection on new_fd
    struct sockaddr_in serv_addr;        // server's address information
    struct sockaddr_storage their_addr; // connector's address information
    socklen_t sin_size;         // their socket length
    unsigned short port;        // their port
    char s[INET6_ADDRSTRLEN];   // their IP address (human readable)

    // Value from the command line.
    const char *server_ip;
    unsigned short server_port;
    const char *dir;

    int yes = 1;
    char buf[MAXDATASIZE + 1];
    char tx_buf[MAXDATASIZE + 1];
    unsigned short n_clients = 0;
    int numbytes;
    int len;

    client *clients = new_list();
    client *current;
    char *filename = NULL;
    char *path = NULL;  // full path to the file (directory + filename)
    int len_path;      // length of full path
    int fd = -1;     // file descriptor for file to read
    struct stat stat_buf;

    unsigned short tmp;         // temporary storage for 2 bytes in message
    opcode_type opcode;
    unsigned short block_num = 0;
    int send_data = 0;          // Send DATA (1) or ERROR (0) packet.

    int timeout = 0;
    struct timeval tv;
    tv.tv_sec = 5;      // Default timeout: 1s
    tv.tv_usec = 0;
    struct timeval tm;  // Current time
    long dt, t1, t2;            // time difference

    int rv;     // holding return value

    // Initialize file descriptor sets.
    FD_ZERO(&read_fds);
    FD_ZERO(&read_fds_tmp);

    // Handle command line arguments.
    if (argc != 4) {
        fprintf(stderr, "Invalid command invocation!\n");
        fprintf(stderr, "Usage: ./server server_ip server_port directory\n");
        exit(ERR_INVOC);
    }
    server_ip = argv[1];
    server_port = strtou16(argv[2]);
    dir = argv[3];

    if ((sockfd = socket(AF_INET, SOCK_DGRAM, 0)) == -1) {
        perror("server: main socket");
        exit(ERR_SOCKET);
    }
    if (setsockopt(sockfd, SOL_SOCKET, SO_REUSEADDR, &yes,
                sizeof(int)) == -1) {
        perror("setsockopt");
        exit(ERR_SOCKOPT);
    }

    memset(&serv_addr, '\0', sizeof(serv_addr));
    serv_addr.sin_family = AF_INET;
    inet_pton(AF_INET, server_ip, &(serv_addr.sin_addr));
    serv_addr.sin_port = htons(server_port);

    if (bind(sockfd, (struct sockaddr *)&serv_addr, sizeof serv_addr) == -1) {
        close(sockfd);
        perror("server: bind");
    }

    // Add the server socket fd to the read_fds set.
    FD_SET(sockfd, &read_fds);
    fd_max = sockfd;

    printf("server: waiting for connections...\n");
    while (1) {                 // main event loop
        // Use tmp since they will be modified by select().
        read_fds_tmp = read_fds;
        if (n_clients == 0) {
            rv = select(fd_max + 1, &read_fds_tmp, NULL, NULL, NULL);
        } else {
            rv = select(fd_max + 1, &read_fds_tmp, NULL, NULL, &tv);
        }
        if (rv == -1) {
            perror("select");
            exit(ERR_SELECT);
        }

        timeout = 1;
        for (i = 0; i <= fd_max; i++) {
            if (FD_ISSET(i, &read_fds_tmp)) {
                timeout = 0;
                debug("No timeout this time!\n");
                if (i == sockfd) { // new client connection
                    sin_size = sizeof their_addr;
                    numbytes = recvfrom(sockfd, buf, MAXDATASIZE, 0,
                            (struct sockaddr *) &their_addr, &sin_size);
                    if (numbytes < 0) {
                        perror("recvfrom");
                    } else if (numbytes == 0) {
                        debug("zero length message!\n");
                        continue;
                    } else {
                        buf[numbytes] = '\0';
                        /*debug("server: got message (len=%d): ", numbytes);*/
                        /*print_hex(buf, numbytes);*/
                        // Sanity check and send first block of data.
                        memcpy(&tmp, buf, OPCODE_LEN);
                        opcode = (opcode_type)ntohs(tmp);
                        if (opcode != RRQ) {
                            debug("Unknown opcode: %d\n", opcode);
                            continue;
                        }
                        // Get filename length.
                        len = strlen(buf + OPCODE_LEN);
                        if (len <= 0) {
                            debug("filename length is %d!\n", len);
                            continue;
                        }
                        filename = (char *)malloc(sizeof(char) * (len + 1));
                        strncpy(filename, buf + OPCODE_LEN, len + 1);
                        filename[len] = '\0';
                        debug("RRQ filename: %s\n", filename);
                        len_path = len + strlen(dir) + 1;
                        path = (char *)malloc(sizeof(char) * (len_path));
                        strncpy(path, dir, strlen(dir) + 1);
                        strncat(path, filename, len);
                        path[len_path] = '\0';
                        debug("RRQ path: %s\n", path);
                        if (stat(path, &stat_buf) == -1) {
                            perror("stat");
                        }
                        if (stat_buf.st_size > MAX_FILE_SIZE) {
                            debug(FILE_TOO_LARGE "\n");
                            // Send ERROR 0 FILE_TOO_LARGE
                            memset(tx_buf, '\0', sizeof(tx_buf));
                            tmp = htons(ERROR);
                            memcpy(tx_buf, &tmp, OPCODE_LEN);
                            tmp = htons(0);
                            memcpy(tx_buf + OPCODE_LEN, &tmp, ERROR_NUM_LEN);
                            memcpy(tx_buf + OPCODE_LEN + ERROR_NUM_LEN, FILE_TOO_LARGE, sizeof(FILE_TOO_LARGE) + 1);
                        } else { // File is OK.
                            fd = open(path, O_RDONLY);
                            free(path);
                            free(filename);
                            if (fd == -1) {
                                perror("open");
                                continue;
                            }
                            memset(tx_buf, '\0', sizeof(tx_buf));
                            tmp = htons(DATA);
                            memcpy(tx_buf, &tmp, OPCODE_LEN);
                            // This is the first block.
                            block_num = 1;
                            tmp = htons(block_num);
                            memcpy(tx_buf + OPCODE_LEN, &tmp, BLOCK_NUM_LEN);
                            // Note the variable numbytes is reused.
                            numbytes = read(fd, tx_buf + OPCODE_LEN + BLOCK_NUM_LEN, BLOCK_LEN);
                            if (numbytes == -1) {
                                perror("read");
                                continue;
                            } else if (numbytes == 0) {
                                debug("EOF\n");
                                continue;
                            }
                            send_data = 1;
                        }
                        if ((new_fd = socket(AF_INET, SOCK_DGRAM, 0)) == -1) {
                            perror("server: new socket");
                            exit(ERR_SOCKET);
                        }
                        if (send_data) {
                            // Add newly connected client to read fd sets.
                            FD_SET(new_fd, &read_fds);
                            if (new_fd > fd_max) {
                                fd_max = new_fd;
                            }
                            add_client(&clients, new_fd, fd);
                            n_clients++;
                            debug("Added a new client.\n");
                            print_list(clients);
                            debug("--------------------\n");
                        }
                        inet_ntop(their_addr.ss_family,
                                get_in_addr((struct sockaddr *) &their_addr),
                                s, sizeof s);
                        port = get_in_port((struct sockaddr *)&their_addr);
                        debug("server: got new connection from %s:%d (new fd=%d)\n", s, port, new_fd);
                        len = OPCODE_LEN + BLOCK_NUM_LEN + numbytes;
                        // Note the variable numbytes is reused again.
                        if ((numbytes = sendto(new_fd, tx_buf, len, 0, (struct sockaddr *)&their_addr, sizeof(their_addr))) == -1) {
                            perror("sendto");
                        } else {
                            if (send_data) {
                                debug("%d bytes DATA (blk=%d) message sent!\n", numbytes, block_num);
                                gettimeofday(&tm, NULL);
                                update_client(clients, new_fd, block_num, tm);
                            } else {
                                /*debug("%d bytes ERROR (blk=%d) message sent!\n", numbytes, block_num);*/
                                close(new_fd);
                            }
                        }
                    }
                } else { // message from existing client connection
                    sin_size = sizeof their_addr;
                    if ((numbytes = recvfrom(i, buf, MAXDATASIZE, 0,
                                    (struct sockaddr *)&their_addr, &sin_size)) <= 0) {
                        // got error or connection closed by client
                        if (numbytes == 0) {
                            debug("zero length message!\n");
                            continue;
                        } else {
                            perror("recvfrom");
                            close(i);
                            FD_CLR(i, &read_fds);
                        }
                    } else { // really got some message
                        buf[numbytes] = '\0';
                        /*debug("server: got message (len=%d) from fd=%d: ", numbytes, i);*/
                        /*print_hex(buf, numbytes);*/

                        // Decode message.
                        memcpy(&tmp, buf, OPCODE_LEN);
                        opcode = (opcode_type)ntohs(tmp);
                        if (opcode == ACK) {
                            // Get the ACKed block number.
                            memcpy(&tmp, buf + OPCODE_LEN, BLOCK_NUM_LEN);
                            block_num = ntohs(tmp);
                            /*debug("ACK (blk=%d)\n", block_num);*/
                            memset(tx_buf, '\0', sizeof(tx_buf));
                            fd = find_client(clients, i);
                            /*debug("client sockfd=%d has filefd=%d\n", i, fd);*/
                            lseek(fd, block_num * BLOCK_LEN, SEEK_SET);
                            numbytes = read(fd, tx_buf + OPCODE_LEN + BLOCK_NUM_LEN, BLOCK_LEN);
                            if (numbytes == -1) {
                                perror("read");
                                continue;
                            } else if (numbytes == 0) {
                                debug("EOF\n");
                                // That was the last block. We are done.
                                remove_client(&clients, i);
                                debug("Removed a client.\n");
                                print_list(clients);
                                debug("--------------------\n");
                                n_clients--;
                                close(fd);
                                close(i);
                                FD_CLR(i, &read_fds);
                                continue;
                            }
                            tmp = htons(DATA);
                            memcpy(tx_buf, &tmp, OPCODE_LEN);
                            block_num++;
                            tmp = htons(block_num);
                            memcpy(tx_buf + OPCODE_LEN, &tmp, BLOCK_NUM_LEN);
                            len = OPCODE_LEN + BLOCK_NUM_LEN + numbytes;
                            // Note the variable numbytes is reused again.
                            if ((numbytes = sendto(i, tx_buf, len, 0, (struct sockaddr *)&their_addr, sizeof(their_addr))) == -1) {
                                perror("sendto");
                            } else {
                                debug("%d bytes DATA (blk=%d) message sent!\n", numbytes, block_num);
                                gettimeofday(&tm, NULL);
                                update_client(clients, new_fd, block_num, tm);
                            }
                        } else if (opcode == ERROR) {
                            debug("ERROR\n");
                        } else {
                            debug("Unknown opcode: %d\n", opcode);
                        }
                    }
                }
            } // end handling data/connections from clients
        } // end loop through file descriptors
        if (timeout) {
            gettimeofday(&tm, NULL);
            t2 = tm.tv_sec * 1000000 + tm.tv_usec;
            current = clients;
            while (current) {
                t1 = current->tx_time.tv_sec * 1000000 + current->tx_time.tv_usec;
                dt = t2 - t1;
                if (dt > tv.tv_sec * 1000000 + tv.tv_usec) {
                    // Retransmit for the client.
                    fd = current->filefd;
                    block_num = current->block_num;
                    memset(tx_buf, '\0', sizeof(tx_buf));
                    lseek(fd, block_num * BLOCK_LEN, SEEK_SET);
                    numbytes = read(fd, tx_buf + OPCODE_LEN + BLOCK_NUM_LEN, BLOCK_LEN);
                    tmp = htons(DATA);
                    memcpy(tx_buf, &tmp, OPCODE_LEN);
                    tmp = htons(block_num);
                    memcpy(tx_buf + OPCODE_LEN, &tmp, BLOCK_NUM_LEN);
                    len = OPCODE_LEN + BLOCK_NUM_LEN + numbytes;
                    // Note the variable numbytes is reused again.
                    if ((numbytes = sendto(current->sockfd, tx_buf, len, 0, (struct sockaddr *)&their_addr, sizeof(their_addr))) == -1) {
                        perror("sendto");
                    } else {
                        debug("%d bytes DATA (blk=%d) message for %d retransmitted!\n", numbytes, block_num, current->sockfd);
                        gettimeofday(&tm, NULL);
                        update_client(clients, new_fd, block_num, tm);
                    }
                }
                current = current->next;
            }
        }
    } // end main loop
    close(sockfd);
    clear_list(&clients);
    return 0;
}
// vim: set et sw=4 ai tw=80:
