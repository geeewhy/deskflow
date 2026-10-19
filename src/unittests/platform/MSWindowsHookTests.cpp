/*
 * Deskflow -- mouse and keyboard sharing utility
 * SPDX-License-Identifier: GPL-2.0-only WITH LicenseRef-OpenSSL-Exception
 */
#include "platform/MSWindowsHook.h"
#include "base/Log.h"
#include <QTest>

namespace {
BOOL WINAPI stationaryCursor(LPPOINT point)
{
  *point = {1920, 1080};
  return TRUE;
}
BOOL WINAPI unavailableCursor(LPPOINT)
{
  return FALSE;
}
}

class MSWindowsHookTests : public QObject
{
  Q_OBJECT
private Q_SLOTS:
  void init()
  {
    MSG msg;
    PeekMessage(&msg, nullptr, WM_USER, WM_USER, PM_NOREMOVE);
    QVERIFY(m_hook.init(GetCurrentThreadId()));
    m_hook.setMode(kHOOK_RELAY_EVENTS);
  }
  void cleanup()
  {
    m_hook.setMode(kHOOK_DISABLE);
    MSG msg;
    while (PeekMessage(&msg, nullptr, DESKFLOW_MSG_INPUT_FIRST, DESKFLOW_MSG_INPUT_LAST, PM_REMOVE)) {}
  }
  void queuedForwardMotionDoesNotReverse()
  {
    // Suppressed mouse events all start at the stationary hidden cursor.
    // A burst of +6,+2,+5 must total +13, never +6,-4,+3.
    for (const int dy : {6, 2, 5})
      QVERIFY(MSWindowsHook::mouseHookHandler(WM_MOUSEMOVE, 1920, 1080 + dy, 0, stationaryCursor));
    for (const int dy : {6, 2, 5}) {
      MSG msg;
      QVERIFY(PeekMessage(&msg, nullptr, DESKFLOW_MSG_INPUT_FIRST, DESKFLOW_MSG_INPUT_LAST, PM_REMOVE));
      QCOMPARE(msg.message, UINT(DESKFLOW_MSG_MOUSE_RELATIVE));
      QCOMPARE(static_cast<int32_t>(msg.wParam), 0);
      QCOMPARE(static_cast<int32_t>(msg.lParam), dy);
    }
    MSG msg;
    QVERIFY(!PeekMessage(&msg, nullptr, DESKFLOW_MSG_INPUT_FIRST, DESKFLOW_MSG_INPUT_LAST, PM_REMOVE));
  }
  void genuineReversalAndDiagonalMotionArePreserved()
  {
    QVERIFY(MSWindowsHook::mouseHookHandler(WM_MOUSEMOVE, 1913, 1084, 0, stationaryCursor));
    QVERIFY(MSWindowsHook::mouseHookHandler(WM_MOUSEMOVE, 1923, 1078, 0, stationaryCursor));
    MSG msg;
    QVERIFY(PeekMessage(&msg, nullptr, DESKFLOW_MSG_MOUSE_RELATIVE, DESKFLOW_MSG_MOUSE_RELATIVE, PM_REMOVE));
    QCOMPARE(static_cast<int32_t>(msg.wParam), -7);
    QCOMPARE(static_cast<int32_t>(msg.lParam), 4);
    QVERIFY(PeekMessage(&msg, nullptr, DESKFLOW_MSG_MOUSE_RELATIVE, DESKFLOW_MSG_MOUSE_RELATIVE, PM_REMOVE));
    QCOMPARE(static_cast<int32_t>(msg.wParam), 3);
    QCOMPARE(static_cast<int32_t>(msg.lParam), -2);
  }
  void stationaryOrUnavailableCursorDoesNotInventMotion()
  {
    QVERIFY(MSWindowsHook::mouseHookHandler(WM_MOUSEMOVE, 1920, 1080, 0, stationaryCursor));
    QVERIFY(MSWindowsHook::mouseHookHandler(WM_MOUSEMOVE, 1920, 1086, 0, unavailableCursor));
    MSG msg;
    QVERIFY(!PeekMessage(&msg, nullptr, DESKFLOW_MSG_INPUT_FIRST, DESKFLOW_MSG_INPUT_LAST, PM_REMOVE));
  }
  void localMotionRemainsAbsolute()
  {
    m_hook.setMode(kHOOK_WATCH_JUMP_ZONE);
    m_hook.setZone(0, 0, 3840, 2160, 1);
    QVERIFY(!MSWindowsHook::mouseHookHandler(WM_MOUSEMOVE, 100, 200, 0, unavailableCursor));
    MSG msg;
    QVERIFY(PeekMessage(&msg, nullptr, DESKFLOW_MSG_INPUT_FIRST, DESKFLOW_MSG_INPUT_LAST, PM_REMOVE));
    QCOMPARE(msg.message, UINT(DESKFLOW_MSG_MOUSE_MOVE));
    QCOMPARE(static_cast<int32_t>(msg.wParam), 100);
    QCOMPARE(static_cast<int32_t>(msg.lParam), 200);
  }
private:
  Log m_log;
  MSWindowsHook m_hook;
};

QTEST_GUILESS_MAIN(MSWindowsHookTests)
#include "MSWindowsHookTests.moc"
