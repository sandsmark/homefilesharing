#include "connectionhandler.h"

#include "common.h"

#include <openssl/x509.h>
#include <openssl/pem.h>
#include <openssl/bio.h>
#include <openssl/err.h>
#include <memory>

#include <QMessageBox>
#include <QSettings>
#include <QHostInfo>

ConnectionHandler::ConnectionHandler()
{
    QSettings settings;
    m_certificate = QSslCertificate(settings.value("privcert").toByteArray());
    m_key = QSslKey(settings.value("privkey").toByteArray(), QSsl::Ec);
    if (m_certificate.isNull() || m_key.isNull()) {
        generateKey();
    }

    m_pingTimer.setInterval(1000);
    connect(&m_pingTimer, &QTimer::timeout, this, &ConnectionHandler::sendPing);
    m_pingTimer.start();
}

void ConnectionHandler::sendPing()
{
    QByteArray datagram(PING_HEADER);
    datagram += ';' + QSysInfo::machineHostName().toUtf8();
    datagram += ';' + m_certificate.toDer().toBase64();
    m_pingSocket.writeDatagram(datagram, QHostAddress::Broadcast, PING_PORT);
}


using BN_ptr = std::unique_ptr<BIGNUM, decltype(&::BN_free)>;
using EC_KEY_ptr = std::unique_ptr<EC_KEY, decltype(&::EC_KEY_free)>;
using EVP_KEY_ptr = std::unique_ptr<EVP_PKEY, decltype(&::EVP_PKEY_free)>;
using BIO_FILE_ptr = std::unique_ptr<BIO, decltype(&::BIO_free)>;
using X509_ptr = std::unique_ptr<X509, decltype(&::X509_free)>;

void ConnectionHandler::generateKey()
{
    EC_KEY_ptr ecKey(EC_KEY_new_by_curve_name(NID_X9_62_prime256v1), ::EC_KEY_free);
    if (!ecKey) {
        qWarning() << ERR_error_string(ERR_get_error(), NULL);
        QMessageBox::warning(nullptr, "Error creating certificate", "Failed to create EC key with required curve");
        return;
    }

    EC_KEY_set_asn1_flag(ecKey.get(), OPENSSL_EC_NAMED_CURVE);

    if (!EC_KEY_generate_key(ecKey.get())) {
        QMessageBox::warning(nullptr, "Error creating certificate", "Failed to generate EC key");
        return;

    }

    EVP_KEY_ptr pkey(EVP_PKEY_new(), ::EVP_PKEY_free);
    int rc = EVP_PKEY_set1_EC_KEY(pkey.get(), ecKey.get());
    Q_ASSERT(rc == 1);


    EVP_PKEY_assign_EC_KEY(pkey.get(), ecKey.release());
    X509_ptr x509(X509_new(), ::X509_free);

    X509_gmtime_adj(X509_get_notBefore(x509.get()), 0); // not before current time
    X509_gmtime_adj(X509_get_notAfter(x509.get()), 315360000L); // 10 years validity
    X509_set_pubkey(x509.get(), pkey.get());

    X509_sign(x509.get(), pkey.get(), EVP_sha1());
    BIO_FILE_ptr bp_private(BIO_new(BIO_s_mem()), ::BIO_free);
    if(!PEM_write_bio_PrivateKey(bp_private.get(), pkey.get(), nullptr, nullptr, 0, nullptr, nullptr)) {
        qFatal("PEM_write_bio_PrivateKey");
    }

    BIO_FILE_ptr bp_public(BIO_new(BIO_s_mem()), ::BIO_free);
    if(!PEM_write_bio_X509(bp_public.get(), x509.get())) {
        qFatal("PEM_write_bio_PrivateKey");
    }

    const char *buffer = nullptr;
    long size = BIO_get_mem_data(bp_public.get(), &buffer);
    q_check_ptr(buffer);

    m_certificate = QSslCertificate(QByteArray(buffer, size));
    if (m_certificate.isNull()) {
        QMessageBox::warning(nullptr, "Error creating certificate", "Failed to generate a random client certificate");
        return;
    }

    size = BIO_get_mem_data(bp_private.get(), &buffer);
    q_check_ptr(buffer);
    m_key = QSslKey(QByteArray(buffer, size), QSsl::Ec);
    if(m_key.isNull()) {
        QMessageBox::warning(nullptr, "Error creating private key", "Failed to generate a random private key");
        return;
    }

    QSettings settings;
    settings.setValue("privcert", m_certificate.toPem());
    settings.setValue("privkey", m_key.toPem());
    qDebug() << "Generated key";
}
