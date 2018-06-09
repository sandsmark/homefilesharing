#ifndef TRANSFERDIALOG_H
#define TRANSFERDIALOG_H

#include <QDialog>
#include <QPointer>

class Connection;
class QProgressBar;
class QLabel;

class TransferDialog : public QDialog
{
    Q_OBJECT
public:
    explicit TransferDialog(QWidget *parent, Connection *transfer, qint64 totalSize);

private slots:
    void onBytesTransferred(qint64 bytes);
    void onCancel();

private:
    QPointer<QProgressBar> m_progressBar;
    QLabel *m_progressLabel;
    Connection *m_connection;
};

#endif // TRANSFERDIALOG_H
