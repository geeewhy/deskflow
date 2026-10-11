/*
 * SPDX-FileCopyrightText: (C) 2026 Deskflow Contributors
 * SPDX-License-Identifier: GPL-2.0-only WITH LicenseRef-OpenSSL-Exception
 */
#include "server/ClientProxy1_9.h"
#include "deskflow/DeskflowException.h"
#include "deskflow/ProtocolTypes.h"
#include "deskflow/ProtocolUtil.h"
#include "server/Server.h"
#include <cstring>

void ClientProxy1_9::sendInputCapability(bool allowed)
{
  ProtocolUtil::writef(getStream(), kMsgDBidirectionalCapability, allowed ? 1 : 0);
}
void ClientProxy1_9::sendInputFocus(uint32_t epoch, bool local)
{
  ProtocolUtil::writef(getStream(), kMsgDBidirectionalFocus, epoch, local ? 1 : 0);
}
void ClientProxy1_9::sendInputHotkeys(const std::vector<uint32_t> &keys)
{
  ProtocolUtil::writef(getStream(), kMsgDBidirectionalHotkeys, &keys);
}
bool ClientProxy1_9::trackHotkey(uint16_t key, uint16_t mask, bool press)
{
  auto pair = std::make_pair(key, mask);
  if (press && m_pressedHotkeys.size() >= 256)
    return false;
  if (press)
    return m_pressedHotkeys.insert(pair).second;
  return m_pressedHotkeys.erase(pair) != 0;
}
bool ClientProxy1_9::parseMessage(const uint8_t *code)
{
  if (memcmp(code, kMsgDBidirectionalInput, 4) != 0)
    return ClientProxy1_8::parseMessage(code);
  deskflow::SharedInput input;
  uint8_t kind;
  ProtocolUtil::readf(
      getStream(), kMsgDBidirectionalInput + 4, &input.epoch, &kind, &input.key, &input.mask, &input.button,
      &input.count, &input.x, &input.y, &input.language
  );
  input.kind = static_cast<deskflow::SharedInput::Kind>(kind);
  if (!input.valid())
    throw BadClientException("Invalid shared input");
  m_server->receiveInput(this, input);
  return true;
}
