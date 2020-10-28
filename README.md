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

## FAQ

* will this brick my rM2?

probably not, but no guarantees

* how do I port my existing app to rM2?

we are figuring out how to give app developers a simple to use API that is
almost a drop in replacement for the rM1 framebuffer rendering code

* how can I get in touch?

use github issues or ping on the discord in one of the homebrew developer
channels. if you mention this repo, someone will probably respond

* what's up with server/client API?

The current way we are interacting with the framebuffer requires modifying a
binary - we figure it is better to isolate that into one process and the rest
of the programs can use simple IPC to communicate with that process.

In addition, using a server/client API will potentially make new features
possible that were not doable on the rM1, since it will enforce that apps
have some sort of coordination between them.

* should you distribute apps that use the LD_PRELOAD method of drawing to the framebuffer?

no, probably not. let's figure out a design that doesn't require every app to
ship a 3rd party binary.
