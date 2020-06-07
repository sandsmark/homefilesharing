#ifndef CONNECTION_H
#define CONNECTION_H

#include <QObject>
#include <QPointer>
#include <QSslSocket>

#include "host.h"

class QSslSocket;
class QSslKey;
class QSslCertificate;
class ConnectionHandler;
class QFile;

class Connection : public QObject
{
    Q_OBJECT
public:
    enum Type {
        Incoming,
        ReceiveListing,
        SendListing,
        ReceiveFile,
        SendMouseControl,
        SendFile
    };
    Q_ENUM(Type)

    explicit Connection(ConnectionHandler *parent);
    explicit Connection(ConnectionHandler *parent, const Host &host, const Type type, const QString &path);
    ~Connection();

    void download(const Host &host, const QString &remotePath, const QString &localPath);
    void upload(const Host &host, const QString &remotePath, const QString &localPath);
    void list(const Host &host, const QString &remotePath);

    bool isConnected() const;

    QSslSocket *socket() const { return m_socket; }

    void sendMouseClickEvent(const QPoint &position, const Qt::MouseButton button);
    void sendMouseMoveEvent(const QPoint &position);

signals:
    void listingReceived(const QString &path, const QStringList &name);
    void connectionEstablished(Connection *who);
    void disconnected();
    void bytesTransferred(qint64 bytes);

    void mouseMoveRequested(const QPoint &position);
    void mouseClickRequested(const QPoint &position, const Qt::MouseButton button);

private slots:
    void onEncrypted();
    void onError();
    void onDisconnected();
    void onReadyRead();
    void onBytesWritten(qint64 bytes);

private:
    void handleCommand(const QString &command, QString path);
    void handleMouseCommand(const QString &command, const QJsonObject &data);

    QPointer<QFile> m_file;

    QPointer<QSslSocket> m_socket = nullptr;
    Host m_host;
    QPointer<ConnectionHandler> m_handler;
    Type m_type = Incoming;
    QString m_remotePath;
    QString m_localPath;
    QString m_basePath;
};

#endif // CONNECTION_H
