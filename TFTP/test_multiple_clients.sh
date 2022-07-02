#!/bin/sh

txtfile="2.txt"
binfile="b.tar.gz"

./server 127.0.0.1 50000 ./ &
spid=$!

cd client/
rm "$binfile" "$txtfile"

tftp -m octet 127.0.0.1 50000 -c get "$binfile" &
pid=$!
tftp -m ascii 127.0.0.1 50000 -c get "$txtfile"

trap "kill $pid" SIGINT

wait $pid

diff "$txtfile" ../"$txtfile"
md5sum "$binfile" ../"$binfile"

cd ..
kill -2 $spid
