## remarkable2-framebuffer

This repo contains code for drawing to the rM2's framebuffer.

## Status

quality: beta

rm2fb can open the framebuffer and draw to it. rm2fb-server exposes a simple
API for other processes to draw to the framebuffer using shared mem and message
queues. rm2fb-client is a shim that creates a fake framebuffer device for apps
to use, allowing rM1 apps to seamlessly draw to the display of the rM2.

## Installation

Before installing, be sure that your xochitl version is supported: https://github.com/ddvk/remarkable2-framebuffer/blob/master/src/shared/config.cpp.

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

build `src/server/librm2fb_server.so.1.0.1`.  Copy it to your
remarkable and run:

```
LD_PRELOAD=/path/to/librm2fb_server.so.1.0.1 /usr/bin/xochitl
```

### Framebuffer Client Shim

build `src/client/librm2fb_client.so.1.0.1`.  Copy it to your
remarkable and run: `LD_PRELOAD=/path/to/librm2fb_client.so.1.0.1 <rm1app>` to
run your app.

The client intercepts interactions with `/dev/fb0` and forwards them to the
rm2fb server.

## Configuration

To do their job, both the client and the server need to know the address at which a number of functions reside in the Xochitl binary.
These addresses change from one release to the next.

Xochitl function name | Xochitl function role | Notes
--------------|---------------|----------
`update` | Sends an update request. This function is replaced by the client shim to talk to our server instead. | Uses the string `Unable to complete update: invalid waveform (`. Prior to version 2.9, the signature of this function was `void (*)(void*, int, int, int, int, int, int)`. Starting from version 2.9, it changed to `void (*)(void*, QRect&, int, int)`.
`create` | Start the threads that handle update requests. This function is replaced by the client shim with a no-op to avoid conflicting with the server. | Uses the string `Unable to start generator thread\n`.
`wait` | Waits until the update-handling threads have started. This function is replaced by the client shim with a no-op to avoid conflicting with the server. | Calls `usleep(1000)`.
`shutdown` | Stops the update-handling threads. This function is replaced by the client shim with a no-op to avoid conflicting with the server. | Uses the string `Shutting down...`.
`getInstance` | Retrieves the instance of the singleton SWTCON class. This function is used by the server to interact with the screen. | Calls a function that itself calls `create` and `wait`.
`notify` | Called when the framebuffer has been updated, used for the Qt signal/slot connections needed for ScreenShare to work | 

The client and the server both ship the [hardcoded addresses](https://github.com/ddvk/remarkable2-framebuffer/blob/master/src/shared/config.cpp#L13) for these functions for various releases.
If you get a message saying `Missing address for function […]`, it means that the release you’re running is not yet supported. Please report this in [this dedicated thread](https://github.com/ddvk/remarkable2-framebuffer/issues/18).

You can manually locate the addresses of the functions listed above by [looking at the disassembly and then add a configuration entry to make remarkable2-framebuffer work with your release](tutorial).
In addition to the hardcoded configuration entries, the client and the server will look for addresses in configuration files in the following locations:

* `/usr/share/rm2fb.conf`
* `/opt/share/rm2fb.conf`
* `/etc/rm2fb.conf`
* `/opt/etc/rm2fb.conf` (best option for Toltec users)
* `rm2fb.conf` (relative to the current working directory, best option for manual installs)

## [Demo](https://imgur.com/gallery/zGMn7Qs)

## Contributing

Please look at the open github issues and add a new one for any work you are planning
on doing so people can see ongoing progress.

Things that can use help:

* writing documentation
* testing apps and packages
* understanding the waveforms used by SWTCON
* writing our own implementation of SWTCON
* adding support for new OS releases

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
