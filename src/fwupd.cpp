/*
 * fwupd.cpp
 *
 *  Created on: 2019-08-18 19:00
 *      Author: Jack Chen <redchenjs@live.com>
 */

#include <cstdio>
#include <string>
#include <iostream>

#include <QtCore>
#include <QtBluetooth>

#include "fwupd.h"

#define OTA_SRV_UUID  0xFF52
#define OTA_CHAR_UUID 0x5201

#define TX_BUF_SIZE 512

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

void FirmwareUpdater::sendData(void)
{
    uint32_t data_remain = data_size - data_done;
    uint32_t read_length = (data_remain >= TX_BUF_SIZE) ? TX_BUF_SIZE : data_remain;

    if (data_remain == 0) {
        std::cout << ">> SENT:100%\r";

        data_fd->close();

        disconnect(data_tim, SIGNAL(timeout()));

        data_tim->stop();
    } else {
        std::cout << ">> SENT:" << data_done*100/data_size << "%\r";

        QByteArray read_buff = data_fd->read(read_length);
        m_service->writeCharacteristic(m_characteristic, read_buff, QLowEnergyService::WriteWithoutResponse);

        data_done += read_length;
    }
}

void FirmwareUpdater::sendCommand(void)
{
    std::cout << "=> " << m_cmd_str;

    m_service->writeCharacteristic(m_characteristic, m_cmd_str, QLowEnergyService::WriteWithoutResponse);
}

void FirmwareUpdater::processData(const QLowEnergyCharacteristic &c, const QByteArray &value)
{
    Q_UNUSED(c);
    const char *recv_buff = value.data();

    if (!value.contains("\r\n")) {
        return;
    }

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

                        connect(data_tim, &QTimer::timeout, this, [&]()->void{this->sendData();});

                        data_tim->start(10);
                    } else {
                        rw_in_progress = RW_NONE;

                        m_cmd_idx = CMD_IDX_RST;
                        snprintf(m_cmd_str, sizeof(m_cmd_str), CMD_FMT_RST"\r\n");

                        sendCommand();
                    }
                }
            } else {
                rw_in_progress = RW_ERROR;

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

        stop(OK);
    }
}

void FirmwareUpdater::deviceDiscovered(const QBluetoothDeviceInfo &device)
{
    if (device.coreConfigurations() & QBluetoothDeviceInfo::LowEnergyCoreConfiguration) {
        if (device.address() == m_device_address) {
            m_device = device;
            m_device_found = true;

            m_device_discovery_agent->stop();
        }
    }
}

void FirmwareUpdater::deviceDiscoveryFinished(void)
{
    if (m_device_found) {
        m_control = QLowEnergyController::createCentral(m_device, this);

        connect(m_control, &QLowEnergyController::connected, this, [&]()->void{m_control->discoverServices();});
        connect(m_control, SIGNAL(serviceDiscovered(QBluetoothUuid)), this, SLOT(serviceDiscovered(QBluetoothUuid)));
        connect(m_control, SIGNAL(discoveryFinished()), this, SLOT(serviceDiscoveryFinished()));
        connect(m_control, SIGNAL(disconnected()), this, SLOT(processRemoteError()));
        connect(m_control, SIGNAL(error(QLowEnergyController::Error)), this, SLOT(processDeviceError()));

        m_control->connectToDevice();
    } else {
        processDeviceError();
    }
}

void FirmwareUpdater::serviceDiscovered(const QBluetoothUuid &service)
{
    if (service == QBluetoothUuid((QBluetoothUuid::ServiceClassUuid)OTA_SRV_UUID)) {
        m_service_found = true;
    }
}

void FirmwareUpdater::serviceDiscoveryFinished(void)
{
    if (m_service_found) {
        m_service = m_control->createServiceObject(QBluetoothUuid((QBluetoothUuid::ServiceClassUuid)OTA_SRV_UUID), this);

        if (m_service) {
            connect(m_service, SIGNAL(stateChanged(QLowEnergyService::ServiceState)), this, SLOT(serviceStateChanged(QLowEnergyService::ServiceState)));
            connect(m_service, SIGNAL(characteristicChanged(QLowEnergyCharacteristic, QByteArray)), this, SLOT(processData(QLowEnergyCharacteristic, QByteArray)));
            connect(m_service, SIGNAL(descriptorWritten(QLowEnergyDescriptor, QByteArray)), this, SLOT(sendCommand()));
            connect(m_service, SIGNAL(error(QLowEnergyService::ServiceError)), this, SLOT(processRemoteError()));

            m_service->discoverDetails();
        }
    } else {
        processRemoteError();
    }
}

