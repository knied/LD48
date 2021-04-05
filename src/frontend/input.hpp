#ifndef FRONTEND_INPUT_HPP
#define FRONTEND_INPUT_HPP

namespace input {

class mouse_observer final {
public:
  mouse_observer();
  ~mouse_observer();
  mouse_observer(mouse_observer const&) = delete;
  mouse_observer(mouse_observer&&) = delete; // could be added
  int movementX() const { return m_data.movementX; }
  int movementY() const { return m_data.movementY; }
  int clientX() const { return m_data.clientX; }
  int clientY() const { return m_data.clientY; }
  int mousedownMain() const { return m_data.mousedownMain; }
  int pressedMain() const { return m_data.pressedMain; }
  int mousedownSecond() const { return m_data.mousedownSecond; }
  int pressedSecond() const { return m_data.pressedSecond; }
private:
  struct {
    int movementX, movementY;
    int clientX, clientY;
    int mousedownMain;
    int pressedMain;
    int mousedownSecond;
    int pressedSecond;
  } m_data;
  int m_handle;
};

class key_observer final {
public:
  key_observer(char const* code);
  ~key_observer();
  key_observer(key_observer const&) = delete;
  key_observer(key_observer&&) = delete; // could be added
  int keydown() const { return m_data.keydown; }
  int pressed() const { return m_data.pressed; }
private:
  struct {
    int keydown, pressed;
  } m_data;
  int m_handle;
};

} // namespace input

#endif // FRONTEND_INPUT_HPP
