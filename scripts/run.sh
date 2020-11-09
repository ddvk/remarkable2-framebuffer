#!/usr/bin/env bash
DIR="$( cd "$( dirname "${BASH_SOURCE[0]}" )" >/dev/null 2>&1 && pwd )"

LD_PRELOAD=${DIR}/server.so `which remarkable-shutdown` &
pid=$!
sleep 2
LD_PRELOAD=${DIR}/client.so $*
kill ${pid}
