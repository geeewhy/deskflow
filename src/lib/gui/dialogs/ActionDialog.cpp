/*
 * Deskflow -- mouse and keyboard sharing utility
 * SPDX-FileCopyrightText: (C) 2025 Chris Rizzitello <sithlord48@gmail.com>
 * SPDX-FileCopyrightText: (C) 2012 - 2016 Synergy App Ltd
 * SPDX-FileCopyrightText: (C) 2008 Volker Lanz <vl@fidra.de>
 * SPDX-License-Identifier: GPL-2.0-only WITH LicenseRef-OpenSSL-Exception
 */

#include "ActionDialog.h"
#include "ui_ActionDialog.h"

#include "common/Action.h"
#include "common/Hotkey.h"
#include "common/KeySequence.h"
#include "config/ServerConfig.h"

#include <QTimer>
#include <QCoreApplication>
#include <QDir>
#include <QFileDialog>
#include <QFileInfo>
#include <QFormLayout>
#include <QLabel>
#include <QLineEdit>
#include <QSpinBox>
#include <QVBoxLayout>

ActionDialog::ActionDialog(QWidget *parent, const ServerConfig &config, Hotkey &hotkey, Action &action)
    : QDialog(parent, Qt::WindowTitleHint | Qt::WindowSystemMenuHint),
      ui{std::make_unique<Ui::ActionDialog>()},
      m_hotkey(hotkey),
      m_action(action)
{
  ui->setupUi(this);
  for (int i = 0; i < ui->comboActionType->count(); ++i)
    ui->comboActionType->setItemData(i, i);
  ui->comboActionType->addItem(tr("Switch monitor input (LG)"), static_cast<int>(Action::Type::switchMonitorInput));
  m_monitorControls = new QWidget(this);
  auto monitorForm = new QFormLayout(m_monitorControls);
  m_monitorIndex = new QSpinBox(this);
  m_monitorIndex->setObjectName("monitorIndex");
  m_monitorIndex->setRange(0, 31);
  m_monitorIndex->setValue(m_action.monitorIndex);
  m_monitorSource = new QComboBox(this);
  m_monitorSource->setObjectName("monitorSource");
  m_monitorSource->addItem(tr("HDMI 1"), "hdmi1");
  m_monitorSource->addItem(tr("USB-C"), "usb-c");
  m_monitorSource->setCurrentIndex(m_monitorSource->findData(m_action.monitorSource));
  m_monitorComputer = new QComboBox(this);
  m_monitorComputer->setObjectName("monitorComputer");
  m_monitorComputer->addItem(tr("Keep keyboard and mouse on current computer"), QString());
  for (const auto &computer : config.computers())
    if (!computer.isNull())
      m_monitorComputer->addItem(computer.name(), computer.name());
  m_monitorComputer->setCurrentIndex(qMax(0, m_monitorComputer->findData(m_action.monitorComputer)));
  m_monitorTool = new QLineEdit(this);
  m_monitorTool->setObjectName("monitorTool");
  auto tool = m_action.monitorTool;
  if (tool.isEmpty()) {
    tool = QCoreApplication::applicationDirPath() + "/writeValueToDisplay.exe";
    if (!QFileInfo::exists(tool))
      tool = QDir::homePath() + "/tools/monitor-control/writeValueToDisplay.exe";
  }
  m_monitorTool->setText(tool);
  auto toolRow = new QWidget(this);
  auto toolLayout = new QHBoxLayout(toolRow);
  toolLayout->setContentsMargins(0, 0, 0, 0);
  toolLayout->addWidget(m_monitorTool);
  auto browse = new QPushButton(tr("Browse…"), this);
  toolLayout->addWidget(browse);
  connect(browse, &QPushButton::clicked, this, [this] {
    const auto path = QFileDialog::getOpenFileName(this, tr("Select writeValueToDisplay.exe"),
                                                 m_monitorTool->text(), tr("Executable (*.exe)"));
    if (!path.isEmpty())
      m_monitorTool->setText(path);
  });
  connect(m_monitorTool, &QLineEdit::textChanged, this, [this] {
    ui->buttonBox->button(QDialogButtonBox::Ok)->setEnabled(canSave());
  });
  monitorForm->addRow(tr("Monitor index (starts at 0)"), m_monitorIndex);
  monitorForm->addRow(tr("Input"), m_monitorSource);
  monitorForm->addRow(tr("Keyboard and mouse"), m_monitorComputer);
  monitorForm->addRow(tr("Control utility"), toolRow);
  auto note = new QLabel(tr("Uses LG proprietary DDC commands, verified on the 27BK67U. Requires a Windows server. "
                           "The utility runs on the server even while its keyboard and mouse control a client."), this);
  note->setWordWrap(true);
  monitorForm->addRow(note);
  static_cast<QVBoxLayout *>(layout())->insertWidget(2, m_monitorControls);
  connect(ui->keySequenceWidget, &KeySequenceWidget::keySequenceChanged, this, &ActionDialog::keySequenceChanged);
  connect(
      ui->comboActionType, QOverload<int>::of(&QComboBox::currentIndexChanged), this, &ActionDialog::actionTypeChanged
  );
  connect(ui->listComputers, &QListWidget::itemChanged, this, &ActionDialog::itemToggled);
  connect(ui->buttonBox, &QDialogButtonBox::accepted, this, &ActionDialog::accept);
  connect(ui->buttonBox, &QDialogButtonBox::rejected, this, &ActionDialog::reject);

  ui->keySequenceWidget->setText(m_action.keySequence().toString());
  ui->keySequenceWidget->setKeySequence(m_action.keySequence());

  ui->comboSwitchInDirection->setCurrentIndex(m_action.switchDirection());
  ui->comboLockCursorToComputer->setCurrentIndex(m_action.lockCursorMode());

  ui->comboActionType->setCurrentIndex(ui->comboActionType->findData(m_action.type()));
  ui->comboTriggerOn->setCurrentIndex(m_action.activeOnRelease());

  for (const Computer &computer : config.computers()) {
    if (computer.isNull())
      continue;
    auto *newListItem = new QListWidgetItem(computer.name());
    newListItem->setCheckState(Qt::Checked);
    if ((m_action.typeComputerNames().indexOf(computer.name()) == -1) &&
        (m_action.haveComputers() && !m_action.typeComputerNames().isEmpty()))
      newListItem->setCheckState(Qt::Unchecked);

    ui->listComputers->addItem(newListItem);

    ui->comboSwitchToComputer->addItem(tr("Switch to %1").arg(computer.name()), computer.name());
    if (computer.name() == m_action.switchComputerName())
      ui->comboSwitchToComputer->setCurrentIndex(ui->comboSwitchToComputer->count() - 1);
  }

  ui->keySequenceWidget->setVisible(false);
  ui->groupComputers->setVisible(false);
  ui->listComputers->setEnabled(!ui->keySequenceWidget->keySequence().isMouseButton());
  ui->comboSwitchToComputer->setVisible(false);
  ui->comboSwitchInDirection->setVisible(false);
  ui->comboLockCursorToComputer->setVisible(false);

  actionTypeChanged(ui->comboActionType->currentIndex());
}

