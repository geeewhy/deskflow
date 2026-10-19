/*
 * Deskflow -- mouse and keyboard sharing utility
 * SPDX-License-Identifier: GPL-2.0-only WITH LicenseRef-OpenSSL-Exception
 */
#pragma once

#include <QMap>
#include <QRectF>
#include <QStringList>
#include <cmath>
#include <iterator>
#include <algorithm>

namespace DisplayLayout {
using Rectangles = QMap<QString, QRectF>;
struct Link
{
  QString source, target, direction;
  double sourceStart, sourceEnd, targetStart, targetEnd;
};

inline QList<Link> links(const Rectangles &rectangles)
{
  QList<Link> result;
  for (auto a = rectangles.cbegin(); a != rectangles.cend(); ++a) {
    struct Candidate { Link link; double distance; };
    QList<Candidate> candidates;
    for (auto b = rectangles.cbegin(); b != rectangles.cend(); ++b) {
      if (a == b || a->width() <= 0 || a->height() <= 0 || b->width() <= 0 || b->height() <= 0)
        continue;
      auto add = [&](const QString &direction, double distance, double start, double end, double aOrigin, double aSize,
                     double bOrigin, double bSize) {
        if (end > start)
          candidates.append({{a.key(), b.key(), direction, 100 * (start - aOrigin) / aSize,
                         100 * (end - aOrigin) / aSize, 100 * (start - bOrigin) / bSize,
                         100 * (end - bOrigin) / bSize}, distance});
      };
      const double top = qMax(a->top(), b->top());
      const double bottom = qMin(a->bottom(), b->bottom());
      const double left = qMax(a->left(), b->left());
      const double right = qMin(a->right(), b->right());
      if (b->left() - a->right() > -0.001)
        add("right", b->left() - a->right(), top, bottom, a->top(), a->height(), b->top(), b->height());
      if (a->left() - b->right() > -0.001)
        add("left", a->left() - b->right(), top, bottom, a->top(), a->height(), b->top(), b->height());
      if (b->top() - a->bottom() > -0.001)
        add("down", b->top() - a->bottom(), left, right, a->left(), a->width(), b->left(), b->width());
      if (a->top() - b->bottom() > -0.001)
        add("up", a->top() - b->bottom(), left, right, a->left(), a->width(), b->left(), b->width());
    }
    std::stable_sort(candidates.begin(), candidates.end(), [](const auto &a, const auto &b) {
      return a.distance < b.distance;
    });
    // Cast straight out from each edge: nearer screens occlude farther ones
    // only over their shared span. Preserve the coordinate mapping when split.
    for (const auto &candidate : candidates) {
      const auto &link = candidate.link;
      QList<QPair<double, double>> spans{{link.sourceStart, link.sourceEnd}};
      for (const auto &used : result) {
        if (used.source != link.source || used.direction != link.direction)
          continue;
        QList<QPair<double, double>> remaining;
        for (const auto &span : spans) {
          if (used.sourceEnd <= span.first || used.sourceStart >= span.second) {
            remaining.append(span);
          } else {
            if (span.first < used.sourceStart)
              remaining.append({span.first, used.sourceStart});
            if (span.second > used.sourceEnd)
              remaining.append({used.sourceEnd, span.second});
          }
        }
        spans = remaining;
      }
      const double scale = (link.targetEnd - link.targetStart) / (link.sourceEnd - link.sourceStart);
      for (const auto &span : spans)
        result.append({link.source, link.target, link.direction, span.first, span.second,
                       link.targetStart + (span.first - link.sourceStart) * scale,
                       link.targetStart + (span.second - link.sourceStart) * scale});
    }
  }
  return result;
}

inline QString validationError(const Rectangles &rectangles)
{
  for (auto a = rectangles.cbegin(); a != rectangles.cend(); ++a) {
    if (!std::isfinite(a->x()) || !std::isfinite(a->y()) || !std::isfinite(a->width()) ||
        !std::isfinite(a->height()) || a->width() <= 0 || a->height() <= 0)
      return QStringLiteral("Give %1 a positive width and height.").arg(a.key());
    for (auto b = std::next(a); b != rectangles.cend(); ++b) {
      const auto overlap = a->intersected(*b);
      if (overlap.width() > 0.001 && overlap.height() > 0.001)
        return QStringLiteral("%1 and %2 overlap. Move their edges together.").arg(a.key(), b.key());
    }
  }
  if (rectangles.size() < 2)
    return {};
  QStringList reached{rectangles.firstKey()};
  const auto connections = links(rectangles);
  for (qsizetype i = 0; i < reached.size(); ++i) {
    for (const auto &link : connections) {
      if (link.source == reached[i] && !reached.contains(link.target))
        reached.append(link.target);
    }
  }
  if (reached.size() != rectangles.size())
    return QStringLiteral("Some computers are disconnected. Align part of their edges so the cursor can cross.");
  return {};
}
} // namespace DisplayLayout
