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
#include <QCheckBox>
#include <QSystemTrayIcon>

#ifdef Q_OS_LINUX
    #include <QApplication>
    #include <QDesktopWidget>
    #include <X11/extensions/XTest.h>
    #include <QX11Info>
#elif Q_OS_WINDOWS
    #ifndef WIN32_LEAN_AND_MEAN
        #define WIN32_LEAN_AND_MEAN
    #endif
    #include <windows.h>
#endif

MainWindow::MainWindow(QWidget *parent) : QMainWindow(parent)
{
    m_tray = new QSystemTrayIcon(QIcon::fromTheme("state-offline"), this);
    m_tray->show();

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
    QCheckBox *useIconsCheckbox = new QCheckBox(tr("Show icons"));

    leftWidget->layout()->addWidget(m_list);
    leftWidget->layout()->addWidget(m_trustButton);
    leftWidget->layout()->addWidget(m_mouseControlButton);
    leftWidget->layout()->addWidget(new QLabel(tr("Our fingerprint:")));
    leftWidget->layout()->addWidget(ourRandomart);
    leftWidget->layout()->addWidget(useIconsCheckbox);
    leftWidget->setMaximumWidth(ourRandomart->maximumWidth());

    connect(m_connectionHandler, &ConnectionHandler::pingFromHost, this, &MainWindow::onPingFromHost);
    connect(m_trustButton, &QPushButton::clicked, this, &MainWindow::onTrustClicked);
    connect(m_mouseControlButton, &QPushButton::clicked, this, &MainWindow::onMouseControlClicked);
    connect(m_list, &QListWidget::currentRowChanged, this, &MainWindow::onHostSelectionChanged);
    connect(m_fileList, &QListWidget::itemDoubleClicked, this, &MainWindow::onFileItemDoubleClicked);
    connect(useIconsCheckbox, &QCheckBox::stateChanged, ourRandomart, &RandomArt::setUseIcons);

    connect(m_tray, &QSystemTrayIcon::activated, this, [this]() { setVisible(!isVisible()); });

    connect(m_connectionHandler, &ConnectionHandler::mouseClickRequested, this, &MainWindow::onMouseClickRequested);
    connect(m_connectionHandler, &ConnectionHandler::mouseMoveRequested, this, [](const QPoint &position) {
#ifdef Q_OS_LINUX
        Display* display = QX11Info::display();
        Q_ASSERT(display);
        XTestFakeMotionEvent(display, -1, position.x(), position.y(), CurrentTime);
        XFlush(display);
#else
        QCursor::setPos(position);
#endif
    });
    m_cleanupTimer = new QTimer(this);
    m_cleanupTimer->setSingleShot(true);
    m_cleanupTimer->setInterval(10000);
    connect(m_cleanupTimer, &QTimer::timeout, this, &MainWindow::onCleanup);
}

void MainWindow::onMouseControlClicked()
{
    int row = m_list->currentRow();
    if (row < 0 || row >= m_visibleHosts.count()) {
        qWarning() << "Invalid selection" << row;
        return;
    }

    MouseControlWindow *mouseInputDialog = new MouseControlWindow;
    mouseInputDialog->setWindowFlags(Qt::Dialog | Qt::NoDropShadowWindowHint | Qt::FramelessWindowHint | mouseInputDialog->windowFlags());

    // Semi-translucent background
    mouseInputDialog->setAttribute(Qt::WA_TranslucentBackground);
    mouseInputDialog->setAttribute(Qt::WA_NoSystemBackground, false);
    QColor background = palette().window().color();
    background.setAlpha(128);
    mouseInputDialog->setStyleSheet(QStringLiteral("background:%1").arg(background.name(QColor::HexArgb)));

    mouseInputDialog->connection = new Connection(m_connectionHandler);
    connect(mouseInputDialog->connection, &Connection::disconnected, mouseInputDialog, &MouseControlWindow::close);

    connect(mouseInputDialog, &MouseControlWindow::destroyed, m_mouseControlButton, [this, mouseInputDialog]() {
        this->m_mouseControlButton->setEnabled(true);
        mouseInputDialog->connection->socket()->disconnectFromHost();
    });

    connect(mouseInputDialog->connection, &Connection::connectionEstablished, mouseInputDialog, [mouseInputDialog]() {
        mouseInputDialog->setGeometry(QApplication::desktop()->availableGeometry(mouseInputDialog));

        connect(mouseInputDialog, &MouseControlWindow::mouseMoved, mouseInputDialog->connection, &Connection::sendMouseMoveEvent);
        connect(mouseInputDialog, &MouseControlWindow::mouseClicked, mouseInputDialog->connection, &Connection::sendMouseClickEvent);

        mouseInputDialog->setText("Press escape to cancel mouse control");
        mouseInputDialog->setMouseTracking(true);
    });


    mouseInputDialog->setAlignment(Qt::AlignCenter);
    mouseInputDialog->setText("Connecting to host...");

    mouseInputDialog->show();
    mouseInputDialog->connection->initiateMouseControl(m_visibleHosts[row]);
}

