make clean
make && scp librm2fb_demo.so.1.0.0  rm: && ssh -tt rm "LD_PRELOAD=/home/root/librm2fb_demo.so.1.0.0 ./remarkable-shutdown"
