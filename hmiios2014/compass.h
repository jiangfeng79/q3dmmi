#ifndef COMPASS_H
#define COMPASS_H

#include <QFont>
#include <QFontMetricsF>
#include <QPainter>
#include <QPointF>
#include <QPolygonF>
#include <QtMath>

#include <cmath>

inline void drawCompass(QPainter& painter, int width, qreal devicePixelRatio, qreal mapRotationDegrees)
{
    const qreal radius = 46.0 * devicePixelRatio;
    const qreal margin = 14.0 * devicePixelRatio;
    const QPointF center(width * devicePixelRatio - margin - radius, margin + radius);
    const qreal normalizedRotation = std::fmod(mapRotationDegrees + 360.0, 360.0);

    painter.save();
    painter.setRenderHint(QPainter::Antialiasing);
    painter.setPen(QPen(QColor(235, 235, 235, 210), 2.0 * devicePixelRatio));
    painter.setBrush(QColor(20, 35, 55, 175));
    painter.drawEllipse(center, radius, radius);

    painter.save();
    painter.translate(center);
    painter.rotate(-mapRotationDegrees);
    QPolygonF northArrow;
    northArrow << QPointF(0, -radius + 8.0 * devicePixelRatio)
               << QPointF(-6.0 * devicePixelRatio, 8.0 * devicePixelRatio)
               << QPointF(0, 4.0 * devicePixelRatio)
               << QPointF(6.0 * devicePixelRatio, 8.0 * devicePixelRatio);
    painter.setPen(Qt::NoPen);
    painter.setBrush(QColor(235, 75, 75, 230));
    painter.drawPolygon(northArrow);
    painter.restore();

    QFont compassFont(QStringLiteral("Tahoma"));
    compassFont.setPixelSize(static_cast<int>(12.0 * devicePixelRatio));
    compassFont.setBold(true);
    painter.setFont(compassFont);
    painter.setPen(QColor(255, 255, 255, 230));
    const QFontMetricsF metrics(compassFont);
    painter.drawText(center.x() - metrics.horizontalAdvance(QStringLiteral("N")) / 2,
                     center.y() - radius + 5.0 * devicePixelRatio + metrics.ascent(), QStringLiteral("N"));
    painter.drawText(center.x() - metrics.horizontalAdvance(QStringLiteral("S")) / 2,
                     center.y() + radius - 5.0 * devicePixelRatio, QStringLiteral("S"));

    QFont headingFont(QStringLiteral("Tahoma"));
    headingFont.setPixelSize(static_cast<int>(10.0 * devicePixelRatio));
    painter.setFont(headingFont);
    const QString heading = QStringLiteral("%1°").arg(normalizedRotation, 0, 'f', 0);
    painter.drawText(center.x() - QFontMetricsF(headingFont).horizontalAdvance(heading) / 2,
                     center.y() + radius + 16.0 * devicePixelRatio, heading);
    painter.restore();
}

#endif  // COMPASS_H