void FirmwareUpdater::serviceStateChanged(QLowEnergyService::ServiceState s)
{
    switch (s) {
    case QLowEnergyService::DiscoveringServices:
        break;
    case QLowEnergyService::ServiceDiscovered: {
        m_characteristic = m_service->characteristic(QBluetoothUuid((QBluetoothUuid::CharacteristicType)OTA_CHAR_UUID));
        if (!m_characteristic.isValid()) {
            break;
        }

        m_descriptor = m_characteristic.descriptor(QBluetoothUuid::ClientCharacteristicConfiguration);
        if (m_descriptor.isValid()) {
            m_service->writeDescriptor(m_descriptor, QByteArray::fromHex("0100"));
        }

        break;
    }
    default:
        break;
    }
}

void FirmwareUpdater::processDeviceError(void)
{
    stop(ERR_DEVICE);
}

void FirmwareUpdater::processRemoteError(void)
{
    stop(ERR_REMOTE);
}

void FirmwareUpdater::stop(int err)
{
    if (rw_in_progress == RW_WRITE) {
        std::cout << std::endl;

        data_tim->stop();
    }

    switch (err) {
    case ERR_DEVICE:
        std::cout << ">! ERROR" << std::endl;
        break;
    case ERR_REMOTE:
        if (m_cmd_idx == CMD_IDX_RST) {
            std::cout << "<= OK" << std::endl;
        } else if (rw_in_progress != RW_ERROR) {
            std::cout << "!< ERROR" << std::endl;
        }
        break;
    default:
        break;
    }

    if (m_control) {
        m_control->disconnectFromDevice();
    }

    emit finished(err);
}

void FirmwareUpdater::start(int argc, char *argv[])
{
    m_arg = argv;
    std::cout << std::unitbuf;

    QString command = QString(m_arg[2]);

    if (command == "update" && argc == 4) {
        data_fd = new QFile(m_arg[3]);
        if (!data_fd->open(QIODevice::ReadOnly)) {
            std::cout << "Could not open file" << std::endl;

            stop(ERR_FILE);
            return;
        }

        data_tim = new QTimer(this);

        data_size = static_cast<uint32_t>(data_fd->size());
        data_done = 0;

        m_cmd_idx = CMD_IDX_UPD;
        snprintf(m_cmd_str, sizeof(m_cmd_str), CMD_FMT_UPD"\r\n", data_size);
    } else if (command == "reset" && argc == 3) {
        m_cmd_idx = CMD_IDX_RST;
        snprintf(m_cmd_str, sizeof(m_cmd_str), CMD_FMT_RST"\r\n");
    } else if (command == "info" && argc == 3) {
        m_cmd_idx = CMD_IDX_RAM;
        snprintf(m_cmd_str, sizeof(m_cmd_str), CMD_FMT_RAM"\r\n");
    } else {
        printUsage();
    }

    m_device_address = QBluetoothAddress(m_arg[1]);
    m_device_discovery_agent = new QBluetoothDeviceDiscoveryAgent(this);
    m_device_discovery_agent->setLowEnergyDiscoveryTimeout(5000);

    connect(m_device_discovery_agent, SIGNAL(deviceDiscovered(QBluetoothDeviceInfo)), this, SLOT(deviceDiscovered(QBluetoothDeviceInfo)));
    connect(m_device_discovery_agent, SIGNAL(finished()), this, SLOT(deviceDiscoveryFinished()));
    connect(m_device_discovery_agent, SIGNAL(canceled()), this, SLOT(deviceDiscoveryFinished()));
    connect(m_device_discovery_agent, SIGNAL(error(QBluetoothDeviceDiscoveryAgent::Error)), this, SLOT(processDeviceError()));

    m_device_discovery_agent->start(QBluetoothDeviceDiscoveryAgent::LowEnergyMethod);
}
