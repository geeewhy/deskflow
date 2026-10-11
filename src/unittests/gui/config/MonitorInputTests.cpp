/*
 * Deskflow -- mouse and keyboard sharing utility
 * SPDX-License-Identifier: GPL-2.0-only WITH LicenseRef-OpenSSL-Exception
 */
#include "arch/Arch.h"
#include "common/Action.h"
#include "common/Hotkey.h"
#include "common/MonitorInput.h"
#include "common/Settings.h"
#include "gui/config/ServerConfig.h"
#include "gui/dialogs/ActionDialog.h"
#include "server/Config.h"
#include <QComboBox>
#include <QDialogButtonBox>
#include <QLineEdit>
#include <QPushButton>
#include <QSpinBox>
#include <QTemporaryDir>
#include <QTest>
#include <sstream>

class MonitorInputTests : public QObject
{
  Q_OBJECT
private Q_SLOTS:
  void initTestCase()
  {
    m_arch = std::make_unique<Arch>();
#ifdef Q_OS_WIN
    m_arch->init();
#endif
    Settings::setSettingsFile(m_directory.filePath("Deskflow.conf"));
    Settings::setStateFile(m_directory.filePath("Deskflow.state"));
    Settings::setValue(Settings::Core::ComputerName, "host");
    Settings::setValue(Settings::Computer::Name.arg("host"), "host");
    Settings::setValue(Settings::Computer::Name.arg("Mac"), "Mac");
  }
  void verifiedCommandsAndPathEncoding()
  {
    QCOMPARE(MonitorInput::arguments(0, "usb-c"), QStringList({"0", "0xD1", "0xF4", "0x50"}));
    QCOMPARE(MonitorInput::arguments(1, "hdmi1"), QStringList({"1", "0x90", "0xF4", "0x50"}));
    QVERIFY(MonitorInput::arguments(-1, "usb-c").isEmpty());
    QVERIFY(MonitorInput::arguments(32, "usb-c").isEmpty());
    QVERIFY(MonitorInput::arguments(0, "unknown").isEmpty());
    const QString path = QString::fromUtf8("C:/tools with spaces/é,(input)/writeValueToDisplay.exe");
    QCOMPARE(MonitorInput::decodedPath(MonitorInput::encodedPath(path)), path);
    QVERIFY(MonitorInput::decodedPath("not-hex").isEmpty());
  }
  void persistenceAndCoreRoundTrip()
  {
    Action action;
    action.setType(static_cast<int>(Action::Type::switchMonitorInput));
    action.monitorIndex = 2;
    action.monitorSource = "hdmi1";
    action.monitorTool = "C:/tools with spaces/monitor,(control)/writeValueToDisplay.exe";
    action.monitorComputer = "Mac";
    action.setActiveOnRelease(true);
    QSettings settings(m_directory.filePath("action.conf"), QSettings::IniFormat);
    action.saveSettings(settings);
    Action restored;
    restored.loadSettings(settings);
    QCOMPARE(restored.text(), action.text());
    QCOMPARE(restored.monitorComputer, action.monitorComputer);
    QVERIFY(restored.activeOnRelease());
    InputFilter::SwitchMonitorInputAction coreAction(nullptr, 2, "hdmi1", action.monitorTool.toStdString(), "Mac");
    QCOMPARE(coreAction.format(), action.text().toStdString());
    std::unique_ptr<InputFilter::Action> clone(coreAction.clone());
    QCOMPARE(clone->format(), coreAction.format());
    Hotkey hotkey;
    KeySequence sequence;
    sequence.appendKey(Qt::Key_F11, 0);
    hotkey.setKeySequence(sequence);
    hotkey.addAction(action);
    QString text;
    QTextStream output(&text);
    output << "section: links\n host:\n Mac:\nend\nsection: options\n" << hotkey << "end\n";
    QVERIFY(text.contains(" = ;switchMonitorInput"));
    std::istringstream input(text.toStdString());
    deskflow::server::Config config(nullptr);
    input >> config;
    std::ostringstream roundTrip;
    roundTrip << config;
    QVERIFY(QString::fromStdString(roundTrip.str()).contains(action.text()));
  }
  void editorValidationAndSave()
  {
#ifdef Q_OS_WIN
    ServerConfig config;
    config.addClient("Mac");
    Hotkey hotkey;
    Action action;
    ActionDialog dialog(nullptr, config, hotkey, action);
    auto type = dialog.findChild<QComboBox *>("comboActionType");
    type->setCurrentIndex(type->findData(static_cast<int>(Action::Type::switchMonitorInput)));
    auto tool = dialog.findChild<QLineEdit *>("monitorTool");
    auto buttons = dialog.findChild<QDialogButtonBox *>("buttonBox");
    tool->setText(m_directory.filePath("missing.exe"));
    QVERIFY(!buttons->button(QDialogButtonBox::Ok)->isEnabled());
    const auto validPath = m_directory.filePath("helper with spaces.exe");
    QFile file(validPath);
    QVERIFY(file.open(QFile::WriteOnly));
    file.close();
    tool->setText(validPath);
    dialog.findChild<QSpinBox *>("monitorIndex")->setValue(3);
    auto source = dialog.findChild<QComboBox *>("monitorSource");
    source->setCurrentIndex(source->findData("hdmi1"));
    auto computer = dialog.findChild<QComboBox *>("monitorComputer");
    computer->setCurrentIndex(computer->findData("Mac"));
    QVERIFY(buttons->button(QDialogButtonBox::Ok)->isEnabled());
    buttons->button(QDialogButtonBox::Ok)->click();
    QCOMPARE(action.type(), static_cast<int>(Action::Type::switchMonitorInput));
    QCOMPARE(action.monitorIndex, 3);
    QCOMPARE(action.monitorSource, "hdmi1");
    QCOMPARE(action.monitorTool, validPath);
    QCOMPARE(action.monitorComputer, "Mac");
#endif
  }
private:
  QTemporaryDir m_directory;
  std::unique_ptr<Arch> m_arch;
};

QTEST_MAIN(MonitorInputTests)
#include "MonitorInputTests.moc"
