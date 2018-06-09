#include "connection.h"

#include "common.h"
#include "connectionhandler.h"

#include <QSslSocket>
#include <QSslConfiguration>
#include <QSettings>
#include <QJsonDocument>
#include <QJsonObject>
#include <QDir>

Connection::Connection(ConnectionHandler *parent) :
    m_handler(parent)
{
    m_basePath = QDir::homePath() + '/';

    m_socket = new QSslSocket(this);

    QSslConfiguration config = m_socket->sslConfiguration();

    QSettings settings;
    QSslCertificate ourCert = QSslCertificate(settings.value("privcert").toByteArray());
    QSslKey ourKey = QSslKey(settings.value("privkey").toByteArray(), QSsl::Ec);

    config.setCaCertificates(parent->trustedCertificates());
    config.setLocalCertificate(ourCert);
    config.setPrivateKey(ourKey);
    m_socket->setSslConfiguration(config);
    m_socket->ignoreSslErrors();

    connect(m_socket, SIGNAL(sslErrors(QList<QSslError>)), m_socket, SLOT(ignoreSslErrors()));
    connect(m_socket, &QSslSocket::disconnected, this, &Connection::disconnected);
    connect(m_socket, &QSslSocket::disconnected, this, &Connection::onDisconnected);
    connect(m_socket, &QSslSocket::encrypted, this, &Connection::onEncrypted);
    connect(m_socket, QOverload<QAbstractSocket::SocketError>::of(&QAbstractSocket::error), this, &Connection::onError);
}

void Connection::download(const Host &host, const QString &remotePath, const QString &localPath)
{
    qDebug() << "downloading" << remotePath << "from" << host.address;
    m_host = host;
    m_type = ReceiveFile;
    m_remotePath = remotePath;
    m_localPath = localPath;

    m_socket->connectToHostEncrypted(host.address.toString(), TRANSFER_PORT);
}

void Connection::upload(const Host &host, const QString &remotePath, const QString &localPath)
{
    qDebug() << "uploading" << remotePath << "to" << host.address;

    m_host = host;
    m_type = ReceiveFile;
    m_remotePath = remotePath;
    m_localPath = localPath;

    m_socket->connectToHostEncrypted(host.address.toString(), TRANSFER_PORT);
}

void Connection::list(const Host &host, const QString &remotePath)
{
    qDebug() << "listing" << remotePath << "on" << host.address;

    m_host = host;
    m_type = ReceiveListing;
    m_remotePath = remotePath;

    m_socket->connectToHostEncrypted(host.address.toString(), TRANSFER_PORT);
}

bool Connection::isConnected() const
{
    return m_socket->isEncrypted() && m_host.trusted;
}

void Connection::onEncrypted()
{
    m_host.certificate = m_socket->peerCertificate();

    if (!m_handler->isTrusted(m_host)) {
        qWarning() << "Connected to host with untrusted certificate";
        qDebug().noquote() << m_socket->peerCertificate().toText();
        m_socket->disconnectFromHost();
        deleteLater();
        return;
    }
    qDebug() << "Encryption complete";

    m_host.address = m_socket->peerAddress();

    emit connectionEstablished(this);

    if (m_type == Incoming) {
        connect(m_socket, &QSslSocket::readyRead, this, &Connection::onReadyRead);
        qDebug() << "Incoming connection, waiting for request";
        return;
    }

    QJsonObject request;

    switch (m_type) {
    case ReceiveListing:
        request["command"] = "list";
        request["path"] = m_remotePath;
        connect(m_socket, &QSslSocket::readyRead, this, &Connection::onReadyRead);
        break;
    case SendFile:
        request["command"] = "upload";
        request["path"] = m_remotePath;
        connect(m_socket, &QSslSocket::bytesWritten, this, &Connection::onBytesWritten);
        break;
    case ReceiveFile:
        request["command"] = "download";
        request["path"] = m_remotePath;
        connect(m_socket, &QSslSocket::readyRead, this, &Connection::onReadyRead);
        break;

    case Incoming:
        return;

    default:
        qDebug() << "Unexpected type" << m_type;
        return;
    }

    qDebug() << "sending" << request;
    m_socket->write(QJsonDocument(request).toJson(QJsonDocument::Compact) + "\n");
}

