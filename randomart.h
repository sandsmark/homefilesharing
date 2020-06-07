#ifndef RANDOMART_H
#define RANDOMART_H

#include <QObject>
#include <QWidget>
#include <QPainterPath>

class QSslCertificate;

class RandomArt : public QWidget
{
    Q_OBJECT
public:
    explicit RandomArt(const QSslCertificate &cert);

signals:

public slots:
    void setUseIcons(int useIcons) { // int cuz checkbox
        m_useIcons = useIcons;
        update();
    }

protected:
    void paintEvent(QPaintEvent *);

private:
    QString m_data;
    QVector<int> m_array;
    QVector<QLine> m_lines;
    QVector<QImage> m_icons;
    QPainterPath m_path;
    bool m_useIcons = false;
};

#endif // RANDOMART_H
