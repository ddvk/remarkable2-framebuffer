qmake
make
mkdir -p dist/rm2fb
cp src/loader/librm2fb_demo.so.1.0.0 dist/rm2fb/demo.so
cp src/server/librm2fb_server.so.1.0.0 dist/rm2fb/server.so
cp src/client/librm2fb_client.so.1.0.0 dist/rm2fb/client.so
cp scripts/run.sh dist/rm2fb/rm2fb.sh
chmod +x dist/rm2fb/rm2fb.sh
