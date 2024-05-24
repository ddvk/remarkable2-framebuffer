qmake
make
mkdir -p dist/rm2fb
cp src/loader/librm2fb_demo.so.1.1.0 dist/rm2fb/
cp src/server/librm2fb_server.so.1.1.0 dist/rm2fb/
cp src/client/librm2fb_client.so.1.1.0 dist/rm2fb/
cp scripts/run.sh dist/rm2fb/rm2fb.sh
chmod +x dist/rm2fb/rm2fb.sh
cd dist/
rev=`git rev-parse --short HEAD`

tar -cvzf rm2fb.${rev}.tar.gz rm2fb/
