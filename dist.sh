#!/bin/sh -eu

qmake
make
mkdir -p ./dist
arm-linux-gnueabihf-strip ./src/client/librm2fb_client.so.1.0.1

nm --dynamic --defined-only --extern-only ./src/client/librm2fb_client.so.1.0.1 | grep -E ' T init$'
nm --dynamic --defined-only --extern-only ./src/client/librm2fb_client.so.1.0.1 | grep -E ' T _ZN6QImageC1EiiNS_6FormatE$'
nm --dynamic --defined-only --extern-only ./src/client/librm2fb_client.so.1.0.1 | grep -E ' T open64$'
nm --dynamic --defined-only --extern-only ./src/client/librm2fb_client.so.1.0.1 | grep -E ' T open$'
nm --dynamic --defined-only --extern-only ./src/client/librm2fb_client.so.1.0.1 | grep -E ' T close$'
nm --dynamic --defined-only --extern-only ./src/client/librm2fb_client.so.1.0.1 | grep -E ' T ioctl$'
nm --dynamic --defined-only --extern-only ./src/client/librm2fb_client.so.1.0.1 | grep -E ' T _Z7qputenvPKcRK10QByteArray$'
nm --dynamic --defined-only --extern-only ./src/client/librm2fb_client.so.1.0.1 | grep -E ' T __libc_start_main$'

cp ./src/xofb/librm2fb_xofb.so.1.0.1 ./dist
cp ./src/server/librm2fb_server.so.1.0.1 ./dist
cp ./src/client/librm2fb_client.so.1.0.1 ./dist
