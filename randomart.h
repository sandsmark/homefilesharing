#ifndef RANDOMART_H
#define RANDOMART_H

#include <QObject>
#include <QWidget>

class RandomArt : public QWidget
{
    Q_OBJECT
public:
    explicit RandomArt(const QByteArray &data);

signals:

public slots:

protected:
    void paintEvent(QPaintEvent *);

private:
    QString m_data;
    QVector<int> m_array;
};

#endif // RANDOMART_H
