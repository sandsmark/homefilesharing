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
    for (char c : data) {
        for (int i=0; i<4; i++) {
            int x = position % WIDTH;
            int y = position / WIDTH;

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
            if (x + y * WIDTH == position) {
                qDebug() << "No change";
                continue;
            }

            position = x + y * WIDTH;
            m_array[position]++;

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
            painter.fillRect(r, QColor::fromHsv(m_array[x + y * WIDTH] * 11, 255, 128));

            painter.drawText(r, Qt::AlignCenter, QString(m_data[x + y * WIDTH]));
        }
    }
}
