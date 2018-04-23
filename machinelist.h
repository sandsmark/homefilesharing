#ifndef MACHINELIST_H
#define MACHINELIST_H

#include <QWidget>
#include <QUdpSocket>

class QListWidget;

class MachineList : public QWidget
{
    Q_OBJECT

public:
    MachineList(QWidget *parent = 0);
    ~MachineList();

private slots:
//    void onDatagram();
//    void connectToHost();

private:
    QUdpSocket m_socket;
};

#endif // MACHINELIST_H
