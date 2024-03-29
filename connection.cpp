#include "connection.h"

#include "common.h"
#include "connectionhandler.h"

#include <QSslSocket>
#include <QSslConfiguration>
#include <QSettings>
#include <QJsonDocument>
#include <QJsonObject>
#include <QPoint>
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

#if (QT_VERSION >= QT_VERSION_CHECK(5, 15, 0))
    connect(m_socket, &QAbstractSocket::errorOccurred, this, &Connection::onError);
#else
    connect(m_socket, QOverload<QAbstractSocket::SocketError>::of(&QAbstractSocket::error), this, &Connection::onError);
#endif

    /// Avoid connections hanging for more than a second
    m_timeoutTimer = new QTimer(this);
    m_timeoutTimer->setInterval(1000); // shouldn't take more than a second to connect..
    m_timeoutTimer->setSingleShot(true);

    // abort() nukes the buffers and doesn't wait
    connect(m_timeoutTimer.data(), &QTimer::timeout, m_socket, [this]() { m_socket->abort(); });
    m_timeoutTimer->start();
}

Connection::~Connection()
{
    if (m_socket->state() != QAbstractSocket::UnconnectedState) {
        m_socket->abort();
    }
}

void Connection::initiateMouseControl(const Host &host)
{
    qDebug() << "initiating mouse control of" << host.address;
    m_host = host;
    m_type = SendMouseControl;

    m_socket->connectToHostEncrypted(host.address.toString(), TRANSFER_PORT);
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

    m_timeoutTimer->stop();

    if (!m_handler->isTrusted(m_host)) {
        qWarning() << "Connected to host with untrusted certificate";
        qDebug().noquote() << m_socket->peerCertificate().toText();
        m_socket->disconnectFromHost();
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

    case SendMouseControl:
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

void Connection::sendMouseClickEvent(const QPoint &position, const MouseButton button)
{
    QJsonObject request;
    request["command"] = "mouseclick";
    request["x"] = position.x();
    request["y"] = position.y();
    request["mousebutton"] = int(button);

    m_socket->write(QJsonDocument(request).toJson(QJsonDocument::Compact) + "\n");
}

void Connection::sendMouseMoveEvent(const QPoint &position)
{
    QJsonObject request;
    request["command"] = "mousemove";
    request["x"] = position.x();
    request["y"] = position.y();

    m_socket->write(QJsonDocument(request).toJson(QJsonDocument::Compact) + "\n");
}

void Connection::handleMouseCommand(const QString &command, const QJsonObject &data)
{
    const QPoint position(data["x"].toInt(-1), data["y"].toInt(-1));
    if (position.x() < 0 || position.y() < 0) {
        qWarning() << "invalid position";
        return;
    }

    if (command == "mousemove") {
        emit mouseMoveRequested(position);
        return;
    }

    if (command != "mouseclick") {
        qWarning() << "Unhandled mouse command" << command;
    }

    const int button = data["mousebutton"].toInt(-1);
    switch(button) {
    case LeftButton:
    case RightButton:
    case MiddleButton:
    case ScrollUp:
    case ScrollDown:
        emit mouseClickRequested(position, MouseButton(button));
        break;
    default:
        qWarning() << "Unhandled mouse button" << button;
        break;
    }

    return;
}

void Connection::onReadyRead()
{
    if (!m_handler->isTrusted(m_host)) {
        qWarning() << "Should not happen, but we got ready read for untrusted";
        qDebug().noquote() << m_socket->peerCertificate().toText();
        m_socket->disconnectFromHost();
        return;
    }

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

        const QString command = request["command"].toString();
        if (command.startsWith("mouse")) {
            handleMouseCommand(command, request);
            return;
        }

        handleCommand(command, request["path"].toString());
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

    const qint64 bytesToRead = m_socket->bytesAvailable();
    m_file->write(m_socket->readAll());
    emit bytesTransferred(bytesToRead);
}

void Connection::onBytesWritten(qint64 bytes)
{
    emit bytesTransferred(bytes);

    if (m_socket->bytesToWrite() > 0) {
        qDebug() << "Still" << m_socket->bytesToWrite() << "bytes to write";
        return;
    }

    if (m_type == SendMouseControl) {
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

        if (!m_file->exists()) {
            qWarning() << "Local file does not exist" << m_localPath;
            m_socket->disconnectFromHost();
            return;
        }

        if (!m_file->open(QIODevice::ReadOnly)) {
            qWarning() << "Failed to open" << m_localPath << "for reading" << m_file->errorString();
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

    m_localPath = path;

    qDebug() << "Got command" << command << "for" << path;

    if (command == "list") {
        QString retData;
        const QDir dir(path);
        const QFileInfoList files = dir.entryInfoList(QDir::Files | QDir::Dirs | QDir::NoDot, QDir::Name | QDir::DirsFirst | QDir::LocaleAware);
        for (const QFileInfo &fi : files) {
            retData += QString::number(fi.size()) + ':' + fi.fileName();
            if (fi.isDir()) {
                retData += '/';
            }
            retData += '\n';
        }
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
        onBytesWritten(0);
    } else  {
        qWarning() << "Unknown command" << command;
        m_socket->disconnectFromHost();
        return;
    }
}
