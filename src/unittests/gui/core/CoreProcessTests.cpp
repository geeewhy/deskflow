/*
 * Deskflow -- mouse and keyboard sharing utility
 * SPDX-License-Identifier: GPL-2.0-only WITH LicenseRef-OpenSSL-Exception
 */
#include "gui/core/CoreProcess.h"
#include "gui/ipc/DaemonIpcClient.h"
#include "gui/dialogs/SettingsDialog.h"
#include "arch/Arch.h"
#include <QCheckBox>
#include <QLocalServer>
#include <QLocalSocket>
#include <QTemporaryDir>
#include <QTest>
#include <QUuid>

namespace deskflow::gui {
class CoreProcessTests : public QObject
{
  Q_OBJECT
private Q_SLOTS:
  void initTestCase()
  {
    m_arch = std::make_unique<Arch>();
#ifdef Q_OS_WIN
    m_arch->init();
#endif
    QVERIFY(m_dir.isValid());
    Settings::setSettingsFile(m_dir.filePath("settings.conf"));
    Settings::setStateFile(m_dir.filePath("state.conf"));
  }
  void preference()
  {
    QVERIFY(Settings::defaultValue(Settings::Gui::StopOnQuit).toBool());
    ServerConfig config;
    {
      SettingsDialog dialog(nullptr, config);
      auto checkbox = dialog.findChild<QCheckBox *>("cbStopOnQuit");
      QVERIFY(checkbox);
      QVERIFY(checkbox->isChecked());
      checkbox->setChecked(false);
      static_cast<QDialog *>(&dialog)->accept();
    }
    QVERIFY(!Settings::value(Settings::Gui::StopOnQuit).toBool());
    SettingsDialog reopened(nullptr, config);
    QVERIFY(!reopened.findChild<QCheckBox *>("cbStopOnQuit")->isChecked());
    Settings::setValue(Settings::Gui::StopOnQuit, true);
  }
  void daemonShutdown_data()
  {
    QTest::addColumn<bool>("enabled");
    QTest::newRow("stop-on-quit") << true;
    QTest::newRow("keep-service-running") << false;
  }
  void daemonShutdown()
  {
    QFETCH(bool, enabled);
    Settings::setValue(Settings::Gui::StopOnQuit, enabled);
    Settings::setValue(Settings::Core::ProcessMode, Settings::ProcessMode::Service);
    QLocalServer server;
    const QString name = QUuid::createUuid().toString(QUuid::WithoutBraces);
    QVERIFY(server.listen(name));
    QByteArray received;
    connect(&server, &QLocalServer::newConnection, &server, [&] {
      auto socket = server.nextPendingConnection();
      socket->write("hello=test\n");
      socket->flush();
      connect(socket, &QLocalSocket::readyRead, &server, [&, socket] { received += socket->readAll(); });
    });
    ServerConfig config;
    CoreProcess process(config);
    delete process.m_daemonIpcClient;
    process.m_daemonIpcClient = new ipc::DaemonIpcClient(&process, name);
    process.m_processState = core::ProcessState::Started;
    process.cleanup();
    if (enabled) {
      QTRY_VERIFY(received.contains("stop\n"));
      QCOMPARE(process.processState(), core::ProcessState::Stopped);
    } else {
      QCOMPARE(process.processState(), core::ProcessState::Started);
      QVERIFY(received.isEmpty());
      QVERIFY(!server.hasPendingConnections());
    }
  }
private:
  QTemporaryDir m_dir;
  std::unique_ptr<Arch> m_arch;
};
}
QTEST_MAIN(deskflow::gui::CoreProcessTests)
#include "CoreProcessTests.moc"
