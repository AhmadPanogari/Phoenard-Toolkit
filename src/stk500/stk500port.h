#ifndef STK500PORT_H
#define STK500PORT_H

#include <QSerialPort>
#include <QSerialPortInfo>
#include <QDateTime>
#include <QDebug>
#include <QThread>

// Amount of time (in ms) spent doing a single reading cycle
#define PORT_READ_STEP_TIME 5

class stk500Port
{
public:
    bool open(const QString &portName);
    void close();
    void clear();
    void reset();
    qint32 baudRate();
    void setBaudRate(qint32 baud);
    void waitBaudCycles(int nrOfBytes);
    QString portName();
    QByteArray readAll(int timeout);
    QByteArray read(int timeout);
    QByteArray readStep();
    int write(const char* buffer, int nrOfBytes);
    QString errorString();
    bool isOpen();

    static QList<QString> getPortNames();

private:
    QSerialPort port;
};

#endif // STK500PORT_H
