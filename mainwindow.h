#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include <QPointer>
#include <QLabel>
#include <QMouseEvent>

#include "connectionhandler.h"
#include "connection.h"

class QListWidget;
class QListWidgetItem;
class QPushButton;

class MouseControlWindow : public QLabel
{
    Q_OBJECT

signals:
    void mouseMoved(const QPoint &position);
    void mouseClicked(const QPoint &position, const Qt::MouseButton button);

public:
    QPointer<Connection> connection;


protected:
    void mouseMoveEvent(QMouseEvent *event) override {
        emit mouseMoved(event->globalPos());
    }
    void mousePressEvent(QMouseEvent *event) override {
        emit mouseClicked(event->globalPos(), event->button());
    }
    void keyPressEvent(QKeyEvent *event) override {
        if (event->key() == Qt::Key_Escape) {
            close();
        }
    }
};

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
    void onCleanup();

    void onMouseControlClicked();
    void onMouseClickRequested(const QPoint &position, const Qt::MouseButton button);

private:
    Host currentHost();
    void updateFileList();

    QListWidget *m_list;
    QPointer<ConnectionHandler> m_connectionHandler;
    QList<Host> m_visibleHosts;
    QPushButton *m_trustButton;
    QPushButton *m_mouseControlButton;
    QPointer<Connection> m_currentConnection;

    QPointer<QTimer> m_cleanupTimer;

    QListWidget *m_fileList;

    QString m_currentPath;
};

#endif // MAINWINDOW_H
