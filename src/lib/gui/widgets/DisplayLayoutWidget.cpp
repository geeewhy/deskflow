/*
 * Deskflow -- mouse and keyboard sharing utility
 * SPDX-License-Identifier: GPL-2.0-only WITH LicenseRef-OpenSSL-Exception
 */
#include "DisplayLayoutWidget.h"
#include "gui/config/ServerConfig.h"
#include <QCheckBox>
#include <QComboBox>
#include <QDoubleSpinBox>
#include <QFormLayout>
#include <QLabel>
#include <QMouseEvent>
#include <QPainter>
#include <QPushButton>
#include <QSignalBlocker>
#include <QVBoxLayout>
#include <functional>

class DisplayLayoutCanvas : public QWidget
{
public:
  explicit DisplayLayoutCanvas(QWidget *parent) : QWidget(parent)
  {
    setMinimumSize(480, 250);
    setObjectName("displayLayoutCanvas");
    setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);
    setToolTip(tr("Drag a computer to position it. Nearby edges snap together. Green segments allow cursor crossings."));
  }
  DisplayLayout::Rectangles rectangles;
  QString selected;
  std::function<void(const QString &, const QRectF &)> moved;
  std::function<void(const QString &)> selectionChanged;
  void fit()
  {
    QRectF bounds;
    for (const auto &rectangle : rectangles)
      bounds = bounds.isNull() ? rectangle : bounds.united(rectangle);
    if (bounds.isEmpty())
      bounds = QRectF(0, 0, 1920, 1080);
    m_scale = qMin((width() - 64.0) / bounds.width(), (height() - 64.0) / bounds.height());
    m_origin = QPointF(width() / 2.0, height() / 2.0) - bounds.center() * m_scale;
    update();
  }
protected:
  void resizeEvent(QResizeEvent *) override { fit(); }
  void paintEvent(QPaintEvent *) override
  {
    QPainter painter(this);
    painter.setRenderHint(QPainter::Antialiasing);
    painter.fillRect(rect(), palette().base());
    if (rectangles.isEmpty()) {
      painter.setPen(palette().text().color());
      painter.drawText(rect(), Qt::AlignCenter, tr("Add computers in the Computers tab."));
    }
    for (auto it = rectangles.cbegin(); it != rectangles.cend(); ++it) {
      const QRectF area(m_origin + it->topLeft() * m_scale, it->size() * m_scale);
      painter.setBrush(it.key() == selected ? palette().highlight() : palette().alternateBase());
      painter.setPen(QPen(palette().text().color(), it.key() == selected ? 2 : 1));
      painter.drawRoundedRect(area, 5, 5);
      painter.setPen(it.key() == selected ? palette().highlightedText().color() : palette().text().color());
      painter.drawText(area.adjusted(4, 4, -4, -4), Qt::AlignCenter | Qt::TextWordWrap,
                       QStringLiteral("%1\n%2 × %3").arg(it.key()).arg(it->width()).arg(it->height()));
    }
    painter.setPen(QPen(QColor(30, 180, 100), 4));
    for (const auto &link : DisplayLayout::links(rectangles)) {
      const auto area = rectangles.value(link.source);
      QPointF start, end;
      if (link.direction == "left" || link.direction == "right") {
        const double x = link.direction == "left" ? area.left() : area.right();
        start = QPointF(x, area.top() + area.height() * link.sourceStart / 100);
        end = QPointF(x, area.top() + area.height() * link.sourceEnd / 100);
      } else {
        const double y = link.direction == "up" ? area.top() : area.bottom();
        start = QPointF(area.left() + area.width() * link.sourceStart / 100, y);
        end = QPointF(area.left() + area.width() * link.sourceEnd / 100, y);
      }
      painter.drawLine(m_origin + start * m_scale, m_origin + end * m_scale);
    }
  }
  void mousePressEvent(QMouseEvent *event) override
  {
    if (event->button() != Qt::LeftButton || !isEnabled())
      return;
    const auto point = (event->position() - m_origin) / m_scale;
    for (auto it = rectangles.cbegin(); it != rectangles.cend(); ++it) {
      if (!it->contains(point))
        continue;
      selected = it.key();
      m_dragOffset = point - it->topLeft();
      m_dragging = true;
      selectionChanged(selected);
      update();
      break;
    }
  }
  void mouseMoveEvent(QMouseEvent *event) override
  {
    if (!m_dragging)
      return;
    auto area = rectangles.value(selected);
    auto position = (event->position() - m_origin) / m_scale - m_dragOffset;
    position = QPointF(std::round(position.x()), std::round(position.y()));
    area.moveTopLeft(position);
    double dx = 12 / m_scale, dy = 12 / m_scale;
    double bestX = 0, bestY = 0;
    for (auto it = rectangles.cbegin(); it != rectangles.cend(); ++it) {
      if (it.key() == selected)
        continue;
      for (double edge : {it->left(), it->right()}) {
        for (double own : {area.left(), area.right()}) {
          if (std::abs(edge - own) < dx) { dx = std::abs(edge - own); bestX = edge - own; }
        }
      }
      for (double edge : {it->top(), it->bottom()}) {
        for (double own : {area.top(), area.bottom()}) {
          if (std::abs(edge - own) < dy) { dy = std::abs(edge - own); bestY = edge - own; }
        }
      }
    }
    area.translate(bestX, bestY);
    rectangles[selected] = area;
    moved(selected, area);
    update();
  }
  void mouseReleaseEvent(QMouseEvent *) override { m_dragging = false; fit(); }
private:
  double m_scale = 1;
  QPointF m_origin, m_dragOffset;
  bool m_dragging = false;
};

