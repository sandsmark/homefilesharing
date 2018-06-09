#include "transferdialog.h"

#include "connection.h"

#include <QLabel>
#include <QProgressBar>
#include <QVBoxLayout>
#include <QSslSocket>
#include <QPushButton>

TransferDialog::TransferDialog(QWidget *parent, Connection *transfer, qint64 totalSize) : QDialog(parent)
{
    setAttribute(Qt::WA_DeleteOnClose);

    m_progressBar = new QProgressBar;
    m_progressBar->setMaximum(totalSize / 1024);

    m_progressLabel = new QLabel;

    QPushButton *cancelButton = new QPushButton("Cancel");

    setLayout(new QVBoxLayout);
    layout()->addWidget(m_progressBar);
    layout()->addWidget(m_progressLabel);
    layout()->addWidget(cancelButton);

    connect(transfer, &Connection::destroyed, this, &TransferDialog::close);
    connect(transfer, &Connection::bytesTransferred, this, &TransferDialog::onBytesTransferred);
    connect(transfer->socket(), &QSslSocket::disconnected, this, &TransferDialog::close);
    connect(cancelButton, &QPushButton::clicked, this, &TransferDialog::onCancel);

    show();
}

void TransferDialog::onBytesTransferred(qint64 bytes)
{
    bytes /= 1024;
    m_progressBar->setValue(m_progressBar->value() + bytes);

    m_progressLabel->setText(QString("%1 kb / %2 kb").arg(m_progressBar->value(), m_progressBar->maximum()));
}

void TransferDialog::onCancel()
{
    if (!m_connection) {
        close();
        return;
    }

    m_connection->socket()->close();
}
