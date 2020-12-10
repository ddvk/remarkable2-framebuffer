qmake
make
mkdir -p ./dist
strip ./src/client/librm2fb_client.so.1.0.0
cp ./src/xofb/librm2fb_xofb.so.1.0.0 ./dist
cp ./src/server/librm2fb_server.so.1.0.0 ./dist
cp ./src/client/librm2fb_client.so.1.0.0 ./dist
