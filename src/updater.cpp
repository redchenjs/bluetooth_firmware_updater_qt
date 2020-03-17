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
#include <QtBluetooth>

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

void FirmwareUpdater::sendData(void)
{
    char read_buff[990] = {0};
    uint32_t data_remain = data_size - data_done;

    if (m_device->bytesToWrite() != 0) {
        return;
    }

    if (data_remain == 0) {
        std::cout << ">> SENT:100%\r";

        data_fd->close();

        disconnect(m_device, SIGNAL(bytesWritten(qint64)), this, SLOT(sendData()));
    } else {
        std::cout << ">> SENT:" << data_done*100/data_size << "%\r";

        if (data_remain >= sizeof(read_buff)) {
            if (data_fd->read(read_buff, sizeof(read_buff)) != sizeof(read_buff)) {
                std::cout << std::endl << "== ERROR" << std::endl;

                data_fd->close();

                stop(ERR_FILE);
                return;
            }

            m_device->write(read_buff, sizeof(read_buff));

            data_done += sizeof(read_buff);
        } else {
            if (data_fd->read(read_buff, data_remain) != data_remain) {
                std::cout << std::endl << "== ERROR" << std::endl;

                data_fd->close();

                stop(ERR_FILE);
                return;
            }

            m_device->write(read_buff, data_remain);

            data_done += data_remain;
        }

        QThread::msleep(20);
    }
}

void FirmwareUpdater::sendCommand(void)
{
    std::cout << "=> " << m_cmd_str;

    m_device->write(m_cmd_str, static_cast<uint32_t>(strlen(m_cmd_str)));
}

void FirmwareUpdater::processData(void)
{
    QByteArray recv = m_device->readAll();
    char *recv_buff = recv.data();

    for (uint8_t i=0; i<sizeof(rsp_fmt)/sizeof(rsp_fmt_t); i++) {
        if (strncmp(recv_buff, rsp_fmt[i].fmt, strlen(rsp_fmt[i].fmt)) == 0) {
            if (rw_in_progress != RW_NONE) {
                std::cout << std::endl;
            }
            std::cout << "<= " << recv_buff;

            if (rsp_fmt[i].flag == true) {
                if (m_cmd_idx == CMD_IDX_UPD) {
                    if (rw_in_progress == RW_NONE) {
                        rw_in_progress = RW_WRITE;

                        connect(m_device, SIGNAL(bytesWritten(qint64)), this, SLOT(sendData()));

                        sendData();
                    } else {
                        rw_in_progress = RW_NONE;

                        m_cmd_idx = CMD_IDX_RST;
                        snprintf(m_cmd_str, sizeof(m_cmd_str), CMD_FMT_RST"\r\n");

                        sendCommand();
                    }
                }
            } else {
                stop(ERR_REMOTE);
            }

            return;
        }
    }

    if (m_cmd_idx == CMD_IDX_RAM) {
        std::cout << "<= " << recv_buff;

        m_cmd_idx = CMD_IDX_VER;
        snprintf(m_cmd_str, sizeof(m_cmd_str), CMD_FMT_VER"\r\n");

        sendCommand();
    } else {
        std::cout << "<= " << recv_buff;
        stop();
    }
}

void FirmwareUpdater::processError(void)
{
    if (m_cmd_idx == CMD_IDX_RST && m_device->error() == QBluetoothSocket::NoSocketError) {
        std::cout << "== OK" << std::endl;
    } else {
        if (rw_in_progress != RW_NONE) {
            std::cout << std::endl << "== ERROR";
        } else {
            std::cout << "== ERROR" << std::endl;
        }
    }

    stop(ERR_ABORT);
}

void FirmwareUpdater::printUsage(void)
{
    std::cout << "Usage:" << std::endl;
    std::cout << "    " << m_arg[0] << " BD_ADDR COMMAND" << std::endl << std::endl;
    std::cout << "Commands:" << std::endl;
    std::cout << "    update [firmware.bin]\tupdate device firmware with [firmware.bin]" << std::endl;
    std::cout << "    reset\t\t\tdisable auto reconnect and then reset the device" << std::endl;
    std::cout << "    info\t\t\tget device information" << std::endl;

    stop(ERR_ARG);
}

void FirmwareUpdater::stop(int err)
{
    if (rw_in_progress != RW_NONE) {
        std::cout << std::endl;
    }

    if (err != OK) {
        m_device->abort();
    }

    emit finished(err);
}

void FirmwareUpdater::start(int argc, char *argv[])
{
    m_arg = argv;
    std::cout << std::unitbuf;

    QBluetoothAddress bdaddr = QBluetoothAddress(m_arg[1]);
    QString command = QString(m_arg[2]);

    m_device = new QBluetoothSocket(QBluetoothServiceInfo::RfcommProtocol, this);
    connect(m_device, SIGNAL(connected()), this, SLOT(sendCommand()));
    connect(m_device, SIGNAL(readyRead()), this, SLOT(processData()));
    connect(m_device, SIGNAL(disconnected()), this, SLOT(processError()));
    connect(m_device, SIGNAL(error(QBluetoothSocket::SocketError)), this, SLOT(processError()));

    if (command == "update" && argc == 4) {
        data_fd = new QFile(m_arg[3]);
        if (!data_fd->open(QIODevice::ReadOnly)) {
            std::cout << "Could not open file" << std::endl;
            stop(ERR_FILE);
            return;
        }

        data_size = static_cast<uint32_t>(data_fd->size());
        data_done = 0;

        m_cmd_idx = CMD_IDX_UPD;
        snprintf(m_cmd_str, sizeof(m_cmd_str), CMD_FMT_UPD"\r\n", data_size);

        m_device->connectToService(bdaddr, QBluetoothUuid::SerialPort);
    } else if (command == "reset" && argc == 3) {
        m_cmd_idx = CMD_IDX_RST;
        snprintf(m_cmd_str, sizeof(m_cmd_str), CMD_FMT_RST"\r\n");

        m_device->connectToService(bdaddr, QBluetoothUuid::SerialPort);
    } else if (command == "info" && argc == 3) {
        m_cmd_idx = CMD_IDX_RAM;
        snprintf(m_cmd_str, sizeof(m_cmd_str), CMD_FMT_RAM"\r\n");

        m_device->connectToService(bdaddr, QBluetoothUuid::SerialPort);
    } else {
        printUsage();
    }
}
