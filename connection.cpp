#include "connection.h"

#include "common.h"

#include <QSslSocket>
#include <QSslConfiguration>

Connection::Connection(QObject *parent, const Host &host, const QSslCertificate &ourCert, const QSslKey &ourKey) : QObject(parent),
    m_host(host)
{
    m_socket = new QSslSocket(this);

    QSslConfiguration config = m_socket->sslConfiguration();

    config.setCaCertificates({host.certificate});
    config.setLocalCertificate(ourCert);
    config.setPrivateKey(ourKey);
    m_socket->setSslConfiguration(config);

    m_socket->connectToHostEncrypted(host.address.toString(), TRANSFER_PORT);

    connect(m_socket, &QSslSocket::disconnected, this, &Connection::disconnected);
    connect(m_socket, &QSslSocket::encrypted, this, &Connection::onEncrypted);
    connect(m_socket, QOverload<QAbstractSocket::SocketError>::of(&QAbstractSocket::error), this, &Connection::onError);
}

bool Connection::isConnected() const
{
    return m_socket->isEncrypted() && m_socket->peerCertificate() == m_host.certificate;
}

void Connection::onEncrypted()
{
    if (m_socket->peerCertificate() != m_host.certificate) {
        qWarning() << "Connected to host with wrong certificate";
        qDebug().noquote() << m_socket->peerCertificate().toText();
        qDebug().noquote() << m_host.certificate.toText();
        m_socket->disconnectFromHost();
        return;
    }

    emit connectionEstablished(this);
}

void Connection::onError()
{
    qWarning() << m_socket->errorString();
}
