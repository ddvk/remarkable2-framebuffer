## remarkable2-framebuffer

This repo contains code for drawing to the rM2's framebuffer.

## Status

quality: beta

rm2fb can open the framebuffer and draw to it. rm2fb-server exposes a simple
API for other processes to draw to the framebuffer using shared mem and message
queues. rm2fb-client is a shim that creates a fake framebuffer device for apps
to use, allowing rM1 apps to seamlessly draw to the display of the rM2.

## Installation

rm2fb is available as a package in [toltec](https://github.com/toltec-dev/toltec) which
sets up the server and client parts for you. Otherwise, the instructions below can be used
for manual installation.

## Set up build environment

[set up remarkable toolchain](https://remarkablewiki.com/devel/qt_creator#toolchain)


## Building

Source the remarkable toolchain (example below) and run the following (replacing ${GITROOT} with the directory you have checked the repository out to)

```
source /usr/local/oecore-x86_64/environment-setup-cortexa9hf-neon-oe-linux-gnueabi
cd ${GITROOT}
qmake
make
```


### Framebuffer Server

build `src/server/librm2fb_server.so.1.0.0`.  Copy it to your
remarkable and run:

```
LD_PRELOAD=/path/to/librm2fb_server.so.1.0.0 /usr/bin/remarkable-shutdown
```

### Framebuffer Client Shim

build `src/client/librm2fb_client.so.1.0.0`.  Copy it to your
remarkable and run: `LD_PRELOAD=/path/to/librm2fb_client.so.1.0.0 <rm1app>` to
run your app.

The client intercepts interactions with `/dev/fb0` and forwards them to the
rm2fb server.

## [Demo](https://imgur.com/gallery/zGMn7Qs)


## Contributing

Please look at the open github issues and add a new one for any work you are planning
on doing so people can see ongoing progress.

Things that can use help:

* writing documentation
* testing apps and packages
* understanding the waveforms used by SWTCON
* writing our own implementation of SWTCON

## FAQ

* will this brick my rM2?

probably not, but no guarantees

* [what apps are supported?](https://github.com/ddvk/remarkable2-framebuffer/issues/14)

* [how does rm2fb work?](https://github.com/ddvk/remarkable2-framebuffer/issues/5#issuecomment-718948222)

* [how do I port my existing app to rM2?](https://github.com/ddvk/remarkable2-framebuffer/issues/13)

* [what's up with server/client API?](https://github.com/ddvk/remarkable2-framebuffer/issues/4)

* [where can i find more information on the framebuffer?](https://remarkablewiki.com/tech/rm2_framebuffer)

* [what about implementing an open source SWTCON?](https://github.com/timower/rM2-stuff/)

* how can I get in touch?

use github issues or ping on the discord in one of the homebrew developer
channels. if you mention this repo, someone will probably respond
