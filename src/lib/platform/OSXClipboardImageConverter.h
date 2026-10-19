/*
 * Deskflow -- mouse and keyboard sharing utility
 * SPDX-License-Identifier: GPL-2.0-only WITH LicenseRef-OpenSSL-Exception
 */
#pragma once
#include "platform/OSXClipboard.h"

class OSXClipboardImageConverter : public IOSXClipboardConverter
{
public:
  explicit OSXClipboardImageConverter(bool tiff = false) : m_tiff(tiff) {}
  IClipboard::Format getFormat() const override { return IClipboard::Format::Bitmap; }
  CFStringRef getOSXFormat() const override { return m_tiff ? CFSTR("public.tiff") : CFSTR("public.png"); }
  std::string fromIClipboard(const std::string &) const override;
  std::string toIClipboard(const std::string &) const override;
private:
  bool m_tiff;
};
