/*
 * Deskflow -- mouse and keyboard sharing utility
 * SPDX-License-Identifier: GPL-2.0-only WITH LicenseRef-OpenSSL-Exception
 */
#include "platform/OSXClipboardImageConverter.h"
#include "platform/OSXClipboardBMPConverter.h"
#include "base/Log.h"
#include <QBuffer>
#include <QImage>
#include <QTest>

class OSXClipboardImageTests : public QObject
{
  Q_OBJECT
private Q_SLOTS:
  void nativeFormatsRoundTrip()
  {
    QImage original(3, 2, QImage::Format_RGB32);
    original.fill(Qt::white);
    original.setPixelColor(0, 0, Qt::red);
    original.setPixelColor(2, 1, Qt::blue);
    QByteArray png;
    QBuffer buffer(&png);
    QVERIFY(buffer.open(QIODevice::WriteOnly));
    QVERIFY(original.save(&buffer, "PNG"));
    OSXClipboardImageConverter pngConverter;
    const auto dib = pngConverter.toIClipboard(png.toStdString());
    QVERIFY(!dib.empty());
    for (bool tiff : {false, true}) {
      OSXClipboardImageConverter converter(tiff);
      const auto native = converter.fromIClipboard(dib);
      QVERIFY(!native.empty());
      const auto roundTrip = converter.toIClipboard(native);
      const auto bmp = OSXClipboardBMPConverter().fromIClipboard(roundTrip);
      const auto decoded = QImage::fromData(QByteArray::fromStdString(bmp), "BMP");
      QCOMPARE(decoded.size(), original.size());
      for (int y = 0; y < original.height(); ++y)
        for (int x = 0; x < original.width(); ++x)
          QCOMPARE(decoded.pixelColor(x, y), original.pixelColor(x, y));
    }
  }
  void invalidImage()
  {
    OSXClipboardImageConverter converter;
    QVERIFY(converter.toIClipboard("not an image").empty());
    QVERIFY(converter.fromIClipboard("").empty());
  }
private:
  Log m_log;
};
QTEST_MAIN(OSXClipboardImageTests)
#include "OSXClipboardImageTests.moc"
