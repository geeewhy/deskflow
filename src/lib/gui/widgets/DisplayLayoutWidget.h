/*
 * Deskflow -- mouse and keyboard sharing utility
 * SPDX-License-Identifier: GPL-2.0-only WITH LicenseRef-OpenSSL-Exception
 */
#pragma once
#include <QWidget>
#include <array>

class ServerConfig;
class QCheckBox;
class QComboBox;
class QDoubleSpinBox;
class QLabel;
class QPushButton;
class DisplayLayoutCanvas;

class DisplayLayoutWidget : public QWidget
{
  Q_OBJECT
public:
  explicit DisplayLayoutWidget(ServerConfig &config, QWidget *parent = nullptr);
  void refresh();
Q_SIGNALS:
  void changed();
private:
  void updateFields();
  ServerConfig &m_config;
  QCheckBox *m_enabled;
  QComboBox *m_computer;
  std::array<QDoubleSpinBox *, 4> m_fields;
  QLabel *m_status;
  QPushButton *m_reportedSize;
  DisplayLayoutCanvas *m_canvas;
};
