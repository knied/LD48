# Deep Space Janitor

This is my entry for the 48h variant of Ludum Dare 48:
https://ldjam.com/events/ludum-dare/48/deep-space-janitor

Only the game part in `src/frontend` was written during the 48 hours. The rest is considered engine/base code.

# Server Instructions

The following instructions are for macos only. I assume you can get this project to compile and run on Linux without too much hassle. But I have not tried it myself. Windows would presumably require a bit more tinkering with the dependencies.

## Dependencies

```
brew install scons
brew install libev
brew install libressl
brew install llvm
```

## Extra WASM/WASI Dependencies

Download prebuilt **sysroot** and **clang_rt** from [wasi-sdk](https://github.com/WebAssembly/wasi-sdk/releases)

Extend Homebrew llvm installation to support wasi:
```
mkdir -p /usr/local/opt/llvm/lib/clang/<version>/lib/wasi
cp libclang_rt.builtins-wasm32.a  /usr/local/opt/llvm/lib/clang/<version>/lib/wasi/
```

Unpack the sysroot package and place it somewhere for the build to find.

## Environment Setup

The scons build expects a few environment variables to be set:

```
export HOMEBREW_PREFIX=/usr/local
export WASI_SYSROOT=/path/to/wasi-sysroot
```

## Compile

```
scons
```

## Run Unittests

```
scons --check
```

## Run

### Development Mode

Launch a development server through scons:

```
scons --run=dev_server
```

Or from a command line:

```
install/server --root install/web --dev
```

Open http://localhost:8080 in your browser to reach it.

### Live Deployment

_Running an internet facing server entails risks. I take no responsibility for any damage this software may cause you. I cannot provide any support._

The development run mode is not indended for deployment, as it does not support secure connections. For deployment, obtain certificates (e.g. through https://letsencrypt.org) for a domain and start the server from a command line or as a OS service:

```
install/server --root install/web --cert /path/to/server.crt --key /path/to/server.key
```

In this run mode, the server starts listening on port 443 (https) and on port 80 (http). Open https://your.domain.example.com in your browser to reach it. Unsecured connections get redirected to https automatically.

### Server Commands

Both run modes also start an http based command handler on port 6789. When deploying the server, make sure **not** to open this port to the public! Supported commands are

* **Shutdown** - http://localhost:6789/shutdown
Shuts down the server gracefully. Alternatively, send SIGTERM to the server process.
