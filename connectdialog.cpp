#include "connectdialog.h"
#include "randomart.h"
#include <QDebug>
#include <QGridLayout>
#include <QLabel>
#include <QHBoxLayout>
#include <QPushButton>

ConnectDialog::ConnectDialog(const Host &host) :
    m_currentHost(host)
{
    setAttribute(Qt::WA_DeleteOnClose);

    QGridLayout *l = new QGridLayout;
    setLayout(l);

    l->addWidget(new QLabel("Hostname:"), 0, 0);
    l->addWidget(new QLabel(host.name), 0, 1);

    l->addWidget(new QLabel("Address:"), 1, 0);
    l->addWidget(new QLabel(host.address.toString()), 1, 1);

    l->addWidget(new QLabel("Certificate:"), 2, 0);
    l->addWidget(new QLabel(host.certificate.digest().toHex()), 2, 1);

    m_randomArt = new RandomArt(host.certificate);
    l->addWidget(m_randomArt, 0, 2, 4, 1);

    QHBoxLayout *buttonsLayout = new QHBoxLayout;
    m_connectButton = new QPushButton("Connect");
    m_cancelButton = new QPushButton("Cancel");
    buttonsLayout->addWidget(m_connectButton);
    buttonsLayout->addWidget(m_cancelButton);
    l->addLayout(buttonsLayout, 3, 0, 1, 2);

    connect(m_connectButton, &QPushButton::clicked, this, &QDialog::accept);
    connect(m_cancelButton, &QPushButton::clicked, this, &QDialog::reject);
}
