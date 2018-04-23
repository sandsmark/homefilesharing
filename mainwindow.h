#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include <QPointer>

#include "connectionhandler.h"
#include "connection.h"

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
    void onListingFinished(const QString &path, const QStringList &names);
    void onConnected(Connection *connection);

private:
    QListWidget *m_list;
    ConnectionHandler *m_connectionHandler;
    QList<Host> m_visibleHosts;
    QPushButton *m_trustButton;
    QHash<Host, Connection*> m_connections;
    QPointer<Connection> m_currentConnection;
};

#endif // MAINWINDOW_H
