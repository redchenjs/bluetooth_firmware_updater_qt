/*
 * updater.h
 *
 *  Created on: 2019-08-18 19:00
 *      Author: Jack Chen <redchenjs@live.com>
 */

#ifndef UPDATER_H
#define UPDATER_H

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

class FirmwareUpdater: public QObject
{
    Q_OBJECT

public:
    void stop(int err = OK);
    void start(int argc, char *argv[]);

private slots:
    void sendData(void);
    void sendCommand(void);

    void processData(void);
    void processError(void);

private:
    char **m_arg = nullptr;

    QBluetoothSocket *m_device = nullptr;

    size_t m_cmd_idx = 0;
    char m_cmd_str[32] = {0};

    QFile *data_fd = nullptr;
    uint32_t data_size = 0;
    uint32_t data_done = 0;

    size_t rw_in_progress = RW_NONE;

    void printUsage(void);

signals:
    void finished(int err = OK);
};

#endif // UPDATER_H
