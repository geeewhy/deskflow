/*
 * SPDX-FileCopyrightText: (C) 2026 Deskflow Contributors
 * SPDX-License-Identifier: GPL-2.0-only WITH LicenseRef-OpenSSL-Exception
 */
#pragma once

#include <cstdint>
#include <map>
#include <set>
#include <string>

namespace deskflow {

// Also used as dwExtraInfo / kCGEventSourceUserData to identify our own injections.
inline constexpr uint32_t kInputEventTag = 0x44464c57;

struct SharedInput
{
  enum class Kind : uint8_t
  {
    KeyDown,
    KeyUp,
    KeyRepeat,
    ButtonDown,
    ButtonUp,
    AbsoluteMotion,
    RelativeMotion,
    Wheel,
    HotkeyDown,
    HotkeyUp
  };
  uint32_t epoch = 0;
  Kind kind = Kind::AbsoluteMotion;
  uint16_t key = 0;
  uint16_t mask = 0;
  uint16_t button = 0;
  uint16_t count = 1;
  int32_t x = 0;
  int32_t y = 0;
  std::string language;

  bool valid() const
  {
    if (language.size() > 128)
      return false;
    switch (kind) {
    case Kind::HotkeyDown:
    case Kind::HotkeyUp:
      return true;
    case Kind::KeyDown:
    case Kind::KeyUp:
      return button != 0;
    case Kind::KeyRepeat:
      return button != 0 && count > 0 && count <= 100;
    case Kind::ButtonDown:
    case Kind::ButtonUp:
      return button >= 1 && button <= 5;
    case Kind::AbsoluteMotion:
      return x >= -32768 && x <= 32767 && y >= -32768 && y <= 32767;
    case Kind::RelativeMotion:
    case Kind::Wheel:
      return x >= -32768 && x <= 32767 && y >= -32768 && y <= 32767;
    }
    return false;
  }
};

// A keyboard chord or drag belongs to one physical source until released.
// Focus changes/disconnects release the recorded inputs before clearing them.
class InputOwnership
{
public:
  bool accept(const std::string &source, const SharedInput &input)
  {
    if (!input.valid() || ((!m_keys.empty() || !m_buttons.empty()) && m_source != source))
      return false;
    using enum SharedInput::Kind;
    switch (input.kind) {
    case KeyDown:
      if (m_keys.contains(input.button) || m_keys.size() >= 512)
        return false;
      m_keys.emplace(input.button, input);
      break;
    case KeyUp:
      if (m_keys.erase(input.button) == 0)
        return false;
      break;
    case KeyRepeat:
      if (!m_keys.contains(input.button))
        return false;
      break;
    case ButtonDown:
      if (!m_buttons.insert(input.button).second)
        return false;
      break;
    case ButtonUp:
      if (m_buttons.erase(input.button) == 0)
        return false;
      break;
    default:
      break;
    }
    m_source = source;
    return true;
  }
  bool held() const
  {
    return !m_keys.empty() || !m_buttons.empty();
  }
  const std::string &source() const
  {
    return m_source;
  }
  const auto &keys() const
  {
    return m_keys;
  }
  const auto &buttons() const
  {
    return m_buttons;
  }
  void clear()
  {
    m_keys.clear();
    m_buttons.clear();
    m_source.clear();
  }

private:
  std::string m_source;
  std::map<uint16_t, SharedInput> m_keys;
  std::set<uint16_t> m_buttons;
};
} // namespace deskflow
