# simple-http-proxy

This is the simple HTTP proxy created by Tao Zhao and Anil Balasubramanya Murthy
(Team 20) for the third network programming assignment of TAMU ECEN 602 in 2016 fall.

## Build & Installation

We use the standard Makefile for building the project. Open up a terminal
window, change into the (decompressed) project directory, and execute

    make

This will build both the proxy and the client application. Standard
build tool chain (such as the GCC suite) is required to build the package.

To clean up the auxiliary files generated by the building process, issue

    make cleanobj

If you want to remove executable files (`server`) in addition
to the auxiliary files, use

    make clean

You can use the proxy and the client application directly from the project
directly. Installation is not required.

## Usage

On the proxy server side, issue

    ./proxy proxy_ip proxy_port

On the client side, use

    ./client proxy_ip proxy_port url

Multiple clients can connect to the same server. Make sure you specify the
server IP address and port number consistently. We recommend a high port
number (like 50000) for unprivileged users.

Make sure the url is spelled correctly.

Use `Ctrl+C` to terminate either the server or the client.

## Errata
