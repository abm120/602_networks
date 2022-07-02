/* list.h: header files for client linked list.

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

#ifndef LIST_H
#define LIST_H

#include <sys/time.h>

typedef struct client_node {
    struct client_node * next;
    int sockfd;
    int servfd; // servfd associated with the client sockfd at the proxy
    char domain[MAX_HOST_LEN + 1];
    char page[MAX_REL_URL_LEN + 1];
} client;

#ifdef __cplusplus
extern "C" { 	/* Assume C declarations for C++   */
#endif  /* __cplusplus */

client *new_list(void);
void print_list(client *head);
void add_client(client **head, int sockfd);
void remove_client(client **head, int sockfd);
client *find_client(client *head, int sockfd);
int find_client_by_servfd(client * head, int servfd);
void clear_list(client ** head);
void update_client(client *head, int sockfd, int servfd, char *domain, char *page);

#ifdef __cplusplus
}		/* End of extern "C" { */
#endif  /* __cplusplus */

#endif /* LIST_H */
