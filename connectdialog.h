#ifndef CONNECTDIALOG_H
#define CONNECTDIALOG_H

#include <QDialog>

#include "host.h"

class RandomArt;

class ConnectDialog : public QDialog
{
public:
    ConnectDialog(const Host &host);

private:
    RandomArt *m_randomArt;
    QPushButton *m_connectButton;
    QPushButton *m_cancelButton;
    Host m_currentHost;
};

#endif // CONNECTDIALOG_H