void ActionDialog::accept()
{
  if (!canSave())
    return;

  m_action.setKeySequence(ui->keySequenceWidget->keySequence());
  m_action.setType(ui->comboActionType->currentData().toInt());
  m_action.monitorIndex = m_monitorIndex->value();
  m_action.monitorSource = m_monitorSource->currentData().toString();
  m_action.monitorComputer = m_monitorComputer->currentData().toString();
  m_action.monitorTool = m_monitorTool->text();

  m_action.clearComputers();

  int computerCount = ui->listComputers->count();

  for (int i = 0; i < ui->listComputers->count(); i++) {
    const auto &item = ui->listComputers->item(i);
    m_action.addComputer(item->text());
    if (item->checkState() == Qt::Unchecked) {
      computerCount--;
      m_action.removeComputer(item->text());
    }
  }

  if (computerCount == ui->listComputers->count())
    m_action.clearComputers();

  m_action.setHaveComputers(computerCount);

  m_action.setSwitchComputerName(ui->comboSwitchToComputer->currentData().toString());
  m_action.setSwitchDirection(ui->comboSwitchInDirection->currentIndex());
  m_action.setLockCursorMode(ui->comboLockCursorToComputer->currentIndex());
  m_action.setActiveOnRelease(ui->comboTriggerOn->currentIndex());
  m_action.setRestartServer(ui->comboActionType->currentIndex() == ActionTypes::RestartServer);

  QDialog::accept();
}

void ActionDialog::updateSize()
{
  setMaximumSize(QSize(16777215, 1677215));
  adjustSize();
  if (!isKeyAction(ui->comboActionType->currentIndex()))
    setMaximumSize(size());
}

void ActionDialog::keySequenceChanged()
{
  ui->listComputers->setEnabled(!ui->keySequenceWidget->keySequence().isMouseButton());
  ui->buttonBox->button(QDialogButtonBox::Ok)->setEnabled(canSave());
}

void ActionDialog::itemToggled() const
{
  ui->buttonBox->button(QDialogButtonBox::Ok)->setEnabled(canSave());
}

void ActionDialog::actionTypeChanged(int index)
{
  m_monitorControls->setVisible(ui->comboActionType->currentData().toInt() == static_cast<int>(Action::Type::switchMonitorInput));
  ui->buttonBox->button(QDialogButtonBox::Ok)->setEnabled(canSave());
  ui->keySequenceWidget->setVisible(isKeyAction(index));
  ui->groupComputers->setVisible(isKeyAction(index));
  ui->listComputers->setEnabled(!ui->keySequenceWidget->keySequence().isMouseButton());
  ui->comboSwitchToComputer->setVisible(index == ActionTypes::SwitchTo);
  ui->comboSwitchInDirection->setVisible(index == ActionTypes::SwitchInDirection);
  ui->comboLockCursorToComputer->setVisible(index == ActionTypes::ModifyCursorLock);
  QTimer::singleShot(1, this, &ActionDialog::updateSize);
}

bool ActionDialog::isKeyAction(int index) const
{
  return ((index == ActionTypes::PressKey) || (index == ActionTypes::ReleaseKey) || (index == ActionTypes::ToggleKey));
}

bool ActionDialog::canSave() const
{
  if (ui->comboActionType->currentData().toInt() == static_cast<int>(Action::Type::switchMonitorInput)) {
#ifdef Q_OS_WIN
    return m_monitorTool && QFileInfo(m_monitorTool->text()).isFile();
#else
    return false;
#endif
  }
  if (isKeyAction(ui->comboActionType->currentIndex())) {
    const QList<QListWidgetItem *> items = ui->listComputers->findItems("*", Qt::MatchWildcard);
    int totalChecked = 0;
    for (const auto &item : items) {
      if (item->checkState() == Qt::Checked)
        totalChecked++;
    }
    return (!ui->keySequenceWidget->keySequence().toString().isEmpty() && (totalChecked > 0));
  }
  return true;
}

ActionDialog::~ActionDialog() = default;
