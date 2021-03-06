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

#define NONE         1
#define OK           0
#define ERR_ARG     -1
#define ERR_FILE    -2
#define ERR_ABORT   -3
#define ERR_DISCOVR -4
#define ERR_CONTROL -5
#define ERR_SERVICE -6

#define RW_NONE  0
#define RW_READ  1
#define RW_WRITE 2

class FirmwareUpdater : public QObject
{
    Q_OBJECT

public:
    void stop(int err = OK);
    void start(int argc, char *argv[]);

private:
    char **m_arg = nullptr;

    QBluetoothAddress m_address;
    QBluetoothDeviceDiscoveryAgent *m_discovery = nullptr;
    QLowEnergyController *m_control = nullptr;
    QLowEnergyService *m_service = nullptr;
    QLowEnergyCharacteristic m_characteristic;
    QLowEnergyDescriptor m_descriptor;

    int m_cmd_idx = 0;
    char m_cmd_str[32] = {0};
    bool m_cmd_sent = false;

    QFile *data_fd = nullptr;
    uint32_t data_size = 0;
    uint32_t data_done = 0;

    int err_code = NONE;
    int rw_state = RW_NONE;

    void printUsage(void);

private slots:
    void sendData(void);
    void sendCommand(void);

    void processData(const QLowEnergyCharacteristic &c, const QByteArray &value);

    void deviceDiscovered(const QBluetoothDeviceInfo &device);
    void deviceDiscoveryFinished(void);
    void deviceConnected(void);
    void deviceDisconnected(void);
    void serviceDiscovered(const QBluetoothUuid &service);
    void serviceDiscoveryFinished(void);
    void serviceStateChanged(QLowEnergyService::ServiceState state);

    void errorDiscovery(QBluetoothDeviceDiscoveryAgent::Error err);
    void errorControl(QLowEnergyController::Error err);
    void errorService(QLowEnergyService::ServiceError err);

signals:
    void finished(int err = OK);
};

#endif // FWUPD_H
