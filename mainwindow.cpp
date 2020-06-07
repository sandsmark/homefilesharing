#include "mainwindow.h"

#include "connectionhandler.h"
#include "connectdialog.h"
#include "randomart.h"
#include "transferdialog.h"

#include <QSplitter>
#include <QListWidget>
#include <QVBoxLayout>
#include <QPushButton>
#include <QMimeDatabase>
#include <QSettings>
#include <QStandardPaths>
#include <QFileDialog>
#include <QMessageBox>

#ifdef Q_OS_LINUX
#include <X11/extensions/XTest.h>
#include <QX11Info>
#endif

MainWindow::MainWindow(QWidget *parent) : QMainWindow(parent)
{
    QSplitter *splitter = new QSplitter;
    setCentralWidget(splitter);

    QWidget *leftWidget = new QWidget;
    leftWidget->setLayout(new QVBoxLayout);
    splitter->addWidget(leftWidget);

    m_list = new QListWidget;
    m_trustButton = new QPushButton("Trust");
    m_trustButton->setEnabled(false);

    m_fileList = new QListWidget;
    splitter->addWidget(m_fileList);

    m_mouseControlButton = new QPushButton("Control remote mouse");
    m_mouseControlButton->setEnabled(false);

    m_connectionHandler = new ConnectionHandler(this);

    RandomArt *ourRandomart = new RandomArt(m_connectionHandler->ourCertificate());

    leftWidget->layout()->addWidget(m_list);
    leftWidget->layout()->addWidget(m_trustButton);
    leftWidget->layout()->addWidget(m_mouseControlButton);
    leftWidget->layout()->addWidget(ourRandomart);

    connect(m_connectionHandler, &ConnectionHandler::pingFromHost, this, &MainWindow::onPingFromHost);
    connect(m_trustButton, &QPushButton::clicked, this, &MainWindow::onTrustClicked);
    connect(m_mouseControlButton, &QPushButton::clicked, this, &MainWindow::onMouseControlClicked);
    connect(m_list, &QListWidget::currentRowChanged, this, &MainWindow::onHostSelectionChanged);
    connect(m_fileList, &QListWidget::itemDoubleClicked, this, &MainWindow::onFileItemDoubleClicked);

    connect(m_connectionHandler, &ConnectionHandler::mouseClickRequested, this, &MainWindow::onMouseClickRequested);
    connect(m_connectionHandler, &ConnectionHandler::mouseMoveRequested, this, [](const QPoint &position) {
        QCursor::setPos(position);
    });
}

void MainWindow::onMouseControlClicked()
{
    qDebug() << "mouse control clicked";

    int row = m_list->currentRow();
    if (row < 0 || row >= m_visibleHosts.count()) {
        qWarning() << "Invalid selection" << row;
        return;
    }

    // QMessageBox because it automatically gets a dismiss action on esc and a text message
    MouseControlWindow *mouseInputDialog = new MouseControlWindow;
    mouseInputDialog->connection = new Connection(m_connectionHandler);
    connect(mouseInputDialog->connection, &Connection::disconnected, mouseInputDialog, &MouseControlWindow::reject);

    connect(mouseInputDialog, &QDialog::finished, m_mouseControlButton, [this, mouseInputDialog]() {
        this->m_mouseControlButton->setEnabled(true);
        mouseInputDialog->connection->socket()->disconnectFromHost();
    });
    connect(mouseInputDialog->connection, &Connection::connectionEstablished, mouseInputDialog, [mouseInputDialog]() {
        mouseInputDialog->showFullScreen();

        connect(mouseInputDialog, &MouseControlWindow::mouseMoved, mouseInputDialog->connection, &Connection::sendMouseMoveEvent);
        connect(mouseInputDialog, &MouseControlWindow::mouseClicked, mouseInputDialog->connection, &Connection::sendMouseClickEvent);

        mouseInputDialog->setText("Press escape to cancel");
        mouseInputDialog->setMouseTracking(true);
    });

    mouseInputDialog->setMouseTracking(true);
    mouseInputDialog->setText("Connecting to host...");

    mouseInputDialog->show();
    mouseInputDialog->connection->initiateMouseControl(m_visibleHosts[row]);
}

