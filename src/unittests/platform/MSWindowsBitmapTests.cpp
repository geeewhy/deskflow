/*
 * Deskflow -- mouse and keyboard sharing utility
 * SPDX-License-Identifier: GPL-2.0-only WITH LicenseRef-OpenSSL-Exception
 */
#include "platform/MSWindowsClipboardBitmapConverter.h"
#include "base/Log.h"
#include <QTest>
#include <cstring>

class MSWindowsBitmapTests : public QObject
{
  Q_OBJECT
private Q_SLOTS:
  void extendedHeaderPixels()
  {
    BITMAPV5HEADER header{};
    header.bV5Size = sizeof(header);
    header.bV5Width = 2;
    header.bV5Height = 2;
    header.bV5Planes = 1;
    header.bV5BitCount = 32;
    header.bV5Compression = BI_BITFIELDS;
    header.bV5RedMask = 0x00ff0000;
    header.bV5GreenMask = 0x0000ff00;
    header.bV5BlueMask = 0x000000ff;
    header.bV5AlphaMask = 0xff000000;
    header.bV5CSType = LCS_sRGB;
    const DWORD pixels[]{0xffff0000, 0xff00ff00, 0xff0000ff, 0xffffffff};
    // Padding makes the old erroneous offset deterministic without an overread.
    HGLOBAL handle = GlobalAlloc(GMEM_MOVEABLE | GMEM_ZEROINIT, sizeof(header) + sizeof(pixels) + 12);
    QVERIFY(handle != nullptr);
    char *memory = static_cast<char *>(GlobalLock(handle));
    QVERIFY(memory != nullptr);
    memcpy(memory, &header, sizeof(header));
    memcpy(memory + sizeof(header), pixels, sizeof(pixels));
    GlobalUnlock(handle);
    const auto result = MSWindowsClipboardBitmapConverter().toIClipboard(handle);
    GlobalFree(handle);
    QCOMPARE(result.size(), sizeof(BITMAPINFOHEADER) + sizeof(pixels));
    for (size_t i = 0; i < 4; ++i) {
      DWORD pixel;
      memcpy(&pixel, result.data() + sizeof(BITMAPINFOHEADER) + i * sizeof(DWORD), sizeof(pixel));
      QCOMPARE(pixel & 0xffffff, pixels[i] & 0xffffff);
    }
  }
private:
  Log m_log;
};
QTEST_MAIN(MSWindowsBitmapTests)
#include "MSWindowsBitmapTests.moc"
