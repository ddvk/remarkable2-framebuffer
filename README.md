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

this will generate `src/loader/librm2fb.so.1.0.0`. Copy it to your remarkable and run:

```
LD_PRELOAD=/path/to/librm2fb.so /usr/bin/remarkable-shutdown 
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

* should you distribute apps that use the LD_PRELOAD method of drawing to the framebuffer?

no, probably not. let's figure out a design that doesn't require every app to
ship a 3rd party binary.
