/* test_cache.c -- Test LRU cache implementation.

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

#include <stdlib.h>
#include "common.h"
#include "cache.h"

int main(void)
{
    cache cache = new_cache();
    entry *rv = NULL;
    struct timeval tv;
    tv.tv_sec = 0;
    tv.tv_usec = 0;

    add_entry(&cache, "alick.me",    "/", "", tv);
    add_entry(&cache, "bigeagle.me", "/", "", tv);
    add_entry(&cache, "maskray.me",  "/", "", tv);
    print_cache(cache);
    debug("\n");
    rv = find_entry(cache, "alick.me", "/");
    if (!rv) {
        debug("Not found!\n");
    } else {
        debug("Found!\n");
    }
    remove_entry(&cache, "alick.me", "/");
    print_cache(cache);
    debug("\n");
    remove_entry(&cache, "xxx", "yyy");
    print_cache(cache);
    debug("\n");
    rv = find_entry(cache, "alick.me", "/");
    if (!rv) {
        debug("Not found!\n");
    } else {
        debug("Found!\n");
    }
    tv.tv_sec = 1;
    update_entry(cache, "maskray.me", "/", tv);
    print_cache(cache);
    clear_cache(&cache);
    return 0;
}
// vim: set et sw=4 ai tw=80:
