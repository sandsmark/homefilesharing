#ifndef CONNECTIONHANDLER_H
#define CONNECTIONHANDLER_H

#include <QSslCertificate>
#include <QSslKey>
#include <QUdpSocket>
#include <QTimer>

#include "host.h"

class Connection;

class ConnectionHandler : public QObject
{
    Q_OBJECT

public:
    ConnectionHandler(QObject *parent);

    void trustHost(const Host &host);
    Connection *connectToHost(const Host &host);

signals:
    void pingFromHost(const Host &host);

private slots:
    void sendPing();
    void onDatagram();

private:
    void generateKey();

    QSslCertificate m_certificate;
    QSslKey m_key;
    QUdpSocket m_pingSocket;
    QTimer m_pingTimer;
    QByteArray m_digest;
    QList<Host> m_trustedHosts;
};

#endif // CONNECTIONHANDLER_H
