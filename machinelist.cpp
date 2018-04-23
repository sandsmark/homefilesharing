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

    m_socket.bind(PING_PORT, QUdpSocket::ShareAddress);

//    connect(&m_socket, &QUdpSocket::readyRead, this, &MachineList::onDatagram);

    QPushButton *connectButton = new QPushButton("Connect");
//    connect(connectButton, &QPushButton::clicked, this, &MachineList::connectToHost);
    layout()->addWidget(connectButton);
}

MachineList::~MachineList()
{

}
