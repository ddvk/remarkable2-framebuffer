## remarkable2-framebuffer

This repo contains a proof of concept for drawing to the rM2's framebuffer.

## status

quality: proof of concept

this PoC can open the framebuffer and draw to it. Additionally, it exposes a
simple API for other processes to draw to the framebuffer using shared mem and
message queues.

## building

get the remarkable toolchain and source it (as per usual), then run:

```
cd ${GITROOT}
qmake
make
```

this will generate two binaries: `src/server/librm2fb.so` and
`src/client/client`. Copy them to your remarkable and run:

```
# start the FB server
LD_PRELOAD=/path/to/librm2fb.so /usr/bin/remarkable-shutdown 

# send an example "update rect" command
./client 
```

NOTE: For this to work, your binary should have the md5sum of
`dbd4e8dfeb8810c88fc9d5a902e65961`

## contributing

Please look at the open github issues and add a new one for any work you are planning
on doing so people can see ongoing progress.

Things that can use help:

* writing documentation
* setting up this repository build system and CI
* achieving fast refresh latency
* understanding the waveforms used by SWTCON
* writing a robust client library for interacting with server process
* making a general way of finding the fb memory in xochitl and exposing it as shared mem
* writing our own implementation of SWTCON
* designing a client API that makes sense
* porting existing apps to use the rm2fb sendUpdate API
