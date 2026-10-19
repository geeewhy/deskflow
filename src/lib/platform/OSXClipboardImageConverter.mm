/*
 * Deskflow -- mouse and keyboard sharing utility
 * SPDX-License-Identifier: GPL-2.0-only WITH LicenseRef-OpenSSL-Exception
 */
#include "platform/OSXClipboardImageConverter.h"
#include "platform/OSXClipboardBMPConverter.h"
#include <QBuffer>
#include <QImage>
#import <AppKit/AppKit.h>

std::string OSXClipboardImageConverter::fromIClipboard(const std::string &dib) const
{
  @autoreleasepool {
    const auto bmp = OSXClipboardBMPConverter().fromIClipboard(dib);
    NSData *data = [NSData dataWithBytes:bmp.data() length:bmp.size()];
    NSBitmapImageRep *image = [NSBitmapImageRep imageRepWithData:data];
    NSData *encoded = [image representationUsingType:m_tiff ? NSBitmapImageFileTypeTIFF : NSBitmapImageFileTypePNG
                                        properties:@{}];
    if (!encoded)
      return {};
    return std::string(static_cast<const char *>(encoded.bytes), encoded.length);
  }
}

std::string OSXClipboardImageConverter::toIClipboard(const std::string &data) const
{
  @autoreleasepool {
    NSBitmapImageRep *native = [NSBitmapImageRep imageRepWithData:[NSData dataWithBytes:data.data() length:data.size()]];
    NSData *png = [native representationUsingType:NSBitmapImageFileTypePNG properties:@{}];
    if (!png)
      return {};
    // Native decoding handles macOS TIFF without a separately deployed Qt TIFF
    // plugin. Keep the existing DIB wire format for older Windows clients.
    QImage image = QImage::fromData(static_cast<const uchar *>(png.bytes), png.length, "PNG");
    if (image.isNull())
      return {};
    QByteArray bmp;
    QBuffer buffer(&bmp);
    if (!buffer.open(QIODevice::WriteOnly) || !image.save(&buffer, "BMP"))
      return {};
    return OSXClipboardBMPConverter().toIClipboard(bmp.toStdString());
  }
}
