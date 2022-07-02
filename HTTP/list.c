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
        debug("socket fd = %d ", current->sockfd);
        debug("(server fd = %d) ", current->servfd);
        debug("%s%s, ", current->domain, current->page);
        current = current->next;
    }
}


// Add a new client to the front of the linked list.
void add_client(client ** head, int sockfd)
{
    client * entry = (client *)malloc(sizeof(client));
    entry->sockfd = sockfd;
    entry->servfd = -1;
    entry->domain[0] = '\0';
    entry->page[0] = '\0';
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

// Find a client with a given sockfd.
// RETURN a pointer to the client node if there is.
// RETURN NULL if no such client can be found.
client *find_client(client * head, int sockfd)
{
    client *ret = NULL;
    client * current = head;
    while (current) {
        if (current->sockfd == sockfd) {
            ret = current;
            break;
        } else {
            current = current->next;
        }
    }
    return ret;
}

// Find a client with a given servfd.
// RETURN  the client sockfd if there is a client with such servfd.
// RETURN -1 indicates no such client can be found.
int find_client_by_servfd(client * head, int servfd)
{
    int ret = -1;
    client * current = head;

    if (servfd < 0) {
        return ret;
    }

    while (current) {
        if (current->servfd == servfd) {
            ret = current->sockfd;
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

// Update the information of a client with sockfd.
// Update happens only when servfd nonnegative, domain or page non NULL.
void update_client(client *head, int sockfd, int servfd, char *domain, char *page)
{
    client * current = head;
    while (current) {
        if (current->sockfd == sockfd) {
            if (servfd >= 0) {
                current->servfd = servfd;
            }
            if (domain) {
                strlcpy(current->domain, domain, sizeof(current->domain));
            }
            if (page) {
                strlcpy(current->page, page, sizeof(current->page));
            }
            break;
        } else {
            current = current->next;
        }
    }
}
