#ifndef CONNECTIONHANDLER_H
#define CONNECTIONHANDLER_H

#include <QSslCertificate>
#include <QSslKey>
#include <QUdpSocket>
#include <QTimer>
#include <QTcpServer>

#include "host.h"

class Connection;
class QTcpServer;

class ConnectionHandler : public QTcpServer
{
    Q_OBJECT

public:
    ConnectionHandler(QObject *parent);

    void trustHost(const Host &host);
    Connection *connectToHost(const Host &host);

    const QSslCertificate &ourCertificate() const;

protected:
    void incomingConnection(qintptr handle) override;

signals:
    void pingFromHost(const Host &host);

private slots:
    void sendPing();
    void onDatagram();
    void onSslClientConnected();
    void onClientError();

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
