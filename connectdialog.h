#ifndef CONNECTDIALOG_H
#define CONNECTDIALOG_H

#include <QDialog>
#include <QSslCertificate>

class RandomArt;

class ConnectDialog : public QDialog
{
public:
    ConnectDialog(const QString &address, const QString &hostname, const QSslCertificate &certificate);

private:
    RandomArt *m_randomArt;
    QPushButton *m_connectButton;
    QPushButton *m_cancelButton;
};

#endif // CONNECTDIALOG_H
