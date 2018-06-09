#ifndef CONNECTION_H
#define CONNECTION_H

#include <QObject>
#include <QPointer>

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
        SendFile
    };
    Q_ENUM(Type)

    explicit Connection(ConnectionHandler *parent);
    explicit Connection(ConnectionHandler *parent, const Host &host, const Type type, const QString &path);

    void download(const Host &host, const QString &remotePath, const QString &localPath);
    void upload(const Host &host, const QString &remotePath, const QString &localPath);
    void list(const Host &host, const QString &remotePath);

    bool isConnected() const;

    QSslSocket *socket() const { return m_socket; }

signals:
    void listingReceived(const QString &path, const QStringList &name);
    void connectionEstablished(Connection *who);
    void disconnected();
    void bytesTransferred(qint64 bytes);

private slots:
    void onEncrypted();
    void onError();
    void onDisconnected();
    void onReadyRead();
    void onBytesWritten(qint64 bytes);

private:
    void handleCommand(const QString &command, QString path);

    QPointer<QFile> m_file;

    QSslSocket *m_socket = nullptr;
    Host m_host;
    ConnectionHandler *m_handler = nullptr;
    Type m_type = Incoming;
    QString m_remotePath;
    QString m_localPath;
    QString m_basePath;
};

#endif // CONNECTION_H
