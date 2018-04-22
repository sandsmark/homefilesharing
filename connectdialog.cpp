#include "connectdialog.h"
#include "randomart.h"
#include <QDebug>
#include <QGridLayout>
#include <QLabel>
#include <QHBoxLayout>
#include <QPushButton>

ConnectDialog::ConnectDialog(const QString &address, const QString &hostname, const QSslCertificate &certificate)
{
    QGridLayout *l = new QGridLayout;
    setLayout(l);

    l->addWidget(new QLabel("Hostname:"), 0, 0);
    l->addWidget(new QLabel(hostname), 0, 1);

    l->addWidget(new QLabel("Address:"), 1, 0);
    l->addWidget(new QLabel(address), 1, 1);

    l->addWidget(new QLabel("Certificate:"), 2, 0);
    l->addWidget(new QLabel(certificate.digest().toHex()), 2, 1);

    m_randomArt = new RandomArt(certificate.digest(QCryptographicHash::Sha3_256));
    l->addWidget(m_randomArt, 0, 2, 4, 1);

    QHBoxLayout *buttonsLayout = new QHBoxLayout;
    m_connectButton = new QPushButton("Connect");
    m_cancelButton = new QPushButton("Cancel");
    buttonsLayout->addWidget(m_connectButton);
    buttonsLayout->addWidget(m_cancelButton);
    l->addLayout(buttonsLayout, 3, 0, 1, 2);


    connect(m_cancelButton, &QPushButton::clicked, this, &QDialog::reject);
}
