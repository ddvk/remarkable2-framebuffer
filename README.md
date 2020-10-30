## remarkable2-framebuffer

This repo contains a proof of concept for drawing to the rM2's framebuffer.

## status

quality: proof of concept

this PoC can open the framebuffer and draw to it. Additionally, it exposes a
simple API for other processes to draw to the framebuffer using shared mem and
message queues.

## set up build environment

[set up remarkable toolchain](https://remarkablewiki.com/devel/qt_creator#toolchain)


## building

Source the remarkable toolchain (example below) and run the following (replacing ${GITROOT} with the directory you have checked the repository out to)

```
source /usr/local/oecore-x86_64/environment-setup-cortexa9hf-neon-oe-linux-gnueabi
cd ${GITROOT}
qmake
make
```

this will generate `src/loader/librm2fb.so.1.0.0`.  Copy it to your
remarkable and run:

```
LD_PRELOAD=/path/to/librm2fb.so /usr/bin/remarkable-shutdown
```

NOTE: For this to work, your binary should have the md5sum of
`fec600ccae7743dd4e5d8046427244c0` from rm2 2.4.1.30

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

[see this proposal](https://github.com/ddvk/remarkable2-framebuffer/issues/4)

* should I distribute apps that use the LD_PRELOAD method of drawing to the framebuffer?

no, probably not. let's figure out a design that doesn't require every app to
ship a 3rd party binary.

## Memory locations

### 2.4.1.30

md5sum fec600ccae7743dd4e5d8046427244c0
0x21F54
0x21A34

### 2.3~

md5sum dbd4e8dfeb8810c88fc9d5a902e65961
0x224BC
0x2257C
