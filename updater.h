/*
 * updater.h
 *
 *  Created on: 2019-08-18 19:00
 *      Author: Jack Chen <redchenjs@live.com>
 */

#ifndef UPDATER_H
#define UPDATER_H

#include <QtCore>
#include <QtSerialPort/QSerialPort>

class updater_class : public QObject
{
    Q_OBJECT

public:
    int exec(int argc, char *argv[]);

private:
    void process_data(void);

    bool send_string(QString *s);
    bool send_byte(const char c);

    QSerialPort *m_device;
    bool m_device_rsp;

};

#endif // UPDATER_H
