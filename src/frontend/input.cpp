#include "input.hpp"
#include "wasm.h"

WASM_IMPORT("input", "create_mouse_observer")
int input_create_mouse_observer(void* buffer);
WASM_IMPORT("input", "destroy_mouse_observer")
void input_destroy_mouse_observer(int id);

WASM_IMPORT("input", "create_key_observer")
int input_create_key_observer(void* buffer, char const* code);
WASM_IMPORT("input", "destroy_key_observer")
void input_destroy_key_observer(int id);

namespace input {

mouse_observer::mouse_observer() {
  m_handle = ::input_create_mouse_observer(&m_data);
}

mouse_observer::~mouse_observer() {
  ::input_destroy_mouse_observer(m_handle);
}

key_observer::key_observer(char const* code) {
  m_handle = ::input_create_key_observer(&m_data, code);
}

key_observer::~key_observer() {
  ::input_destroy_key_observer(m_handle);
}

} // namespace input
