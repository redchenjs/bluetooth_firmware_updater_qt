/*
 * main.cpp
 *
 *  Created on: 2019-08-18 19:00
 *      Author: Jack Chen <redchenjs@live.com>
 */

#include <csignal>
#include <iostream>

#include <QtCore>
#include <QtGlobal>

#include "fwupd.h"

static FirmwareUpdater fwupd;

void signalHandle(int signum)
{
    switch (signum) {
    case SIGINT:
    case SIGTERM:
        fwupd.stop(ERR_ABORT);
        break;
    default:
        break;
    }
}

int main(int argc, char *argv[])
{
    QCoreApplication app(argc, argv);

    signal(SIGINT, signalHandle);
    signal(SIGTERM, signalHandle);

    QObject::connect(&fwupd, SIGNAL(finished()), &app, SLOT(quit()));

    QTimer::singleShot(0, &fwupd, [&]()->void{fwupd.start(argc, argv);});

    return app.exec();
}
