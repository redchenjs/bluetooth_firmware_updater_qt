/*
 * updater.cpp
 *
 *  Created on: 2019-08-18 19:00
 *      Author: Jack Chen <redchenjs@live.com>
 */

#include <string>
#include <iostream>
#include <QtCore>
#include <QThread>
#include <QtSerialPort/QSerialPort>
#include "updater.h"

bool updater_class::send_byte(const char c)
{
    if (!m_device->isOpen()) {
        return false;
    }

    if ((m_device->write(&c, 1)) < 1) {
        return false;
    }

    return true;
}

bool updater_class::send_string(QString *s)
{
    QByteArray bytes;
    bytes.reserve(s->size());
    for (auto &c : *s) {
        bytes.append(static_cast<char>(c.unicode()));
    }

    for (int i=0; i<bytes.length(); i++) {
        if (!send_byte(bytes[i])) {
            return false;
        }
    }

    if (!send_byte('\r') || !send_byte('\n')) {
        return false;
    }

    return true;
}

void updater_class::process_data(void)
{
    QString data = m_device->readAll();

    if (data.contains("RAM:") || data.contains("VER:") || data.contains("OK") || data.contains("DONE")) {
        m_device_rsp = 1;
    } else if (data.contains("ERROR") || data.contains("LOCKED")) {
        m_device_rsp = 2;
    }

    std::cout << "<< " << data.toStdString();
}

int updater_class::open_device(const QString &devname)
{
    m_device->setPortName(devname);
    if (m_device->open(QIODevice::ReadWrite)) {
        m_device->setBaudRate(115200);
        m_device->setDataBits(QSerialPort::Data8);
        m_device->setParity(QSerialPort::NoParity);
        m_device->setStopBits(QSerialPort::OneStop);
        m_device->setFlowControl(QSerialPort::NoFlowControl);
        m_device->flush();
    } else {
        std::cout << "could not open device" << std::endl;
        return -1;
    }

    return 0;
}

int updater_class::close_device(void)
{
    m_device->clearError();
    m_device->close();

    return 0;
}

int updater_class::update_firmware(const QString &devname, QString filename)
{
    // open serial device
    if (open_device(devname)) {
        return -1;
    }

    // open firmware file
    QFile fd(filename);
    if (!fd.open(QIODevice::ReadOnly)) {
        std::cout << "could not open file" << std::endl;
        return -1;
    }

    // send update command to target device
    qint64 filesize = fd.size();
    QString cmd = QString("FW+UPD:") + cmd.number(filesize);
    std::cout << ">> " << cmd.toStdString() << std::endl;
    send_string(&cmd);
    m_device->waitForBytesWritten();
    while (m_device_rsp == 0) {
        QThread::msleep(50);
        if (m_device_rsp == 0) {
            m_device->waitForReadyRead();
        }
    }
    // error
    if (m_device_rsp == 2) {
        return -2;
    }
    m_device_rsp = 0;

    // send firmware data
    QByteArray filedata = fd.readAll();
    for (int i=0; i<filedata.size(); i++) {
        bool rc = send_byte(filedata.at(i));
        if (!rc) {
            std::cout << "write failed" << std::endl;
            return -3;
        }
        // flush every 32k data
        if ((i+1) % 32768 == 0) {
            m_device->waitForBytesWritten();
            QThread::msleep(750);
        }
        std::cout << ">> SENT:" << i*100/filedata.size() << "%\r";
    }
    std::cout << std::endl;
    m_device->waitForBytesWritten();
    while (m_device_rsp == 0) {
        QThread::msleep(50);
        if (m_device_rsp == 0) {
            m_device->waitForReadyRead();
        }
    }
    // error
    if (m_device_rsp == 2) {
        return -4;
    }
    m_device_rsp = 0;

    // reset target device
    cmd = QString("FW+RST");
    std::cout << ">> " << cmd.toStdString() << std::endl;
    send_string(&cmd);
    m_device->waitForBytesWritten();

    // close firmware file
    fd.close();

    // close serial device
    return close_device();
}

int updater_class::get_device_info(const QString &devname)
{
    // open serial device
    if (open_device(devname)) {
        return -1;
    }

    // get target RAM information
    QString cmd = QString("FW+RAM?");
    std::cout << ">> " << cmd.toStdString() << std::endl;
    send_string(&cmd);
    m_device->waitForBytesWritten();
    while (m_device_rsp == 0) {
        QThread::msleep(50);
        if (m_device_rsp == 0) {
            m_device->waitForReadyRead();
        }
    }
    m_device_rsp = 0;

    // get target firmware version
    cmd = QString("FW+VER?");
    std::cout << ">> " << cmd.toStdString() << std::endl;
    send_string(&cmd);
    m_device->waitForBytesWritten();
    while (m_device_rsp == 0) {
        QThread::msleep(50);
        if (m_device_rsp == 0) {
            m_device->waitForReadyRead();
        }
    }
    m_device_rsp = 0;

    // close serial device
    return close_device();
}

int updater_class::reset_device(const QString &devname)
{
    // open serial device
    if (open_device(devname)) {
        return -1;
    }

    // reset target device
    QString cmd = QString("FW+RST");
    std::cout << ">> " << cmd.toStdString() << std::endl;
    send_string(&cmd);
    m_device->waitForBytesWritten();

    // close serial device
    return close_device();
}

void updater_class::print_usage(void)
{
    std::cout << "Usage:" << std::endl;
    std::cout << "\tspp-firmware-updater /dev/rfcommX [OPTIONS]\n" << std::endl;
    std::cout << "Options:" << std::endl;
    std::cout << "\t-u [firmware.bin]\tupdate device firmware with [firmware.bin]" << std::endl;
    std::cout << "\t-r\t\t\treset device immediately" << std::endl;
    std::cout << "\t-i\t\t\tget device information" << std::endl;
}

int updater_class::exec(int argc, char *argv[])
{
    int res = 0;

    if (argc < 3) {
        print_usage();
        return -1;
    }

    m_device = new QSerialPort(this);
    connect(m_device, &QSerialPort::readyRead, this, &updater_class::process_data);

    QString devname = QString(argv[1]);
    QString options = QString(argv[2]);

    if (options == "-i") {
        res = get_device_info(devname);
    } else if (options == "-r") {
        res = reset_device(devname);
    } else if (options == "-u") {
        QString filename = QString(argv[3]);
        res = update_firmware(devname, filename);
    } else {
        print_usage();
        return -1;
    }

    return res;
}
