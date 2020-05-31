#include "randomart.h"
#include <QPainter>
#include <QDebug>
#include <iostream>

#include <QSslCertificate>

const char symbols[] = {
    ' ', '.', 'o', '+',
    '=', '*', 'B', 'O',
    'X', '@', '%', '&',
    '#', '/', '^', '$',
    '~', '|', 'M', 'Y',
    'W', '<', '>', 'Z',
    'S', 'E'
};

//const QChar symbols[] = {
//    ' ', '.', 'o', '+',
//    '=', '*', 'B', 'O',
//    'X', '@', '%', '&',
//    '#', '/', '^', 'S', 'E'
//};

#define WIDTH 17
#define HEIGHT 9


RandomArt::RandomArt(const QSslCertificate &cert)
{
    const QByteArray data = cert.digest(QCryptographicHash::Sha3_256);

    if (data.length() != 32) {
        qWarning() << "Invalid data" << data.length();
        return;
    }


    m_array.fill(0, WIDTH * HEIGHT);

    int position = 76;
    m_path.moveTo(position % WIDTH, position / WIDTH);
    for (char c : data) {
        for (int i=0; i<4; i++) {
            int x = position % WIDTH;
            int y = position / WIDTH;
            QPoint first(x, y);

            switch(c & 3) {
            case 0:
                x--;
                y--;
                break;
            case 1:
                x++;
                y--;
                break;
            case 2:
                x--;
                y++;
                break;
            case 3:
                x++;
                y++;
                break;
            case 4:
                Q_ASSERT(false);
            }

            x = qBound(0, x, WIDTH-1);
            y = qBound(0, y, HEIGHT-1);
            QPoint second(x, y);
            m_path.lineTo(second);
            if (x + y * WIDTH == position) {
                qDebug() << "No change";
                continue;
            }

            position = x + y * WIDTH;
            m_array[position]++;
            m_lines.append(QLine(first, second));

            c >>= 2;
        }
    }

    m_array[76] = 15;
    m_array[position] = 16;

    m_data.fill(' ', WIDTH * HEIGHT);

    for (int i=0; i<WIDTH*HEIGHT; i++) {
        m_data[i] = QString::number(m_array[i])[0];
        Q_ASSERT(size_t(m_array[i]) < sizeof(symbols));
        m_data[i] = symbols[m_array[i]];
    }

    setMinimumSize(WIDTH * 15, HEIGHT * 15);
}

void RandomArt::paintEvent(QPaintEvent *)
{
    QPainter painter(this);
    if (m_data.isEmpty()) {
        painter.fillRect(rect(), Qt::red);
        painter.drawText(0, 0, "Invalid data");
        return;
    }

    const int dx = width() / WIDTH;
    const int dy = height() / HEIGHT;

    for (int y=0; y<HEIGHT; y++) {
        for (int x=0; x<WIDTH; x++) {
            const QRect r(x * dx, y * dy, dx, dy);
            painter.fillRect(r, QColor::fromHsv(m_array[x + y * WIDTH] * 11, 255, m_array[x + y * WIDTH]  ? 255 : 0));

            //painter.drawText(r, Qt::AlignCenter, QString(m_data[x + y * WIDTH]));
        }
    }
    painter.save();
    painter.setRenderHint(QPainter::Antialiasing);
    painter.setPen(QPen(QColor(255, 255, 0, 182), dx/4));
    painter.scale(dx, dy);
    painter.setBrush(Qt::transparent);
    QPainterPathStroker stroker;

    painter.fillPath(stroker.createStroke(m_path), QColor(255, 255, 0, 32));
    painter.restore();

    for (const QLine &line : m_lines) {
        QColor color = QColor::fromHsv(m_array[line.p1().x() + line.p1().y() * WIDTH] * 11, 255, m_array[line.p2().x() + line.p2().y() * WIDTH]  ? 255 : 0);
        color.setAlpha(64);
        painter.setPen(QPen(color, dx/4));

        painter.drawLine(
                line.p1().x() * dx + dx/2, line.p1().y() * dy + dy/2,
                line.p2().x() * dx + dx/2, line.p2().y() * dy + dy/2
                );
    }
    //painter.drawPath(m_path);
}
