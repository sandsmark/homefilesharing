#include "mainwindow.h"

#include "connectionhandler.h"
#include "connectdialog.h"
#include "randomart.h"

#include <QSplitter>
#include <QListWidget>
#include <QVBoxLayout>
#include <QPushButton>
#include <QMimeDatabase>

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


    m_connectionHandler = new ConnectionHandler(this);

    RandomArt *ourRandomart = new RandomArt(m_connectionHandler->ourCertificate());

    leftWidget->layout()->addWidget(m_list);
    leftWidget->layout()->addWidget(m_trustButton);
    leftWidget->layout()->addWidget(ourRandomart);

    connect(m_connectionHandler, &ConnectionHandler::pingFromHost, this, &MainWindow::onPingFromHost);
    connect(m_trustButton, &QPushButton::clicked, this, &MainWindow::onTrustClicked);
    connect(m_list, &QListWidget::currentRowChanged, this, &MainWindow::onHostSelectionChanged);
    connect(m_fileList, &QListWidget::itemDoubleClicked, this, &MainWindow::onFileItemDoubleClicked);
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
        return;
    }

    const Host &host = m_visibleHosts[row];

    if (!host.trusted) {
        m_trustButton->setEnabled(true);
        return;
    }

    m_trustButton->setEnabled(false);

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
    for (const QString name : names) {
        QListWidgetItem *item = new QListWidgetItem(name);
        if (name.endsWith('/')) {
            item->setIcon(folderIcon);
        } else {
            item->setIcon(QIcon::fromTheme(mimeDb.mimeTypeForFile(name).iconName()));
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
