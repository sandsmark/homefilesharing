#ifndef HOST_H
#define HOST_H

#include <QSslCertificate>
#include <QHostAddress>

struct Host {
    QString name;
    QHostAddress address;
    QSslCertificate certificate;
    bool trusted = false;

    bool operator ==(const Host &other) const {
        return certificate == other.certificate;
    }
};

static inline uint qHash(const Host &host) {
    return qHash(host.certificate);
}



#endif // HOST_H