void Connection::onError()
{
    qWarning() << "server error" << m_socket->errorString();
    m_socket->disconnectFromHost();
    deleteLater();
}

void Connection::onDisconnected()
{
    qDebug() << "Disconnected from" << m_host.address << "type" << m_type;

    if (m_file) {
        m_file->close();
        m_file->deleteLater();
        m_file = nullptr;
    }

    deleteLater();
}

void Connection::onReadyRead()
{
    if (m_type == Incoming) {
        if (!m_socket->canReadLine()) {
            return;
        }

        QJsonParseError parseError;
        QJsonObject request = QJsonDocument::fromJson(m_socket->readLine(), &parseError).object();
        if (parseError.error != QJsonParseError::NoError) {
            qWarning() << "Failed to parse json" << parseError.errorString();
            m_socket->disconnectFromHost();
            return;
        }

        handleCommand(request["command"].toString(), request["path"].toString());
        return;
    }

    if (m_type == ReceiveListing) {
        qDebug() << "Listing, received data";
        QStringList entries;
        while (m_socket->canReadLine()) {
            QString entry = QString::fromUtf8(m_socket->readLine()).trimmed();
            if (!entry.isEmpty()) {
                entries.append(entry);
            }
        }
        if (!entries.isEmpty()) {
            emit listingReceived(m_remotePath, entries);
        }
        return;
    }

    if (m_type != ReceiveFile){
        qWarning() << "Unexpected readyread for connection type" << m_type;
        m_socket->disconnectFromHost();
        return;
    }

    if (!m_file) {
        m_file = new QFile(m_localPath, this);

        if (m_file->exists()) {
            qWarning() << "Refusing to overwrite local file" << m_localPath;
            m_socket->disconnectFromHost();
            return;
        }

        if (!m_file->open(QIODevice::WriteOnly)) {
            qWarning() << "Failed to open" << m_localPath << "for writing" << m_file->errorString();
            m_socket->disconnectFromHost();
            return;
        }

        qDebug() << "Opened" << m_localPath << "for writing";
    }

    m_file->write(m_socket->readAll());
}

void Connection::onBytesWritten(qint64 bytes)
{
    qDebug() << "Wrote" << bytes;

    if (m_socket->bytesToWrite() > 0) {
        qDebug() << "Still" << m_socket->bytesToWrite() << "bytes to write";
        return;
    }

    if (m_type == SendListing) {
        qDebug() << "Finished sending list";
        m_socket->disconnectFromHost();
        return;
    }

    if (m_type != SendFile) {
        qWarning() << "Bytes written, but we're not sending a file!";
        m_socket->disconnectFromHost();
        return;
    }

    if (!m_file) {
        m_file = new QFile(m_localPath, this);

        if (m_file->exists()) {
            qWarning() << "Refusing to overwrite local file" << m_localPath;
            m_socket->disconnectFromHost();
            return;
        }

        if (!m_file->open(QIODevice::ReadOnly)) {
            qWarning() << "Failed to open" << m_localPath << "for writing" << m_file->errorString();
            m_socket->disconnectFromHost();
            return;
        }

        qDebug() << "Opened" << m_localPath << "for reading";
    }

    if (m_file->atEnd()) {
        qDebug() << "finished sending file";
        m_socket->disconnectFromHost();
        return;
    }

    m_socket->write(m_file->read(TRANSFER_BYTE_SIZE));
}

void Connection::handleCommand(const QString &command, QString path)
{
    path = m_basePath + QDir::cleanPath(path);

    qDebug() << "Got command" << command << "for" << path;

    if (command == "list") {
        QString retData = QDir(path).entryList().join('\n') + "\n";
        m_socket->write(retData.toUtf8());
        m_type = SendListing;
        connect(m_socket, &QSslSocket::bytesWritten, this, &Connection::onBytesWritten);
        return;
    }

    if (command == "upload") {
        m_type = ReceiveFile;
    } else if (command == "download") {
        m_type = SendFile;
        connect(m_socket, &QSslSocket::bytesWritten, this, &Connection::onBytesWritten);
    } else  {
        qWarning() << "Unknown command" << command;
        m_socket->disconnectFromHost();
        return;
    }

    m_localPath = path;
}
