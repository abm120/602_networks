/* cache.h: header files for the LRU cache of proxy.

   Copyright (C) 2016 Tao Zhao (alick@tamu.edu)

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

#ifndef CACHE_H
#define CACHE_H

#include <sys/time.h>
#include <stdbool.h>

#define MAX_NUM_ENTRIES 10

typedef struct cache_entry_node {
    struct cache_entry_node * next;
    char domain[MAX_HOST_LEN + 1];
    char page[MAX_REL_URL_LEN + 1];
    char response[MAX_HTTP_RESPONSE_LEN + 1];
    bool may_expire;    // whether the entry might be expired.
    struct timeval atime;       // access time of the cache entry
    time_t modified_time;       // last modified time (in epoch)
    time_t expiry_time;       // expiry time (in epoch)
} entry;

typedef entry * cache;

#ifdef __cplusplus
extern "C" { 	/* Assume C declarations for C++   */
#endif  /* __cplusplus */

entry *new_cache(void);
void print_cache(entry *head);
void add_entry(entry ** head,
               char *domain,
               char *page,
               char *response,
               struct timeval atime);
void remove_entry(entry **head, char *domain, char *page);
entry *find_entry(entry *head, char *domain, char *page);
void clear_cache(entry ** head);
void update_entry(entry *head, char *domain, char *page, struct timeval atime);

#ifdef __cplusplus
}		/* End of extern "C" { */
#endif  /* __cplusplus */

#endif /* CACHE_H */
