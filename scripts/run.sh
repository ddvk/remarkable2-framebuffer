#!/usr/bin/env bash
DIR="$( cd "$( dirname "${BASH_SOURCE[0]}" )" >/dev/null 2>&1 && pwd )"
systemctl stop xochitl
LD_PRELOAD=${DIR}/librm2fb_server.so.1.1.0 `which xochitl` &
pid=$!
sleep 2
LD_PRELOAD=${DIR}/librm2fb_client.so.1.1.0 $*
pid2=$!
wait $pid2
kill -9 -${pid}
kill -9 -${pid2}
systemctl start xochitl
