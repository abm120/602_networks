#include <stdio.h>
#include <stdlib.h>

#include "common.h"
#include "list.h"

client *new_list(void)
{
    return (client *)NULL;
}

void print_list(client * head)
{
    client * current = head;

    if (!head) {
        debug("Empty list!\n");
        return;
    }

    while (current) {
        debug("socket fd = %d, ", current->sockfd);
        debug("file fd = %d\n", current->filefd);
        current = current->next;
    }
}


// Add a new client to the front of the linked list.
void add_client(client ** head, int sockfd, int filefd)
{
    client * entry = (client *)malloc(sizeof(client));
    entry->sockfd = sockfd;
    entry->filefd = filefd;
    if (*head) {
        entry->next = (*head);
    } else {
        entry->next = NULL;
    }
    *head = entry;
}

void remove_client(client ** head, int sockfd) {
    client ** pp = head; // pointer to a pointer
    while (*pp) {
        client * entry = *pp;
        if (entry->sockfd == sockfd)
        {
            *pp = entry->next;
            free(entry);
        } else {
            pp = &(entry->next);
        }
    }
}

// Find the filefd associated with a client per sockfd.
// RETURN -1 indicates no such client can be found.
int find_client(client * head, int sockfd)
{
    int ret = -1;
    client * current = head;
    while (current) {
        if (current->sockfd == sockfd) {
            ret = current->filefd;
            break;
        } else {
            current = current->next;
        }
    }
    return ret;
}

// Empty the client list.
void clear_list(client **head)
{
    while (*head) {
        client * entry = *head;
        *head = entry->next;
        free(entry);
    }
}

void update_client(client *head, int sockfd, unsigned short block_num, struct timeval tx_time)
{
    client * current = head;
    while (current) {
        if (current->sockfd == sockfd) {
            current->block_num = block_num;
            current->tx_time = tx_time;
            break;
        } else {
            current = current->next;
        }
    }
}
