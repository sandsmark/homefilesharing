#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>

#include "connectionhandler.h"

class QListWidget;
class QPushButton;

class MainWindow : public QMainWindow
{
    Q_OBJECT
public:
    explicit MainWindow(QWidget *parent = nullptr);

signals:

private slots:
    void onPingFromHost(const Host &host);
    void onTrustClicked();
    void onHostSelectionChanged(int row);

private:
    QListWidget *m_list;
    ConnectionHandler *m_connectionHandler;
    QList<Host> m_visibleHosts;
    QPushButton *m_trustButton;
};

#endif // MAINWINDOW_H