DisplayLayoutWidget::DisplayLayoutWidget(ServerConfig &config, QWidget *parent)
    : QWidget(parent), m_config(config)
{
  auto layout = new QVBoxLayout(this);
  m_enabled = new QCheckBox(tr("Use display sizes and positions instead of the grid"), this);
  m_enabled->setObjectName("useDisplayLayout");
  layout->addWidget(m_enabled);
  auto explanation = new QLabel(tr("Use the same units for every computer: pixels, or physical dimensions such as millimetres. "
                                   "Drag rectangles until their edges touch; only the green portions allow crossings. "
                                   "Each rectangle represents a computer's whole desktop. Use its reported pixel size or enter dimensions manually."), this);
  explanation->setWordWrap(true);
  layout->addWidget(explanation);
  m_canvas = new DisplayLayoutCanvas(this);
  layout->addWidget(m_canvas, 1);
  auto form = new QFormLayout;
  m_computer = new QComboBox(this);
  m_computer->setObjectName("layoutComputer");
  form->addRow(tr("Computer"), m_computer);
  const QStringList labels{tr("X position"), tr("Y position"), tr("Width"), tr("Height")};
  for (int i = 0; i < 4; ++i) {
    m_fields[i] = new QDoubleSpinBox(this);
    m_fields[i]->setObjectName(QStringLiteral("layoutField%1").arg(i));
    m_fields[i]->setDecimals(2);
    m_fields[i]->setRange(i < 2 ? -1000000 : 1, 1000000);
    form->addRow(labels[i], m_fields[i]);
    connect(m_fields[i], &QDoubleSpinBox::valueChanged, this, [this] {
      if (!m_config.useDisplayLayout || m_computer->currentText().isEmpty())
        return;
      m_config.displayLayout[m_computer->currentText()] =
          QRectF(m_fields[0]->value(), m_fields[1]->value(), m_fields[2]->value(), m_fields[3]->value());
      m_canvas->rectangles = m_config.activeDisplayLayout();
      m_canvas->fit();
      m_status->setText(DisplayLayout::validationError(m_canvas->rectangles));
      Q_EMIT changed();
    });
  }
  layout->addLayout(form);
  m_reportedSize = new QPushButton(tr("Use reported pixel size"), this);
  m_reportedSize->setObjectName("useReportedSize");
  layout->addWidget(m_reportedSize);
  connect(m_reportedSize, &QPushButton::clicked, this, [this] {
    const auto size = m_config.reportedDisplaySizes.value(m_computer->currentText());
    if (!m_config.useDisplayLayout || size.isEmpty())
      return;
    auto &rectangle = m_config.displayLayout[m_computer->currentText()];
    rectangle.setSize(size);
    m_canvas->rectangles = m_config.activeDisplayLayout();
    updateFields();
    m_canvas->fit();
    Q_EMIT changed();
  });
  m_status = new QLabel(this);
  m_status->setWordWrap(true);
  layout->addWidget(m_status);
  auto fit = new QPushButton(tr("Fit arrangement in view"), this);
  layout->addWidget(fit);
  connect(fit, &QPushButton::clicked, this, [this] { m_canvas->fit(); });
  connect(m_computer, &QComboBox::currentTextChanged, this, [this] { updateFields(); });
  connect(m_enabled, &QCheckBox::toggled, this, [this](bool enabled) {
    m_config.useDisplayLayout = enabled;
    if (enabled)
      m_config.syncDisplayLayout();
    refresh();
    Q_EMIT changed();
  });
  m_canvas->selectionChanged = [this](const QString &name) { m_computer->setCurrentText(name); };
  m_canvas->moved = [this](const QString &name, const QRectF &rectangle) {
    m_config.displayLayout[name] = rectangle;
    updateFields();
    Q_EMIT changed();
  };
  refresh();
}

void DisplayLayoutWidget::refresh()
{
  const QSignalBlocker blockEnabled(m_enabled), blockComputer(m_computer);
  m_enabled->setChecked(m_config.useDisplayLayout);
  if (m_config.useDisplayLayout) {
    m_config.syncDisplayLayout();
    m_canvas->rectangles = m_config.activeDisplayLayout();
  } else {
    auto preview = m_config;
    preview.syncDisplayLayout();
    m_canvas->rectangles = preview.activeDisplayLayout();
  }
  const auto selection = m_computer->currentText();
  m_computer->clear();
  m_computer->addItems(m_canvas->rectangles.keys());
  if (m_canvas->rectangles.contains(selection))
    m_computer->setCurrentText(selection);
  m_canvas->setEnabled(m_config.useDisplayLayout);
  updateFields();
  m_canvas->fit();
}

void DisplayLayoutWidget::updateFields()
{
  m_canvas->selected = m_computer->currentText();
  const auto area = m_canvas->rectangles.value(m_canvas->selected);
  const std::array<double, 4> values{area.x(), area.y(), area.width(), area.height()};
  for (int i = 0; i < 4; ++i) {
    const QSignalBlocker blocker(m_fields[i]);
    m_fields[i]->setValue(values[i]);
    m_fields[i]->setEnabled(m_config.useDisplayLayout && !m_canvas->selected.isEmpty());
  }
  m_status->setText(m_config.useDisplayLayout ? DisplayLayout::validationError(m_canvas->rectangles) : QString());
  const auto reported = m_config.reportedDisplaySizes.value(m_canvas->selected);
  m_reportedSize->setEnabled(m_config.useDisplayLayout && !reported.isEmpty());
  m_reportedSize->setText(reported.isEmpty() ? tr("Reported size unavailable (connect this computer first)") :
                            tr("Use reported pixel size (%1 × %2)").arg(reported.width()).arg(reported.height()));
  m_canvas->update();
}
