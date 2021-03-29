# Instructions

## Dependencies

```
brew install scons
brew install libev
brew install libressl
brew install llvm
```

## Extra WASM/WASI Dependencies

Download prebuilt **sysroot** and **clang_rt** from [wasi-sdk](https://github.com/WebAssembly/wasi-sdk/releases)

Extend Homebrew llvm installation to support wasm32:
```
mkdir -p /usr/local/opt/llvm/lib/clang/<version>/lib/wasi
cp libclang_rt.builtins-wasm32.a  /usr/local/opt/llvm/lib/clang/<version>/lib/wasi/
```

## Environment Setup

```:template-setup.sh

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

Launch through scons:

```
scons --run
```

Or from a command prompt:

```
install/server --root install/web --cert /path/to/server.crt --key /path/to/server.key
```
