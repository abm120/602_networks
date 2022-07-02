#define _XOPEN_SOURCE
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <strings.h>
#include <time.h>
#include <sys/time.h>

#include "common.h"
#include "cache.h"

static unsigned num_entries = 0;

entry *new_cache(void)
{
    num_entries = 0;
    return (entry *)NULL;
}

void print_cache(entry * head)
{
    entry * current = head;

    if (!head) {
        debug("Empty cache!\n");
        return;
    }

    while (current) {
        debug("%s%s: ", current->domain, current->page);
        debug("'%s'", current->response);
        debug(", %s expire", (current->may_expire) ? ("may") : ("never"));
        debug(", atime %ld.%ld", current->atime.tv_sec, current->atime.tv_usec);
        debug(", last modified: %ld", current->modified_time);
        debug(", expired at: %ld\n", current->expiry_time);
        current = current->next;
    }
}


// Add a new entry to the front of the linked cache.
void add_entry(entry ** head,
               char *domain,
               char *page,
               char *response,
               struct timeval atime)
{
    entry * e = (entry *)malloc(sizeof(entry));
    char timestr[100];
    struct tm tm;
    int len;    // length of date time string
    char *p;
    char *q;

    // LRU eviction.
    entry *current;
    char domain_rm[MAX_HOST_LEN + 1];
    char page_rm[MAX_REL_URL_LEN + 1];
    struct timeval atime_rm;
    long t_rm, t_cur;

    strlcpy(e->domain, domain, sizeof(e->domain));
    strlcpy(e->page, page, sizeof(e->page));
    strlcpy(e->response, response, sizeof(e->response));
    e->atime = atime;

    p = strstr(response, "Expires: ");
    if (!p) {
        e->may_expire = false;
        e->modified_time = 0;
        e->expiry_time = 0;
    } else {
        e->may_expire = true;
        /*p += strlen("Expires: ");*/
        p += 9; // skip 'Expires: '
        q = strstr(p, "\r\n");
        if (!q) {
            // Try again to find a single newline.
            q = strstr(p, "\n");
        }
        if (!q) {
            debug("Bad Expires header!");
            // Assume already expired.
            e->expiry_time = 0;
        }
        len = q - p;
        strlcpy(timestr, p, len + 1);
        debug("timestr: %s\n", timestr);
        memset(&tm, 0, sizeof(struct tm));
        if (strptime(timestr, "%a, %d %b %Y %T GMT", &tm)) {
            e->expiry_time = mktime(&tm);
        } else {
            debug("Bad Expires header!");
            // Assume already expired.
            e->expiry_time = 0;
        }
    }

    //TODO: parse last modified header
    e->modified_time = 0;

    while (num_entries >= MAX_NUM_ENTRIES) {
        // Evict least recent used.
        current = (*head);
        atime_rm = current->atime;
        t_rm = atime_rm.tv_sec * 1000000 + atime_rm.tv_usec;
        strlcpy(domain_rm, current->domain, sizeof(domain_rm));
        strlcpy(page_rm, current->page, sizeof(page_rm));
        current = current->next;
        while (current) {
            t_cur = current->atime.tv_sec * 1000000 + current->atime.tv_usec;
            if (t_cur < t_rm) {
                atime_rm = current->atime;
                t_rm = atime_rm.tv_sec * 1000000 + atime_rm.tv_usec;
                strlcpy(domain_rm, current->domain, sizeof(domain_rm));
                strlcpy(page_rm, current->page, sizeof(page_rm));
            }
            current = current->next;
        }
        remove_entry(head, domain_rm, page_rm);
    }

    if (*head) {
        e->next = (*head);
    } else {
        e->next = NULL;
    }
    *head = e;
    num_entries++;
    debug("num_entries=%d\n", num_entries);
}

void remove_entry(entry **head, char *domain, char *page)
{
    entry ** pp = head; // pointer to a pointer
    while (*pp) {
        entry * e = *pp;
        if (streqi(e->domain, domain) && streqi(e->page, page)) {
            *pp = e->next;
            free(e);
        } else {
            pp = &(e->next);
        }
    }
    num_entries--;
}

// Find the cache entry per domain name and page url.
// RETURN a pointer to the entry or NULL if no such entry can be found.
entry *find_entry(entry *head, char *domain, char *page)
{
    entry *ret = NULL;
    entry *current = head;
    while (current) {
        if (streqi(current->domain, domain) && streqi(current->page, page)) {
            ret = current;
            break;
        } else {
            current = current->next;
        }
    }
    return ret;
}

// Empty the entry cache.
void clear_cache(entry **head)
{
    while (*head) {
        entry * e = *head;
        *head = e->next;
        free(e);
    }
    num_entries = 0;
}

void update_entry(entry *head, char *domain, char *page, struct timeval atime)
{
    entry * current = head;
    while (current) {
        if (streqi(current->domain, domain) && streqi(current->page, page)) {
            current->atime = atime;
            break;
        } else {
            current = current->next;
        }
    }
}
