/*
 * ** server.c -- an SBCP server
 * */
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

#include "common.h"

#define BACKLOG 10              // how many pending connections queue will hold

int main(int argc, char *argv[])
{
    fd_set read_fds;            // read file descriptor list for select()
    fd_set read_fds_tmp;        // temp read file descriptor list for select()
    int fd_max;                 // maximum file descriptor number
    unsigned short i, j;                   // loop counter

    int sockfd, new_fd;         // listen on sock_fd, new connection on new_fd
    struct sockaddr_in serv_addr;        // server's address information
    struct sockaddr_storage their_addr; // connector's address information
    socklen_t sin_size;
    const char *server_ip;
    unsigned short server_port;
    unsigned short max_clients;
    int yes = 1;
    char s[INET6_ADDRSTRLEN];
    char buf[MAXDATASIZE];
    char tx_buf[MAXDATASIZE];
    char username[MAX_USERNAME_LEN + 1];
    char reason[MAX_REASON_LEN + 1];
    char msg[MAX_MESSAGE_LEN + 1];
    unsigned short n_clients = 0;
    int numbytes;
    int len;
    unsigned short attr_len, msg_len, len_pad;

    client *clients;

    sbcp_attr attr;
    sbcp_hdr hdr;
    unsigned tmp;

    // Initialize file descriptor sets.
    FD_ZERO(&read_fds);
    FD_ZERO(&read_fds_tmp);

    // Handle command line arguments.
    if (argc != 4) {
        fprintf(stderr, "Invalid command invocation!\n");
        fprintf(stderr, "Usage: ./server server_ip server_port max_clients\n");
        exit(ERR_INVOC);
    }
    server_ip = argv[1];
    server_port = strtou16(argv[2]);
    max_clients = strtou16(argv[3]);
    clients = (client *)malloc(sizeof(client) * max_clients);

    if ((sockfd = socket(AF_INET, SOCK_STREAM, 0)) == -1) {
        perror("server: socket");
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

    if (listen(sockfd, BACKLOG) == -1) {
        perror("listen");
        exit(1);
    }

    // Add the server socket fd to the read_fds set.
    FD_SET(sockfd, &read_fds);
    fd_max = sockfd;

    printf("server: waiting for connections...\n");
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
                        // Add newly connected client to read fd sets.
                        FD_SET(new_fd, &read_fds);
                        if (new_fd > fd_max) {
                            fd_max = new_fd;
                        }
                        inet_ntop(their_addr.ss_family,
                                get_in_addr((struct sockaddr *) &their_addr),
                                s, sizeof s);
                        unsigned short port = get_in_port((struct sockaddr *)&their_addr);
                        debug("server: got connection from %s:%d (fd=%d)\n", s, port, new_fd);
                    }
                } else { // new data message
                    if ((numbytes = recv(i, buf, MAXDATASIZE, 0)) <= 0) {
                        // got error or connection closed by client
                        if (numbytes == 0) {
                            // connection closed
                            printf("server: socket %d hung up!\n", i);
                        } else {
                            perror("recv");
                        }
                        close(i);
                        FD_CLR(i, &read_fds);
                    } else { // really got some data
                        buf[numbytes] = '\0';
                        debug("server: got data (len=%d) from fd=%d: ", numbytes, i);
                        print_hex(buf, numbytes);
                        // Decode packet.
                        memcpy(&tmp, buf, 4);
                        debug("tmp=%08x\n", (unsigned)tmp);
                        hdr = ntohl(tmp);
                        debug("hdr=%08x\n", (unsigned)hdr);
                        if (get_protocol_version(&hdr) != (unsigned)3) {
                            debug("Protocol version mismatch! Discard.\n");
                            continue;
                        }
                        hdr_type type = get_hdr_type(&hdr);
                        if (type == JOIN) {
                            memcpy(&tmp, buf + 4, 4);
                            attr = ntohl(tmp);
                            debug("attr=%08x\n", (unsigned)attr);
                            if (get_attr_type(&attr) != Username) {
                                debug("JOIN message has no Username attribute!\n");
                                continue;
                            }
                            attr_len = get_attr_len(&attr);
                            msg_len = get_msg_len(&hdr);
                            if (attr_len != msg_len - 4) {
                                debug("Packet length mismatch!\n");
                                debug("attr_len=%d\n, msg_len=%d\n", attr_len, msg_len);
                                continue;
                            }
                            memset(username, '\0', sizeof(username));
                            memcpy(username, buf + 8, attr_len - 4);
                            debug("server: recv client '%s' JOIN\n", username);
                            int nak = 0;
                            if (n_clients >= max_clients) {
                                // NAK: Maximum number of clients reached.
                                len = sizeof(NAK_MAX_CLIENTS);
                                strncpy(reason, NAK_MAX_CLIENTS, MAX_REASON_LEN);
                                nak = 1;
                            }
                            for (j = 0; j < n_clients; j++) {
                                if (strcmp(clients[j].username, username) == 0) {
                                    // NAK: Username already used.
                                    len = sizeof(NAK_USERNAME);
                                    strncpy(reason, NAK_USERNAME, MAX_REASON_LEN);
                                    nak = 1;
                                    break;
                                }
                            }
                            if (nak) {
                                hdr = new_hdr(NAK);
                                attr = new_attr(Reason);

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
                                memcpy(tx_buf + 8, reason, len);
                                len = msg_len;
                                if (sendall(i, tx_buf, &len) < 0) {
                                    perror("sendall");
                                    fprintf(stderr, "We only sent %d bytes because of the error!\n", len);
                                } else {
                                    debug("%d bytes NAK sent!\n", len);
                                    tx_buf[0] = '\0';
                                }
                                continue; // continue to handle next fd
                            }
                            // Add the new client to the chat room.
                            clients[n_clients].sockfd = i;
                            strncpy(clients[n_clients].username, username, MAX_USERNAME_LEN);
                            clients[n_clients].username[MAX_USERNAME_LEN] = '\0';
                            n_clients++;
                            // Send out ACK.
                            memset(tx_buf, '\0', sizeof(tx_buf));
                            hdr = new_hdr(ACK);
                            len = 4;
                            attr = new_attr(ClientCount);
                            attr_len = 4 + 4; // attr hdr 4 + client count 4 (with padding)
                            set_attr_len(&attr, attr_len);
                            tmp = htonl(attr);
                            memcpy(tx_buf + 4, &tmp, 4);
                            tmp = htons(n_clients);
                            memcpy(tx_buf + 8, &tmp, 4);
                            len += attr_len;
                            for (j = 0; j < n_clients; j++) {
                                attr = new_attr(Username);
                                attr_len = 4 + MAX_USERNAME_LEN;
                                set_attr_len(&attr, attr_len);
                                tmp = htonl(attr);
                                memcpy(tx_buf + len, &tmp, 4);
                                memcpy(tx_buf + len + 4, clients[j].username, MAX_USERNAME_LEN);
                                len += attr_len;
                            }
                            set_msg_len(&hdr, (unsigned)len);
                            tmp = htonl(hdr);
                            debug("tmp=%08x\n", (unsigned)tmp);
                            memcpy(tx_buf, &tmp, 4);
                            if (sendall(i, tx_buf, &len) < 0) {
                                perror("sendall");
                                fprintf(stderr, "We only sent %d bytes because of the error!\n", len);
                            } else {
                                debug("%d bytes ACK sent!\n", len);
                                tx_buf[0] = '\0';
                            }
                            // end handling JOIN request (ACK/NAK)
                        } else if (type == SEND) {
                            memcpy(&tmp, buf + 4, 4);
                            attr = ntohl(tmp);
                            debug("attr=%08x\n", (unsigned)attr);
                            if (get_attr_type(&attr) != Message) {
                                debug("SEND message has no Message attribute!\n");
                                continue;
                            }
                            attr_len = get_attr_len(&attr);
                            msg_len = get_msg_len(&hdr);
                            if (attr_len != msg_len - 4) {
                                debug("Packet length mismatch!\n");
                                debug("attr_len=%d\n, msg_len=%d\n", attr_len, msg_len);
                                continue;
                            }
                            memset(msg, '\0', sizeof(msg));
                            memcpy(msg, buf + 8, attr_len - 4);
                            int flag = 0;
                            for (j = 0; j < n_clients; j++) {
                                if (clients[j].sockfd == i) {
                                    strncpy(username, clients[j].username, MAX_USERNAME_LEN);
                                    username[MAX_USERNAME_LEN] = '\0';
                                    flag = 1;
                                    break;
                                }
                            }
                            if (flag == 0) {
                                debug("No user with the sockfd %d!\n", i);
                                continue;
                            }
                            debug("server: recv client '%s' SEND\n", username);
                            // Send out FWD.
                            memset(tx_buf, '\0', sizeof(tx_buf));
                            hdr = new_hdr(FWD);
                            len = 4;
                            attr = new_attr(Username);
                            set_attr_len(&attr, 4 + MAX_USERNAME_LEN);
                            tmp = htonl(attr);
                            memcpy(tx_buf + 4, &tmp, 4);
                            memcpy(tx_buf + 8, username, MAX_USERNAME_LEN);
                            len += 4 + MAX_USERNAME_LEN;
                            attr = new_attr(Message);
                            // attr_len is the one received.
                            set_attr_len(&attr, attr_len);
                            tmp = htonl(attr);
                            memcpy(tx_buf + len, &tmp, 4);
                            len += 4;
                            memcpy(tx_buf + len, msg, attr_len - 4);
                            len += attr_len;
                            set_msg_len(&hdr, (unsigned)len);
                            tmp = htonl(hdr);
                            memcpy(tx_buf, &tmp, 4);

                            // Forward data to other clients.
                            debug("ready to forward...\n");
                            for (j = 0; j <= fd_max; j++) {
                                if (FD_ISSET(j, &read_fds) && (j != sockfd) &&
                                        (j != i)) {
                                    if (sendall(j, tx_buf, &len) < 0) {
                                        perror("sendall");
                                        fprintf(stderr, "We only sent %d bytes because of the error!\n", len);
                                    } else {
                                        debug("%d bytes FWD sent to fd=%d!\n", len, j);
                                    }
                                }
                            }
                            tx_buf[0] = '\0';
                        } // end handling SEND request (FWD)
                    }
                }
            } // end handling data/connections from clients
        } // end loop through file descriptors
    } // end main loop
    free(clients);
    return 0;
}
// vim: set et sw=4 ai tw=80:
