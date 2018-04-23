#ifndef CONNECTION_H
#define CONNECTION_H

#include <QObject>

#include "host.h"

class QSslSocket;
class QSslKey;
class QSslCertificate;

class Connection : public QObject
{
    Q_OBJECT
public:
    explicit Connection(QObject *parent, const Host &host, const QSslCertificate &ourCert, const QSslKey &ourKey);

    bool isConnected() const;

signals:
    void listingFinished(const QString &path, const QStringList &name);
    void connectionEstablished(Connection *who);
    void disconnected();

private slots:
    void onEncrypted();
    void onError();

private:
    QSslSocket *m_socket;
    Host m_host;
};

#endif // CONNECTION_H
