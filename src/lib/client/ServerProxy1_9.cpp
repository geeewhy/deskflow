/*
 * SPDX-FileCopyrightText: (C) 2026 Deskflow Contributors
 * SPDX-License-Identifier: GPL-2.0-only WITH LicenseRef-OpenSSL-Exception
 */
#include "client/ServerProxy1_9.h"
#include "client/Client.h"
#include "deskflow/ProtocolTypes.h"
#include "deskflow/ProtocolUtil.h"
#include <cstring>

ServerProxy1_9::ServerProxy1_9(Client *client, deskflow::IStream *stream, IEventQueue *events)
    : ServerProxy1_8(client, stream, events),
      m_client(client)
{
}

bool ServerProxy1_9::parseExtension(const uint8_t *code)
{
  if (memcmp(code, kMsgDBidirectionalHotkeys, 4) == 0) {
    std::vector<uint32_t> keys;
    ProtocolUtil::readf(getStream(), kMsgDBidirectionalHotkeys + 4, &keys);
    m_client->setInputHotkeys(keys);
    return true;
  }
  if (memcmp(code, kMsgDBidirectionalCapability, 4) == 0) {
    uint8_t allowed;
    ProtocolUtil::readf(getStream(), kMsgDBidirectionalCapability + 4, &allowed);
    m_client->setBidirectionalAllowed(allowed == 1);
    return true;
  }
  if (memcmp(code, kMsgDBidirectionalFocus, 4) == 0) {
    uint32_t epoch;
    uint8_t local;
    ProtocolUtil::readf(getStream(), kMsgDBidirectionalFocus + 4, &epoch, &local);
    m_client->setInputFocus(epoch, local == 1);
    return true;
  }
  return false;
}

ServerProxy::ConnectionResult ServerProxy1_9::parseHandshakeMessage(const uint8_t *code)
{
  return parseExtension(code) ? ConnectionResult::Okay : ServerProxy1_8::parseHandshakeMessage(code);
}
ServerProxy::ConnectionResult ServerProxy1_9::parseMessage(const uint8_t *code)
{
  return parseExtension(code) ? ConnectionResult::Okay : ServerProxy1_8::parseMessage(code);
}
void ServerProxy1_9::sendInput(const deskflow::SharedInput &input)
{
  ProtocolUtil::writef(
      getStream(), kMsgDBidirectionalInput, input.epoch, static_cast<uint8_t>(input.kind), input.key, input.mask,
      input.button, input.count, input.x, input.y, &input.language
  );
}
