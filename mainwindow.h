#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include <QPointer>

#include "connectionhandler.h"
#include "connection.h"

class QListWidget;
class QListWidgetItem;
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
    void onFileItemDoubleClicked(QListWidgetItem *item);

private:
    Host currentHost();
    void updateFileList();

    QListWidget *m_list;
    ConnectionHandler *m_connectionHandler;
    QList<Host> m_visibleHosts;
    QPushButton *m_trustButton;
    QPointer<Connection> m_currentConnection;

    QListWidget *m_fileList;

    QString m_currentPath;
};

#endif // MAINWINDOW_H
