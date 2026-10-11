/*
 * SPDX-FileCopyrightText: (C) 2026 Deskflow Contributors
 * SPDX-License-Identifier: GPL-2.0-only WITH LicenseRef-OpenSSL-Exception
 */
#pragma once
#include "client/ServerProxy1_8.h"
#include "deskflow/BidirectionalInput.h"

class ServerProxy1_9 : public ServerProxy1_8
{
public:
  ServerProxy1_9(Client *client, deskflow::IStream *stream, IEventQueue *events);
  void sendInput(const deskflow::SharedInput &input);

protected:
  ConnectionResult parseHandshakeMessage(const uint8_t *code) override;
  ConnectionResult parseMessage(const uint8_t *code) override;

private:
  bool parseExtension(const uint8_t *code);
  Client *m_client;
};
