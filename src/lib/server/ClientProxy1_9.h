/*
 * SPDX-FileCopyrightText: (C) 2026 Deskflow Contributors
 * SPDX-License-Identifier: GPL-2.0-only WITH LicenseRef-OpenSSL-Exception
 */
#pragma once
#include "server/ClientProxy1_8.h"
#include <set>
#include <vector>

class ClientProxy1_9 : public ClientProxy1_8
{
public:
  using ClientProxy1_8::ClientProxy1_8;
  void sendInputFocus(uint32_t epoch, bool local);
  void sendInputCapability(bool allowed);
  void sendInputHotkeys(const std::vector<uint32_t> &keys);
  bool trackHotkey(uint16_t key, uint16_t mask, bool press);

protected:
  bool parseMessage(const uint8_t *code) override;

private:
  std::set<std::pair<uint16_t, uint16_t>> m_pressedHotkeys;
};
