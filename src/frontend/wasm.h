#ifndef FRONTEND_WASM_H
#define FRONTEND_WASM_H

#define WASM_EXPORT(name)                                               \
  __attribute__((export_name(name), visibility("default"))) extern "C"
#define WASM_IMPORT(module,name)                                        \
  __attribute__((import_module(module), import_name(name))) extern "C"

#endif // FRONTEND_WASM_H