void MainWindow::onMouseClickRequested(const QPoint &position, const Qt::MouseButton button)
{
#ifdef Q_OS_LINUX
    QCursor::setPos(position);

    int xButton = 0;
    switch(button) {
    case Qt::LeftButton:
        xButton = 1;
        break;
    case Qt::MidButton:
        xButton = 2;
        break;
    case Qt::RightButton:
        xButton = 3;
        break;
    default:
        qWarning() << "unhandled button" << button;
        return;
    }

    Display* display = QX11Info::display();
    Q_ASSERT(display);

    XTestFakeButtonEvent(display, xButton, True, 0);
    XTestFakeButtonEvent(display, xButton, False, 0);
    XFlush(display);
#else
    qWarning() << "Mouse stuff only available on Linux/X11";
    Q_UNUSED(position);
    Q_UNUSED(button);
#endif
}

void MainWindow::onPingFromHost(const Host &host)
{
    QString displayName = host.name + " (" + host.address.toString() + ")";;
    int index = m_visibleHosts.indexOf(host);
    if (index >= 0) {
        m_visibleHosts[index] = host;
        m_list->item(index)->setText(displayName);
        return;
    }

    QListWidgetItem *item = new QListWidgetItem(displayName);
    item->setIcon(host.trusted ? QIcon::fromTheme("security-high") : QIcon::fromTheme("security-low"));
    m_list->addItem(item);
    m_visibleHosts.append(host);
}

void MainWindow::onTrustClicked()
{
    int row = m_list->currentRow();
    if (row < 0 || row >= m_visibleHosts.count()) {
        qWarning() << "Invalid selection" << row;
        return;
    }

    ConnectDialog *dialog = new ConnectDialog(m_visibleHosts[row]);
    if (dialog->exec() == QDialog::Rejected) {
        return;
    }

    m_visibleHosts[row].trusted = true;
    m_list->item(row)->setIcon(QIcon::fromTheme("security-high"));
    m_connectionHandler->trustHost(m_visibleHosts[row]);
}

void MainWindow::onHostSelectionChanged(int row)
{
    if (row < 0 || row >= m_visibleHosts.count()) {
        qWarning() << "Invalid selection" << row;
        m_trustButton->setEnabled(false);
        m_mouseControlButton->setEnabled(false);
        return;
    }

    const Host &host = m_visibleHosts[row];

    if (!host.trusted) {
        m_trustButton->setEnabled(true);
        return;
    }

    m_trustButton->setEnabled(false);

    m_mouseControlButton->setEnabled(true);

    m_currentPath = "/";

    updateFileList();
}

void MainWindow::onListingFinished(const QString &path, const QStringList &names)
{
    if (path != m_currentPath) {
        return;
    }
    QMimeDatabase mimeDb;
    QIcon folderIcon = QIcon::fromTheme(mimeDb.mimeTypeForName("inode/directory").iconName());
    for (QString name : names) {
        int separatorPos = name.indexOf(':');
        if (separatorPos == -1) {
            qDebug() << "Invalid line" << name;
        }
        int size = name.left(separatorPos).toInt();
        name = name.mid(separatorPos + 1);

        QListWidgetItem *item = new QListWidgetItem(name);
        if (name.endsWith('/')) {
            item->setIcon(folderIcon);
        } else {
            item->setIcon(QIcon::fromTheme(mimeDb.mimeTypeForFile(name).iconName()));
            item->setData(Qt::UserRole, size);
        }

        m_fileList->addItem(item);
    }
}

void MainWindow::onFileItemDoubleClicked(QListWidgetItem *item)
{
    Q_ASSERT(item && !item->text().isEmpty());

    const QString filename = item->text();

    if (filename.endsWith('/')) {
        m_currentPath += filename;
        updateFileList();
        return;
    }

    QSettings settings;
    QString lastPath = settings.value("lastsavepath", QStandardPaths::writableLocation(QStandardPaths::DocumentsLocation)).toString();

    QString localPath = QFileDialog::getSaveFileName(this, "Where to save", lastPath + '/' + filename);
    if (localPath.isEmpty()) {
        return;
    }

    Connection *connection = new Connection(m_connectionHandler);
    new TransferDialog(this, connection, item->data(Qt::UserRole).toInt());

    connection->download(currentHost(), m_currentPath + filename, localPath);
}

Host MainWindow::currentHost()
{
    int row = m_list->currentRow();
    if (row < 0 || row >= m_visibleHosts.count()) {
        qWarning() << "Invalid selection" << row;
        return Host();
    }

    return m_visibleHosts[row];
}

void MainWindow::updateFileList()
{
    m_fileList->clear();

    if (m_currentConnection) {
        disconnect(m_currentConnection, &Connection::listingReceived, this, nullptr);
        m_currentConnection->deleteLater();
    }

    m_currentConnection = new Connection(m_connectionHandler);
    connect(m_currentConnection, &Connection::listingReceived, this, &MainWindow::onListingFinished);
    m_currentConnection->list(currentHost(), m_currentPath);
}
