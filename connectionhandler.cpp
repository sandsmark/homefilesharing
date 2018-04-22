#include "connectionhandler.h"

#include "common.h"

#include <openssl/x509.h>
#include <openssl/pem.h>
#include <openssl/bio.h>
#include <memory>

#include <QMessageBox>
#include <QSettings>
#include <QHostInfo>

ConnectionHandler::ConnectionHandler()
{
    QSettings settings;
    m_certificate = QSslCertificate(settings.value("privcert").toByteArray());
    m_key = QSslKey(settings.value("privkey").toByteArray(), QSsl::Rsa);
    if (m_certificate.isNull() || m_key.isNull()) {
        generateKey();
    }

    m_digest = m_certificate.digest(QCryptographicHash::Sha3_256).toHex();

    m_pingTimer.setInterval(1000);
    connect(&m_pingTimer, &QTimer::timeout, this, &ConnectionHandler::sendPing);
    m_pingTimer.start();
}

void ConnectionHandler::sendPing()
{
    QByteArray datagram(PING_HEADER);
    datagram += ';' + QSysInfo::machineHostName().toUtf8();
    datagram += ';' + m_digest;
    m_pingSocket.writeDatagram(datagram, QHostAddress::Broadcast, PING_PORT);
}


using BN_ptr = std::unique_ptr<BIGNUM, decltype(&::BN_free)>;
using RSA_ptr = std::unique_ptr<RSA, decltype(&::RSA_free)>;
using EVP_KEY_ptr = std::unique_ptr<EVP_PKEY, decltype(&::EVP_PKEY_free)>;
using BIO_FILE_ptr = std::unique_ptr<BIO, decltype(&::BIO_free)>;
using X509_ptr = std::unique_ptr<X509, decltype(&::X509_free)>;

void ConnectionHandler::generateKey()
{
    RSA_ptr rsa(RSA_new(), ::RSA_free);
    BN_ptr bn(BN_new(), ::BN_free);

    int rc = BN_set_word(bn.get(), RSA_F4);
    Q_ASSERT(rc == 1);

    // Generate key
    if (!RSA_generate_key_ex(rsa.get(), 2048, bn.get(), NULL)) {
        QMessageBox::warning(nullptr, "Error creating certificate", "Failed to generate RSA key");
        return;
    }

    // Convert RSA to PKEY
    EVP_KEY_ptr pkey(EVP_PKEY_new(), ::EVP_PKEY_free);
    rc = EVP_PKEY_set1_RSA(pkey.get(), rsa.get());
    Q_ASSERT(rc == 1);


    EVP_PKEY_assign_RSA(pkey.get(), rsa.release());
    X509_ptr x509(X509_new(), ::X509_free);

    ASN1_INTEGER_set(X509_get_serialNumber(x509.get()), 1);
    X509_gmtime_adj(X509_get_notBefore(x509.get()), 0); // not before current time
    X509_gmtime_adj(X509_get_notAfter(x509.get()), 31536000L); // not after a year from this point
    X509_set_pubkey(x509.get(), pkey.get());

    X509_NAME *name = X509_get_subject_name(x509.get());
    X509_NAME_add_entry_by_txt(name, "C", MBSTRING_ASC, (unsigned char *)"NO", -1, -1, 0);
    X509_NAME_add_entry_by_txt(name, "O", MBSTRING_ASC, (unsigned char *)"Iskrembilen", -1, -1, 0);
    X509_NAME_add_entry_by_txt(name, "CN", MBSTRING_ASC, (unsigned char *)"Random", -1, -1, 0);
    X509_set_issuer_name(x509.get(), name);

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
    m_key = QSslKey(QByteArray(buffer, size), QSsl::Rsa);
    if(m_key.isNull()) {
        QMessageBox::warning(nullptr, "Error creating private key", "Failed to generate a random private key");
        return;
    }

    QSettings settings;
    settings.setValue("privcert", m_certificate.toPem());
    settings.setValue("privkey", m_key.toPem());
    qDebug() << "Generated key";
}