void MainWindow::onMouseClickRequested(const QPoint &position, const MouseButton button)
{
    m_mouseCommandTimer.restart();
    updateTrayIcon();

    QCursor::setPos(position);

#ifdef Q_OS_LINUX
    int xButton = 0;
    switch(button) {
    case LeftButton:
        xButton = Button1;
        break;
    case MiddleButton:
        xButton = Button2;
        break;
    case RightButton:
        xButton = Button3;
        break;
    case ScrollUp:
        xButton = Button4;
        break;
    case ScrollDown:
        xButton = Button5;
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
#elif Q_OS_WINDOWS
    INPUT inputEvent;
    ZeroMemory(&inputEvent, sizeof(inputEvent));
    inputEvent.type = INPUT_MOUSE;

    // Press
    switch(button) {
    case LeftButton:
        inputEvent.mi.dwFlags = MOUSEEVENTF_LEFTDOWN;
        break;
    case MidButton:
        inputEvent.mi.dwFlags = MOUSEEVENTF_MIDDLEDOWN;
        break;
    case RightButton:
        inputEvent.mi.dwFlags = MOUSEEVENTF_RIGHTDOWN;
        break;
    default:
        qWarning() << "unhandled button" << button;
        return;
    }
    SendInput(1, &inputEvent, sizeof(inputEvent));

    // Release
    switch(button) {
    case LeftButton:
        inputEvent.mi.dwFlags = MOUSEEVENTF_LEFTUP;
        break;
    case MidButton:
        inputEvent.mi.dwFlags = MOUSEEVENTF_MIDDLEUP;
        break;
    case RightButton:
        inputEvent.mi.dwFlags = MOUSEEVENTF_RIGHTUP;
        break;
    default:
        qWarning() << "unhandled button" << button;
        return;
    }
    SendInput(1, &inputEvent, sizeof(inputEvent));
#else
    qWarning() << "Mouse stuff only available on Linux/X11 and windows";
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
        m_visibleHosts[index].offline = false;
        m_list->item(index)->setText(displayName);
        m_list->item(index)->setIcon(host.trusted ? QIcon::fromTheme("security-high") : QIcon::fromTheme("security-low"));
        if (!m_cleanupTimer->isActive()) {
            m_cleanupTimer->start();
        }
        return;
    }

    Q_ASSERT(m_visibleHosts.count() == m_list->count());

    if (m_visibleHosts.count() > 100) {
        qWarning() << "Too many";
        if (m_visibleHosts.last().trusted) {
            // full with trusted hosts, can't do anything
            return;
        }
        m_visibleHosts.removeLast();
        delete m_list->takeItem(m_list->count() - 1);
        Q_ASSERT(m_visibleHosts.count() == m_list->count());
    }

    // Insert before the first non-trusted
    int insertPosition = 0;
    for (int i=0; i<m_visibleHosts.count(); i++) {
        if (!m_visibleHosts[i].trusted) {
            break;
        }
    }

    QListWidgetItem *item = new QListWidgetItem(displayName);
    item->setIcon(host.trusted ? QIcon::fromTheme("security-high") : QIcon::fromTheme("security-low"));
    m_list->insertItem(insertPosition, item);
    m_visibleHosts.insert(insertPosition, host);

    if (!m_cleanupTimer->isActive()) {
        m_cleanupTimer->start();
    }

    updateTrayIcon();
}

void MainWindow::onCleanup()
{
    updateTrayIcon();

    Q_ASSERT(m_visibleHosts.count() == m_list->count());

    if (m_visibleHosts.isEmpty()) {
        return;
    }

    bool anyActive = false;
    for (int i=0; i<m_visibleHosts.size(); i++) {
        if (m_visibleHosts[i].offline) {
            continue;
        }
        if (m_visibleHosts[i].lastSeen.msecsTo(QDateTime::currentDateTime()) > 5000) {
            m_visibleHosts[i].offline = true;
            m_list->item(i)->setIcon(QIcon::fromTheme("network-offline"));
            continue;
        }
        anyActive = true;
    }

    if (anyActive) {
        m_cleanupTimer->start(10000);
    }
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

    if (host.offline) {
        m_trustButton->setEnabled(false);
        m_mouseControlButton->setEnabled(false);
        m_fileList->clear();
        return;
    }

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
    if (currentHost().offline) {
        return;
    }

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

void MainWindow::updateTrayIcon()
{
    QString trayIcon = "state-offline";
    if (m_mouseCommandTimer.isValid() && m_mouseCommandTimer.elapsed() < 10000) {
        trayIcon = "state-warning";
    } else if (!m_visibleHosts.isEmpty()) {
        trayIcon = "state-information";
        for (const Host &host : m_visibleHosts) {
            if (host.offline) {
                continue;
            }
            if (host.trusted) {
                trayIcon = "state-ok";
                break;
            }
        }
    }
    if (trayIcon == m_trayIcon) {
        return;
    }
    m_trayIcon = trayIcon;

    m_tray->setIcon(QIcon::fromTheme(m_trayIcon));
}
