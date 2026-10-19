/*
 * Deskflow -- mouse and keyboard sharing utility
 * SPDX-FileCopyrightText: (C) 2013 - 2016 Synergy App Ltd
 * SPDX-License-Identifier: GPL-2.0-only WITH LicenseRef-OpenSSL-Exception
 */

#pragma once

#import <CoreFoundation/CoreFoundation.h>

#if defined(__cplusplus)
extern "C"
{
#endif
  void runCocoaApp();
  void stopCocoaLoop();
  void showScreenEntryCue(double x, double y);
  void hideScreenEntryCue();

#if defined(__cplusplus)
}
#endif
