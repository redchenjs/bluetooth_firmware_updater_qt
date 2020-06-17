/*
 * fwupd.h
 *
 *  Created on: 2019-08-18 19:00
 *      Author: Jack Chen <redchenjs@live.com>
 */

#ifndef FWUPD_H
#define FWUPD_H

#include <QtCore>
#include <QtBluetooth>

#define OK           0
#define ERR_ARG     -1
#define ERR_FILE    -2
#define ERR_ABORT   -3
#define ERR_DEVICE  -4
#define ERR_REMOTE  -5

#define RW_NONE     0
#define RW_READ     1
#define RW_WRITE    2
#define RW_ERROR    3

class FirmwareUpdater: public QObject
{
    Q_OBJECT

public:
    void stop(int err = OK);
    void start(int argc, char *argv[]);

private:
    char **m_arg = nullptr;

    bool m_device_found = false;
    bool m_service_found = false;

    QBluetoothAddress m_device_address;
    QBluetoothDeviceDiscoveryAgent *m_device_discovery_agent = nullptr;
    QBluetoothDeviceInfo m_device;
    QLowEnergyController *m_control = nullptr;
    QLowEnergyService *m_service = nullptr;
    QLowEnergyCharacteristic m_characteristic;
    QLowEnergyDescriptor m_descriptor;

    size_t m_cmd_idx = 0;
    char m_cmd_str[32] = {0};

    QFile *data_fd = nullptr;
    QTimer *data_tim = nullptr;
    uint32_t data_size = 0;
    uint32_t data_done = 0;

    size_t rw_in_progress = RW_NONE;

    void printUsage(void);

private slots:
    void deviceDiscovered(const QBluetoothDeviceInfo &device);
    void deviceDiscoveryFinished(void);
    void serviceDiscovered(const QBluetoothUuid &service);
    void serviceDiscoveryFinished(void);
    void serviceStateChanged(QLowEnergyService::ServiceState s);

    void sendData(void);
    void sendCommand(void);

    void processData(const QLowEnergyCharacteristic &c, const QByteArray &value);
    void processDeviceError(void);
    void processRemoteError(void);

signals:
    void finished(int err = OK);
};

#endif // FWUPD_H
