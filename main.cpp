#include "mainwindow.h"

#include <QApplication>
#include <QStandardPaths>
#include <QLockFile>
#include <QMessageBox>

int main(int argc, char *argv[])
{
    QApplication a(argc, argv);
    a.setOrganizationName("Martin Sandsmark");
    a.setApplicationName("homefilesharing");

    const QString lockPath = QStandardPaths::writableLocation(QStandardPaths::TempLocation) + "/" + a.applicationName() + ".lock";
    QLockFile lockFile(lockPath);
    lockFile.setStaleLockTime(1);
    if (!lockFile.tryLock()) {
        qWarning() << lockFile.error() << lockPath;
        QMessageBox::critical(nullptr, "Already running", "Close the other one and try again");
        return 1;
    }


    MainWindow w;
    w.show();

    return a.exec();
}
