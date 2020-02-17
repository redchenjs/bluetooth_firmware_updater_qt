/*
 * main.cpp
 *
 *  Created on: 2019-08-18 19:00
 *      Author: Jack Chen <redchenjs@live.com>
 */

#include <csignal>
#include <QtGlobal>

#include "updater.h"

static FirmwareUpdater updater;

void signalHandle(int signum)
{
    switch (signum) {
    case SIGINT:
    case SIGTERM:
        updater.stop();
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

    QObject::connect(&updater, SIGNAL(finished()), &app, SLOT(quit()));

    QTimer::singleShot(0, &updater, [=]()->void{updater.start(argc, argv);});

    return app.exec();
}
