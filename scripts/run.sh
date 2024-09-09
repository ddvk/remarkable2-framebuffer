#!/usr/bin/env bash
DIR="$( cd "$( dirname "${BASH_SOURCE[0]}" )" >/dev/null 2>&1 && pwd )"
systemctl stop xochitl
version=$(grep VERSION= version.pri | cut -d= -f2)
LD_PRELOAD=${DIR}/librm2fb_server.so.$version `which xochitl` &
pid=$!
sleep 2
LD_PRELOAD=${DIR}/librm2fb_client.so.$version $*
pid2=$!
wait $pid2
kill -9 -${pid}
kill -9 -${pid2}
systemctl start xochitl
