/*
 * Deskflow -- mouse and keyboard sharing utility
 * SPDX-License-Identifier: GPL-2.0-only WITH LicenseRef-OpenSSL-Exception
 */
#pragma once
#include <QStringList>

namespace MonitorInput {
// LG proprietary input selection, verified on the 27BK67U via NVIDIA DDC.
inline QStringList arguments(int monitor, const QString &source)
{
  if (monitor < 0 || monitor > 31 || (source != "hdmi1" && source != "usb-c"))
    return {};
  return {QString::number(monitor), source == "usb-c" ? "0xD1" : "0x90", "0xF4", "0x50"};
}
inline QString encodedPath(const QString &path) { return QString::fromLatin1(path.toUtf8().toHex()); }
inline QString decodedPath(const QString &encoded)
{
  const auto path = QString::fromUtf8(QByteArray::fromHex(encoded.toLatin1()));
  return encodedPath(path) == encoded.toLower() ? path : QString();
}
} // namespace MonitorInput
