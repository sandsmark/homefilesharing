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
    ~ConnectionHandler();

    void trustHost(const Host &host);

    const QSslCertificate &ourCertificate() const;
    const QList<QSslCertificate> trustedCertificates() const;

    const Host hostWithCert(const QSslCertificate &cert) const;
    bool isTrusted(const Host &host) const;

protected:
    void incomingConnection(qintptr handle) override;

signals:
    void pingFromHost(const Host &host);

    void mouseMoveRequested(const QPoint &position);
    void mouseClickRequested(const QPoint &position, const Qt::MouseButton button);

private slots:
    void sendPing();
    void onDatagram();
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
