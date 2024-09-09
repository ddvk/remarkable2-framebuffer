qmake
make
mkdir -p ./dist
version=$(grep VERSION= version.pri | cut -d= -f2)
arm-linux-gnueabihf-strip ./src/client/librm2fb_client.so.$version
cp ./src/xofb/librm2fb_xofb.so.$version ./dist
cp ./src/server/librm2fb_server.so.$version ./dist
cp ./src/client/librm2fb_client.so.$version ./dist
