make && scp libfb.so.1.0.0 rm:main.so && ssh -tt rm "LD_PRELOAD=/home/root/main.so ./remarkable-shutdown"
