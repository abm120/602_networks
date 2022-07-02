We tested our implementation mostly on our local network.

First, start the proxy server:

    ./proxy 127.0.0.1 50000

Next, in a new window, start a client connection:

    ./client 127.0.0.1 50000 alick.me

This will request the webpage at <http://alick.me>. The HTTP response
(including the header) is saved in the file `response`. The saved file
should be of size 3955 bytes (if there is no further change to the
webpage) and have the whole header information and html contents.

To test concurrent connection, we need to open a new window and start
another client:

    ./client 127.0.0.1 50000 alick.me/about
