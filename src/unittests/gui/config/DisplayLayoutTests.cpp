/*
 * Deskflow -- mouse and keyboard sharing utility
 * SPDX-License-Identifier: GPL-2.0-only WITH LicenseRef-OpenSSL-Exception
 */
#include "common/Settings.h"
#include "arch/Arch.h"
#include "gui/config/ServerConfig.h"
#include "gui/widgets/DisplayLayoutWidget.h"
#include "server/Config.h"
#include <QCheckBox>
#include <QComboBox>
#include <QDoubleSpinBox>
#include <QPushButton>
#include <QTemporaryDir>
#include <QTest>
#include <sstream>

class DisplayLayoutTests : public QObject
{
  Q_OBJECT
private Q_SLOTS:
  void initTestCase()
  {
    m_arch = std::make_unique<Arch>();
#ifdef Q_OS_WIN
    m_arch->init();
#endif
  }
  void partialEdges()
  {
    const DisplayLayout::Rectangles rectangles{{"LG", {0, 0, 600, 340}}, {"Mac", {200, 340, 300, 200}}};
    const auto links = DisplayLayout::links(rectangles);
    QCOMPARE(links.size(), 2);
    QCOMPARE(links[0].direction, "down");
    QVERIFY(qAbs(links[0].sourceStart - 100.0 / 3) < 0.00001);
    QVERIFY(qAbs(links[0].sourceEnd - 250.0 / 3) < 0.00001);
    QCOMPARE(links[0].targetStart, 0.0);
    QCOMPARE(links[0].targetEnd, 100.0);
    QCOMPARE(links[1].direction, "up");
    QVERIFY(DisplayLayout::validationError(rectangles).isEmpty());
  }
  void multipleNeighborsAndNegativeCoordinates()
  {
    const DisplayLayout::Rectangles rectangles{
        {"host", {-600, -400, 600, 400}}, {"upper", {0, -400, 300, 200}}, {"lower", {0, -200, 300, 200}}};
    const auto links = DisplayLayout::links(rectangles);
    QCOMPARE(links.size(), 6);
    QVERIFY(DisplayLayout::validationError(rectangles).isEmpty());
    QList<DisplayLayout::Link> hostLinks;
    for (const auto &link : links)
      if (link.source == "host")
        hostLinks.append(link);
    QCOMPARE(hostLinks.size(), 2);
    QCOMPARE(hostLinks[0].direction, "right");
    QVERIFY(hostLinks[0].sourceEnd <= hostLinks[1].sourceStart ||
            hostLinks[1].sourceEnd <= hostLinks[0].sourceStart);
  }
  void invalidArrangements()
  {
    QVERIFY(!DisplayLayout::validationError({{"a", {0, 0, 100, 100}}, {"b", {99, 0, 100, 100}}}).isEmpty());
    QVERIFY(!DisplayLayout::validationError({{"a", {0, 0, 100, 100}}, {"b", {101, 0, 100, 100}}}).isEmpty());
    const DisplayLayout::Rectangles corner{{"a", {0, 0, 100, 100}}, {"b", {100, 100, 100, 100}}};
    QVERIFY(DisplayLayout::links(corner).isEmpty());
    QVERIFY(!DisplayLayout::validationError(corner).isEmpty());
    QVERIFY(!DisplayLayout::validationError({{"a", {0, 0, 0, 100}}}).isEmpty());
  }
  void persistenceAndCoreMapping()
  {
    QTemporaryDir directory;
    QVERIFY(directory.isValid());
    Settings::setSettingsFile(directory.filePath("Deskflow.conf"));
    Settings::setStateFile(directory.filePath("Deskflow.state"));
    Settings::setValue(Settings::Core::ComputerName, "LG");
    ServerConfig config;
    config.addClient("Mac");
    config.useDisplayLayout = true;
    config.displayLayout = {{"LG", {0, 0, 600, 340}}, {"Mac", {200, 340, 300, 200}}};
    config.commit();
    ServerConfig restored;
    QVERIFY(restored.useDisplayLayout);
    QCOMPARE(restored.displayLayout, config.displayLayout);
    QString serialized;
    QTextStream output(&serialized);
    Settings::setValue(Settings::Computer::Name.arg("LG"), "LG");
    Settings::setValue(Settings::Computer::Name.arg("Mac"), "Mac");
    output << restored;
    std::istringstream input(serialized.toStdString());
    deskflow::server::Config core(nullptr);
    input >> core;
    float position = 0;
    QCOMPARE(core.getNeighbor("LG", Direction::Bottom, 0.2f, &position), std::string());
    QCOMPARE(core.getNeighbor("LG", Direction::Bottom, 0.5f, &position), std::string("Mac"));
    QVERIFY(qAbs(position - 1.0f / 3) < 0.00001f);
    QCOMPARE(core.getNeighbor("Mac", Direction::Top, position, &position), std::string("LG"));
    QVERIFY(qAbs(position - 0.5f) < 0.00001f);
    restored.useDisplayLayout = false;
    QVERIFY(!(config == restored));
  }
  void editorAndCancelIsolation()
  {
    QTemporaryDir directory;
    Settings::setSettingsFile(directory.filePath("Deskflow.conf"));
    Settings::setStateFile(directory.filePath("Deskflow.state"));
    Settings::setValue(Settings::Core::ComputerName, "LG");
    ServerConfig original;
    original.addClient("Mac");
    auto editable = original;
    DisplayLayoutWidget widget(editable);
    QVERIFY(editable == original); // Opening the editor must not alter the configuration.
    widget.findChild<QCheckBox *>()->setChecked(true);
    QVERIFY(editable.useDisplayLayout);
    QCOMPARE(editable.displayLayout.size(), 2);
    widget.findChild<QComboBox *>()->setCurrentText("Mac");
    const auto fields = widget.findChildren<QDoubleSpinBox *>();
    QCOMPARE(fields.size(), 4);
    fields[2]->setValue(300);
    fields[3]->setValue(200);
    QCOMPARE(editable.displayLayout.value("Mac").size(), QSizeF(300, 200));
    editable.reportedDisplaySizes.insert("Mac", QSize(3008, 2809));
    widget.refresh();
    auto reported = widget.findChild<QPushButton *>("useReportedSize");
    QVERIFY(reported->isEnabled());
    reported->click();
    QCOMPARE(editable.displayLayout.value("Mac").size(), QSizeF(3008, 2809));
    QVERIFY(!original.useDisplayLayout);
    QVERIFY(original.displayLayout.isEmpty());
    editable = original;
    widget.refresh();
    QVERIFY(!widget.findChild<QCheckBox *>()->isChecked());
    QVERIFY(editable == original);
  }
private:
  std::unique_ptr<Arch> m_arch;
};

QTEST_MAIN(DisplayLayoutTests)
#include "DisplayLayoutTests.moc"
