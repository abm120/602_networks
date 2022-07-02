/* test_list.c -- Test linked list implementation.

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
#include "list.h"

int main(void)
{
    client *clients = new_list();
    int fd = -1;

    add_client(&clients, 1, 11);
    add_client(&clients, 2, 22);
    add_client(&clients, 5, 33);
    print_list(clients);
    fd = find_client(clients, 1);
    if (fd == -1) {
        debug("Client 1 not found!\n");
    } else {
        debug("Client 1 file fd: %d\n", fd);
    }
    remove_client(&clients, 1);
    print_list(clients);
    remove_client(&clients, 2);
    print_list(clients);
    fd = find_client(clients, 1);
    if (fd == -1) {
        debug("Client 1 not found!\n");
    } else {
        debug("Client 1 file fd: %d\n", fd);
    }
    clear_list(&clients);
    return 0;
}
// vim: set et sw=4 ai tw=80:
