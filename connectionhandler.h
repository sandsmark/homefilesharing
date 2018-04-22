#ifndef CONNECTIONHANDLER_H
#define CONNECTIONHANDLER_H

#include <QSslCertificate>
#include <QSslKey>
#include <QUdpSocket>
#include <QTimer>

class ConnectionHandler : public QObject
{
    Q_OBJECT

public:
    ConnectionHandler();

private slots:
    void sendPing();

private:
    void generateKey();

    QSslCertificate m_certificate;
    QSslKey m_key;
    QUdpSocket m_pingSocket;
    QTimer m_pingTimer;
    QByteArray m_digest;
};

#endif // CONNECTIONHANDLER_H
