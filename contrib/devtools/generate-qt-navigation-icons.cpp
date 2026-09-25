// Copyright (c) 2026 The BitBlocks developers
// Distributed under the MIT software license, see the accompanying file COPYING.

#include <QColor>
#include <QDir>
#include <QImage>
#include <QPainter>
#include <QPainterPath>
#include <QPen>
#include <QString>

#include <functional>
#include <iostream>

namespace {

const QColor WHITE(255, 255, 255);
const QColor ACCENT(94, 220, 235);

QPen pen(const QColor& color, qreal width = 7.0)
{
    QPen result(color, width, Qt::SolidLine, Qt::RoundCap, Qt::RoundJoin);
    return result;
}

bool saveIcon(const QDir& output, const QString& name,
    const std::function<void(QPainter&)>& draw)
{
    QImage image(128, 128, QImage::Format_ARGB32_Premultiplied);
    image.fill(Qt::transparent);

    QPainter painter(&image);
    painter.setRenderHint(QPainter::Antialiasing, true);
    draw(painter);
    painter.end();

    return image.save(output.filePath(name + "-v2.png"), "PNG");
}

void drawOverview(QPainter& p)
{
    p.setPen(Qt::NoPen);
    p.setBrush(WHITE);
    p.drawRoundedRect(QRectF(18, 18, 40, 40), 9, 9);
    p.drawRoundedRect(QRectF(70, 18, 40, 40), 9, 9);
    p.drawRoundedRect(QRectF(18, 70, 40, 40), 9, 9);
    p.setBrush(ACCENT);
    p.drawRoundedRect(QRectF(70, 70, 40, 40), 9, 9);
}

void drawSend(QPainter& p)
{
    QPainterPath plane;
    plane.moveTo(15, 61);
    plane.lineTo(112, 19);
    plane.lineTo(83, 110);
    plane.lineTo(59, 75);
    plane.closeSubpath();
    p.setPen(pen(WHITE, 6));
    p.setBrush(Qt::NoBrush);
    p.drawPath(plane);
    p.setPen(pen(ACCENT, 7));
    p.drawLine(QPointF(59, 75), QPointF(91, 43));
}

void drawReceive(QPainter& p)
{
    p.setPen(pen(WHITE, 7));
    p.setBrush(Qt::NoBrush);
    p.drawRoundedRect(QRectF(20, 76, 88, 30), 8, 8);
    p.drawLine(QPointF(64, 18), QPointF(64, 72));
    p.drawLine(QPointF(42, 51), QPointF(64, 73));
    p.drawLine(QPointF(86, 51), QPointF(64, 73));
    p.setPen(pen(ACCENT, 6));
    p.drawLine(QPointF(35, 91), QPointF(93, 91));
}

void drawHistory(QPainter& p)
{
    p.setPen(pen(WHITE, 7));
    p.setBrush(Qt::NoBrush);
    p.drawArc(QRectF(22, 22, 84, 84), -35 * 16, 300 * 16);
    p.drawLine(QPointF(28, 30), QPointF(27, 56));
    p.drawLine(QPointF(28, 30), QPointF(53, 29));
    p.drawLine(QPointF(64, 40), QPointF(64, 66));
    p.setPen(pen(ACCENT, 7));
    p.drawLine(QPointF(64, 66), QPointF(83, 77));
}

void drawMasternodes(QPainter& p)
{
    p.setPen(pen(WHITE, 6));
    p.setBrush(Qt::NoBrush);
    p.drawLine(QPointF(64, 37), QPointF(31, 85));
    p.drawLine(QPointF(64, 37), QPointF(97, 85));
    p.drawLine(QPointF(31, 85), QPointF(97, 85));

    p.setPen(Qt::NoPen);
    p.setBrush(WHITE);
    p.drawEllipse(QPointF(64, 31), 17, 17);
    p.drawEllipse(QPointF(29, 91), 17, 17);
    p.drawEllipse(QPointF(99, 91), 17, 17);
    p.setBrush(ACCENT);
    p.drawEllipse(QPointF(64, 31), 7, 7);
}

} // namespace

int main(int argc, char* argv[])
{
    if (argc != 2) {
        std::cerr << "Usage: generate-qt-navigation-icons <output-directory>\n";
        return 1;
    }

    QDir output(QString::fromLocal8Bit(argv[1]));
    if (!output.exists() && !output.mkpath(".")) {
        std::cerr << "Unable to create output directory\n";
        return 1;
    }

    const bool ok = saveIcon(output, "overview", drawOverview)
        && saveIcon(output, "send", drawSend)
        && saveIcon(output, "receive", drawReceive)
        && saveIcon(output, "history", drawHistory)
        && saveIcon(output, "masternodes", drawMasternodes);

    if (!ok) {
        std::cerr << "Unable to save one or more icons\n";
        return 1;
    }
    return 0;
}
