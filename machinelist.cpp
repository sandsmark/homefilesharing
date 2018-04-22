#include "machinelist.h"
#include "randomart.h"
#include "connectdialog.h"

#include "common.h"

#include <QListWidget>
#include <QVBoxLayout>
#include <QNetworkInterface>
#include <QPushButton>

MachineList::MachineList(QWidget *parent)
    : QWidget(parent)
{
    setLayout(new QVBoxLayout);

    m_list = new QListWidget;
    layout()->addWidget(m_list);

    m_socket.bind(PING_PORT, QUdpSocket::ShareAddress);

    connect(&m_socket, &QUdpSocket::readyRead, this, &MachineList::onDatagram);

    QPushButton *connectButton = new QPushButton("Connect");
    connect(connectButton, &QPushButton::clicked, this, &MachineList::connectToHost);
    layout()->addWidget(connectButton);
}

MachineList::~MachineList()
{

}

void MachineList::onDatagram()
{
    QList<QHostAddress> ourAddresses = QNetworkInterface::allAddresses();
    while (m_socket.hasPendingDatagrams()) {
        QByteArray datagram;
        datagram.resize(m_socket.pendingDatagramSize());

        QHostAddress sender;
        m_socket.readDatagram(datagram.data(), datagram.size(), &sender);

        // Convoluted because QHostAddress doesn't handle ipv6 stuff nicely by default
        bool isOurs = false;
        for (const QHostAddress &address : ourAddresses) {
            if (address.isEqual(sender, QHostAddress::TolerantConversion)) {
                isOurs = true;
                break;
            }
        }

        if (isOurs) {
            qDebug() << "Got ping from ourselves";
            continue;
        }

//        qDebug() << "Got ping from" << sender << datagram;

        if (!datagram.startsWith(PING_HEADER)) {
            qDebug() << "Invalid header";
            continue;
        }

        datagram.remove(0, sizeof(PING_HEADER));

        QList<QByteArray> parts = datagram.split(';');
        if (parts.count() != 2) {
            qDebug() << "Invalid structure";
            continue;
        }

        const QString hostname = QString::fromUtf8(parts[0]) + " (" + sender.toString() + ")";

        if (!m_list->findItems(hostname, Qt::MatchExactly).isEmpty()) {
            continue;
        }

        QListWidgetItem *item = new QListWidgetItem(hostname);
        item->setData(Qt::UserRole, sender.toString());
        item->setData(Qt::UserRole + 1, parts[1]);
        item->setData(Qt::UserRole + 2, QString::fromUtf8(parts[0]));

        m_list->addItem(item);
    }
}

void MachineList::connectToHost()
{
    QList<QListWidgetItem *> selectedItems = m_list->selectedItems();
    if (selectedItems.isEmpty()) {
        return;
    }

    QListWidgetItem *item = selectedItems.first();

    const QByteArray certEncoded = QByteArray::fromBase64(item->data(Qt::UserRole + 1).toByteArray());
    QSslCertificate cert = QSslCertificate(certEncoded, QSsl::Der);
    const QString hostname = item->data(Qt::UserRole + 2).toString();
    const QString hostAddress = item->data(Qt::UserRole).toString();

    ConnectDialog *dialog = new ConnectDialog(hostAddress, hostname, cert);
    dialog->show();

}
