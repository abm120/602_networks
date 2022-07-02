/* client.c -- The SBCP client application.

   Copyright (C) 2016 Alick Zhao (alick9188@gmail.com)

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
    fd_set read_fds;             // read file descriptor list for select()
    fd_set write_fds;            // write file descriptor list for select()
    fd_set read_fds_tmp;         // temp read file descriptor list for select()
    fd_set write_fds_tmp;        // temp write file descriptor list for select()
    int fd_max;                  // maximum file descriptor number
    int i;

    struct sockaddr_in saddr;
    int sockfd;
    unsigned short server_port;
    const char *server_ip;
    const char *username;
    char their_username[MAX_USERNAME_LEN + 1];
    char their_msg[MAX_MESSAGE_LEN + 1];
    char reason[MAX_REASON_LEN + 1];
    char rx_buf[MAXDATASIZE];   // buffer for received packets
    char tx_buf[MAXDATASIZE];   // buffer for packets to be send out
    char in_buf[MAXDATASIZE];   // buffer for stdin input message
    int numbytes;
    int len;
    unsigned short attr_len, msg_len, len_pad;
    unsigned tmp;

    // Initialize file descriptor sets.
    FD_ZERO(&read_fds);
    FD_ZERO(&write_fds);
    FD_ZERO(&read_fds_tmp);
    FD_ZERO(&write_fds_tmp);
    FD_SET(STDIN_FILENO, &read_fds);
    fd_max = STDIN_FILENO;

    tx_buf[0] = '\0';
    rx_buf[0] = '\0';

    // Handle command line arguments.
    if (argc != 4) {
        fprintf(stderr, "Invalid command invocation!\n");
        fprintf(stderr, "Usage: ./client username server_ip server_port\n");
        exit(ERR_INVOC);
    }
    username = argv[1];
    len = strlen(username);
    if (len > MAX_USERNAME_LEN) {
        fprintf(stderr, "Username is too long! Maximum 16 characters.\n");
        exit(ERR_USERNAME);
    }
    server_ip = argv[2];
    server_port = strtou16(argv[3]);

    if ((sockfd = socket(AF_INET, SOCK_STREAM, 0)) < 0) {
        perror("client: socket");
        exit(ERR_SOCKET);
    }
    memset(&saddr, '\0', sizeof(saddr));
    saddr.sin_family = AF_INET;
    inet_pton(AF_INET, server_ip, &(saddr.sin_addr));
    saddr.sin_port = htons(server_port);

    if (connect(sockfd, (struct sockaddr *)&saddr, sizeof(saddr)) < 0) {
        close(sockfd);
        perror("client connect");
        fprintf(stderr, "Error connecting!\n");
        exit(ERR_CONN);
    }
    debug("Client %s connected!\n", username);
    // Send JOIN message.
    sbcp_hdr hdr = new_hdr(JOIN);
    sbcp_attr attr = new_attr(Username);
    attr_len = 4 + MAX_USERNAME_LEN;
    set_attr_len(&attr, attr_len);
    debug("attr=%08x\n", (unsigned)attr);
    msg_len = 4 + attr_len;
    set_msg_len(&hdr, (unsigned)msg_len);
    debug("hdr=%08x\n", (unsigned)hdr);
    memset(tx_buf, '\0', sizeof(tx_buf));
    tmp = htonl(hdr);
    debug("tmp=%08x\n", (unsigned)tmp);
    memcpy(tx_buf, &tmp, 4);
    tmp = htonl(attr);
    memcpy(tx_buf + 4, &tmp, 4);
    memcpy(tx_buf + 8, username, MAX_USERNAME_LEN);
    len = msg_len;
    if (sendall(sockfd, tx_buf, &len) < 0) {
        perror("sendall");
        fprintf(stderr, "We only sent %d bytes because of the error!\n", len);
    } else {
        debug("%d bytes JOIN sent!\n", len);
        tx_buf[0] = '\0';
    }
    // Add the socket fd to the read/write sets.
    FD_SET(sockfd, &read_fds);
    FD_SET(sockfd, &write_fds);
    if (fd_max < sockfd) {
        fd_max = sockfd;
    }

    while (1) { // main event loop
        // Use tmp since they will be modified by select().
        read_fds_tmp = read_fds;
        write_fds_tmp = write_fds;
        if (select(fd_max + 1, &read_fds_tmp, &write_fds_tmp, NULL, NULL) == -1) {
            perror("select");
            exit(ERR_SELECT);
        }

        for (i = 0; i <= fd_max; i++) {
            if (FD_ISSET(i, &read_fds_tmp)) {
                if (i == sockfd) { // message from server
                    if ((numbytes = recv(sockfd, rx_buf, MAXDATASIZE - 1, 0)) <= 0) {
                        if (numbytes == 0) {
                            fprintf(stderr, "client: server hung up!\n");
                        } else {
                            perror("recv");
                        }
                        close(i);
                        FD_CLR(i, &read_fds);
                        exit(ERR_SERVER);
                    } else {
                        rx_buf[numbytes] = '\0';
                        debug("Client %s received: ", username);
                        print_hex(rx_buf, numbytes);
                        // Decode packet.
                        memcpy(&tmp, rx_buf, 4);
                        debug("tmp=%08x\n", (unsigned)tmp);
                        hdr = ntohl(tmp);
                        debug("hdr=%08x\n", (unsigned)hdr);
                        if (get_protocol_version(&hdr) != (unsigned)3) {
                            debug("Protocol version mismatch! Discard.\n");
                            continue;
                        }
                        hdr_type type = get_hdr_type(&hdr);
                        if (type == ACK) {
                            memcpy(&tmp, rx_buf + 4, 4);
                            attr = ntohl(tmp);
                            debug("attr=%08x\n", (unsigned)attr);
                            if (get_attr_type(&attr) != ClientCount) {
                                debug("ACK message has no ClientCount attribute!\n");
                                continue;
                            }
                            memcpy(&tmp, rx_buf + 8, 4);
                            printf("Client %s logged in!\n", username);
                            printf("%d client(s) in the chat room.\n", ntohs(tmp));
                            printf("TIP: Type your message and hit Enter key to send (Ctrl+C to exit)\n");
                            printf("<%s>: ", username);
                            fflush(stdout);
                        } else if (type == NAK) {
                            memcpy(&tmp, rx_buf + 4, 4);
                            attr = ntohl(tmp);
                            debug("attr=%08x\n", (unsigned)attr);
                            if (get_attr_type(&attr) != Reason) {
                                debug("ACK message has no Reason attribute!\n");
                                continue;
                            }
                            attr_len = get_attr_len(&attr);
                            memcpy(&reason, rx_buf + 8, attr_len - 4);
                            reason[MAX_REASON_LEN] = '\0';
                            fprintf(stderr, "Rejected: %s\n", reason);
                            exit(ERR_REJECTED);
                        } else if (type == FWD) {
                            memcpy(&tmp, rx_buf + 4, 4);
                            attr = ntohl(tmp);
                            debug("attr=%08x\n", (unsigned)attr);
                            if (get_attr_type(&attr) != Username) {
                                debug("ACK message has no Username attribute!\n");
                                continue;
                            }
                            memset(&their_username, '\0', sizeof(their_username));
                            memcpy(&their_username, rx_buf + 8, MAX_USERNAME_LEN);
                            memset(&their_msg, '\0', sizeof(their_msg));
                            int offset = 4 + 4 + MAX_USERNAME_LEN + 4;
                            memcpy(&their_msg, rx_buf + offset, numbytes - offset);
                            printf("<%s>: %s\n", their_username, their_msg);
                        }
                    }
                } else if (i == STDIN_FILENO) {
                    if (fgets(in_buf, sizeof in_buf, stdin)) {
                        len = strlen(in_buf);
                        assert(len >= 1);
                        assert(in_buf[len - 1] == '\n');
                        // Chomp trailing newline character.
                        in_buf[len - 1] = '\0';
                        /*debug("%ld bytes to be sent...\n", strlen(in_buf));*/
                    } else {
                        fprintf(stderr, "fgets error!\n");
                    }
                } else {
                    fprintf(stderr, "Unknown fd: %d\n", i);
                }
            } // end handling data from stdin/server
        } // end looping through file descriptors

        for (i = 0; i <= fd_max; i++) {
            if (FD_ISSET(i, &write_fds_tmp)) {
                if (i == sockfd) { // able to send to server
                    len = strlen(in_buf);
                    if (len > 0) {
                        debug("%d bytes ready to be sent...\n", len);
                        // SEND message.
                        sbcp_hdr hdr = new_hdr(SEND);
                        sbcp_attr attr = new_attr(Message);
                        if (len % 4) {
                            len_pad = 4 - (len % 4);
                        } else {
                            len_pad = 0;
                        }
                        attr_len = 4 + len + len_pad;
                        set_attr_len(&attr, attr_len);
                        debug("attr=%08x\n", (unsigned)attr);
                        msg_len = 4 + attr_len;
                        set_msg_len(&hdr, (unsigned)msg_len);
                        debug("hdr=%08x\n", (unsigned)hdr);
                        memset(tx_buf, '\0', sizeof(tx_buf));
                        tmp = htonl(hdr);
                        debug("tmp=%08x\n", (unsigned)tmp);
                        memcpy(tx_buf, &tmp, 4);
                        tmp = htonl(attr);
                        memcpy(tx_buf + 4, &tmp, 4);
                        memcpy(tx_buf + 8, in_buf, len);
                        len = msg_len;
                        if (sendall(sockfd, tx_buf, &len) < 0) {
                            perror("sendall");
                            fprintf(stderr, "We only sent %d bytes because of the error!\n", len);
                        } else {
                            debug("%d bytes sent: ", len);
                            print_hex(tx_buf, len);
                            in_buf[0] = '\0';
                            printf("<%s>: ", username);
                            fflush(stdout);
                        }
                    }
                } else {
                    fprintf(stderr, "Unknown fd: %d\n", i);
                }
            } // end handling sending to the server
        } // end looping through file descriptors
    } // end main event loop

    close(sockfd);
    return 0;
}
// vim: set et sw=4 ai tw=80:
