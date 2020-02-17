/*
 * updater.cpp
 *
 *  Created on: 2019-08-18 19:00
 *      Author: Jack Chen <redchenjs@live.com>
 */

#include <cstdio>
#include <string>
#include <iostream>

#include <QtCore>
#include <QThread>
#include <QtSerialPort/QSerialPort>

#include "updater.h"

#define CMD_FMT_UPD "FW+UPD:%u"
#define CMD_FMT_RST "FW+RST!"
#define CMD_FMT_RAM "FW+RAM?"
#define CMD_FMT_VER "FW+VER?"

enum cmd_idx {
    CMD_IDX_UPD = 0x0,
    CMD_IDX_RST = 0x1,
    CMD_IDX_RAM = 0x2,
    CMD_IDX_VER = 0x3,
};

enum rsp_idx {
    RSP_IDX_NONE  = 0x0,
    RSP_IDX_TRUE  = 0x1,
    RSP_IDX_FALSE = 0x2,
};

typedef struct {
    const bool flag;
    const char fmt[32];
} rsp_fmt_t;

static const rsp_fmt_t rsp_fmt[] = {
    {  true, "OK\r\n" },     // OK
    {  true, "DONE\r\n" },   // Done
    { false, "FAIL\r\n" },   // Fail
    { false, "ERROR\r\n" },  // Error
};

int FirmwareUpdater::open_device(const QString &devname)
{
    m_device->setPortName(devname);
    if (m_device->open(QIODevice::ReadWrite)) {
        m_device->setBaudRate(921600);
        m_device->setDataBits(QSerialPort::Data8);
        m_device->setParity(QSerialPort::NoParity);
        m_device->setStopBits(QSerialPort::OneStop);
        m_device->setFlowControl(QSerialPort::HardwareControl);
        m_device->clearError();
        m_device->flush();
    } else {
        std::cout << "could not open device" << std::endl;
        return ERR_DEVICE;
    }

    return OK;
}

int FirmwareUpdater::close_device(void)
{
    m_device->clearError();
    m_device->close();

    return OK;
}

void FirmwareUpdater::process_data(void)
{
    QByteArray data = m_device->readAll();

    char *data_buff = data.data();
    uint32_t data_size = static_cast<uint32_t>(data.size());

    for (uint8_t i=0; i<sizeof(rsp_fmt)/sizeof(rsp_fmt_t); i++) {
        if (strncmp(data_buff, rsp_fmt[i].fmt, strlen(rsp_fmt[i].fmt)) == 0) {
            if (rsp_fmt[i].flag == true) {
                m_device_rsp = RSP_IDX_TRUE;
            } else {
                m_device_rsp = RSP_IDX_FALSE;
                if (rw_in_progress) {
                    std::cout << std::endl;
                }
            }
            if (data_size == strlen(rsp_fmt[i].fmt)) {
                std::cout << "<= " << data_buff;
                return;
            }
        }
    }

    m_device_rsp = RSP_IDX_TRUE;
    std::cout << "<= " << data_buff;
}

void FirmwareUpdater::clear_response(void)
{
    m_device_rsp = RSP_IDX_NONE;
}

size_t FirmwareUpdater::check_response(void)
{
    m_device->waitForReadyRead(10);

    return m_device_rsp;
}

size_t FirmwareUpdater::wait_for_response(void)
{
    while (m_device_rsp == RSP_IDX_NONE) {
        m_device->waitForReadyRead(10);
    }

    return m_device_rsp;
}

int FirmwareUpdater::send_data(const char *data, uint32_t length)
{
    if (m_device->write(data, length) < 1) {
        return ERR_DEVICE;
    }

    while (m_device_rsp == RSP_IDX_NONE) {
        QThread::msleep(20);
        if (m_device->waitForBytesWritten(10)) {
            break;
        }
    }

    if (m_device_rsp == RSP_IDX_FALSE) {
        return ERR_DEVICE;
    }

    return OK;
}

int FirmwareUpdater::update(const QString &devname, QString filename)
{
    int ret = OK;
    uint32_t length = 0;
    char cmd_str[32] = {0};
    char data_buff[990] = {0};

    QFile fd(filename);
    if (!fd.open(QIODevice::ReadOnly)) {
        std::cout << "could not open file" << std::endl;
        return ERR_FILE;
    }

    length = static_cast<uint32_t>(fd.size());

    if (open_device(devname)) {
        fd.close();
        return ERR_DEVICE;
    }

    // send command
    snprintf(cmd_str, sizeof(cmd_str), CMD_FMT_UPD"\r\n", length);
    std::cout << "=> " << cmd_str;

    clear_response();

    ret = send_data(cmd_str, static_cast<uint32_t>(strlen(cmd_str)));
    if (ret != OK) {
        fd.close();
        close_device();
        return ret;
    }

    if (wait_for_response() == RSP_IDX_FALSE) {
        fd.close();
        close_device();
        return ERR_REMOTE;
    }

    // send data
    rw_in_progress = 1;
    std::cout << ">> SENT:" << "0%\r";

    clear_response();

    uint32_t pkt = 0;
    for (pkt=0; pkt<length/990; pkt++) {
        if (fd.read(data_buff, 990) != 990) {
            std::cout << std::endl << "=> ERROR" << std::endl;
            fd.close();
            close_device();
            return ERR_FILE;
        }

        ret = send_data(data_buff, 990);
        if (ret != OK) {
            fd.close();
            close_device();
            return ret;
        }

        if (m_device_rsp == RSP_IDX_FALSE) {
            fd.close();
            close_device();
            return ERR_REMOTE;
        }

        std::cout << ">> SENT:" << pkt*990*100/length << "%\r";
    }

    uint32_t data_remain = length - pkt * 990;
    if (data_remain != 0 && data_remain < 990) {
        if (fd.read(data_buff, data_remain) != data_remain) {
            std::cout << std::endl << "=> ERROR" << std::endl;
            fd.close();
            close_device();
            return ERR_FILE;
        }

        ret = send_data(data_buff, data_remain);
        if (ret != OK) {
            fd.close();
            close_device();
            return ret;
        }

        if (m_device_rsp == RSP_IDX_FALSE) {
            fd.close();
            close_device();
            return ERR_REMOTE;
        }

        std::cout << ">> SENT:" << (data_remain+pkt*990)*100/length << "%\r";
    }

    std::cout << std::endl;
    rw_in_progress = 0;

    if (wait_for_response() == RSP_IDX_FALSE) {
        fd.close();
        close_device();
        return ERR_REMOTE;
    }

    // send command
    snprintf(cmd_str, sizeof(cmd_str), CMD_FMT_RST"\r\n");
    std::cout << "=> " << cmd_str;

    clear_response();

    ret = send_data(cmd_str, static_cast<uint32_t>(strlen(cmd_str)));

    fd.close();
    close_device();

    return ret;
}

int FirmwareUpdater::reset(const QString &devname)
{
    int ret = OK;
    char cmd_str[32] = {0};

    if (open_device(devname)) {
        return ERR_DEVICE;
    }

    // send command
    snprintf(cmd_str, sizeof(cmd_str), CMD_FMT_RST"\r\n");
    std::cout << "=> " << cmd_str;

    clear_response();

    ret = send_data(cmd_str, static_cast<uint32_t>(strlen(cmd_str)));

    close_device();

    return ret;
}

int FirmwareUpdater::info(const QString &devname)
{
    int ret = OK;
    char cmd_str[32] = {0};

    if (open_device(devname)) {
        return ERR_DEVICE;
    }

    // send command
    snprintf(cmd_str, sizeof(cmd_str), CMD_FMT_RAM"\r\n");
    std::cout << "=> " << cmd_str;

    clear_response();

    ret = send_data(cmd_str, static_cast<uint32_t>(strlen(cmd_str)));
    if (ret != OK) {
        close_device();
        return ret;
    }

    if (wait_for_response() == RSP_IDX_FALSE) {
        close_device();
        return ERR_REMOTE;
    }

    // send command
    snprintf(cmd_str, sizeof(cmd_str), CMD_FMT_VER"\r\n");
    std::cout << "=> " << cmd_str;

    clear_response();

    ret = send_data(cmd_str, static_cast<uint32_t>(strlen(cmd_str)));
    if (ret != OK) {
        close_device();
        return ret;
    }

    wait_for_response();

    close_device();

    return ret;
}

void FirmwareUpdater::print_usage(char *appname)
{
    std::cout << "Usage:" << std::endl;
    std::cout << "    " << appname << " /dev/rfcommX COMMAND\n" << std::endl;
    std::cout << "Commands:" << std::endl;
    std::cout << "    update [firmware.bin]\tupdate device firmware with [firmware.bin]" << std::endl;
    std::cout << "    reset\t\t\tdisable auto reconnect and then reset the device" << std::endl;
    std::cout << "    info\t\t\tget device information" << std::endl;
}

void FirmwareUpdater::error(QSerialPort::SerialPortError err)
{
    switch (err) {
    case QSerialPort::NoError:
    case QSerialPort::TimeoutError:
        return;
    case QSerialPort::WriteError:
    case QSerialPort::ResourceError:
        if (rw_in_progress) {
            std::cout << std::endl << ">> ERROR";
        } else {
            std::cout << ">> ERROR" << std::endl;
        }
        break;
    case QSerialPort::ReadError:
        if (rw_in_progress) {
            std::cout << std::endl << "<< ERROR";
        } else {
            std::cout << "<< ERROR" << std::endl;
        }
        break;
    default:
        break;
    }

    stop();
}

void FirmwareUpdater::stop(void)
{
    if (rw_in_progress) {
        std::cout << std::endl;
    }

    m_device_rsp = RSP_IDX_FALSE;
}

void FirmwareUpdater::start(int argc, char *argv[])
{
    int ret = OK;

    if (argc < 3) {
        print_usage(argv[0]);
        emit finished(ERR_ARG);
        return;
    }

    QString devname = QString(argv[1]);
    QString options = QString(argv[2]);

    std::cout << std::unitbuf;

    m_device = new QSerialPort(this);
    connect(m_device, &QSerialPort::readyRead, this, &FirmwareUpdater::process_data);
    connect(m_device, SIGNAL(error(QSerialPort::SerialPortError)), this, SLOT(error(QSerialPort::SerialPortError)));

    if (options == "update" && argc == 4) {
        QString filename = QString(argv[3]);
        ret = update(devname, filename);
    } else if (options == "reset" && argc == 3) {
        ret = reset(devname);
    } else if (options == "info" && argc == 3) {
        ret = info(devname);
    } else {
        print_usage(argv[0]);
        emit finished(ERR_ARG);
        return;
    }

    emit finished(ret);
    return;
}
