#include "mainwindow.h"

#include <QApplication>

int main(int argc, char *argv[])
{
    QApplication a(argc, argv);
    a.setOrganizationName("Martin Sandsmark");
    a.setApplicationName("homefilesharing");

//    ConnectionHandler h;

//    MachineList w;
//    w.show();

    MainWindow w;
    w.show();

    return a.exec();
}
