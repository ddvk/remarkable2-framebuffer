## remarkable2-framebuffer

This repo contains a proof of concept for drawing to the rM2's framebuffer.

## Status

quality: pre-alpha

rm2fb can open the framebuffer and draw to it. Additionally, it exposes a
simple API for other processes to draw to the framebuffer using shared mem and
message queues.

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
LD_PRELOAD=/path/to/librm2fb_server.so /usr/bin/remarkable-shutdown
```


### Framebuffer Client Shim

build `src/client/librm2fb_client.so.1.0.0`.  Copy it to your
remarkable and run: `LD_PRELOAD=/path/to/librm2fb_client.so <rm1app>` to
run your app.

The client intercepts interactions with `/dev/fb0` and forwards them to the
rm2fb server.

## [Demo](https://imgur.com/gallery/zGMn7Qs)


## Contributing

Please look at the open github issues and add a new one for any work you are planning
on doing so people can see ongoing progress.

Things that can use help:

* writing documentation
* setting up this repository build system and CI
* understanding the waveforms used by SWTCON
* writing our own implementation of SWTCON

## FAQ

* will this brick my rM2?

probably not, but no guarantees

* [what apps are supported?](https://github.com/ddvk/remarkable2-framebuffer/issues/14)

* [how does rm2fb work?](https://github.com/ddvk/remarkable2-framebuffer/issues/5#issuecomment-718948222)

* [how do I port my existing app to rM2?](https://github.com/ddvk/remarkable2-framebuffer/issues/13)

* [what's up with server/client API?](https://github.com/ddvk/remarkable2-framebuffer/issues/4)


* how can I get in touch?

use github issues or ping on the discord in one of the homebrew developer
channels. if you mention this repo, someone will probably respond
