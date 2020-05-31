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

protected:
    void paintEvent(QPaintEvent *);

private:
    QString m_data;
    QVector<int> m_array;
    QVector<QLine> m_lines;
    QPainterPath m_path;
};

#endif // RANDOMART_H
