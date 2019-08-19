/*
 * main.cpp
 *
 *  Created on: 2019-08-18 19:00
 *      Author: Jack Chen <redchenjs@live.com>
 */

#include <time.h>
#include <sys/stat.h>
#include <QtGlobal>
#include "updater.h"

#define LOG_FILE_PATH "/tmp/spp-firmware-updater.log"
#define LOG_FILE_MAX_LEN 2048

void msg_handle(QtMsgType type, const QMessageLogContext &context, const QString &msg)
{
    FILE *fp = nullptr;
    struct stat file_stat;

    if (!(stat(LOG_FILE_PATH, &file_stat))) {
        if (file_stat.st_size >= LOG_FILE_MAX_LEN) {
            fp = fopen(LOG_FILE_PATH, "w+");
        } else {
            fp = fopen(LOG_FILE_PATH, "a+");
        }
        if (!fp) {
            return;
        }
    } else {
        if (!(fp = fopen(LOG_FILE_PATH, "w+"))) {
            return;
        }
    }

    char time_str[24];
    time_t lt = time(nullptr);
    strftime(time_str, sizeof(time_str), "%Y-%m-%d %I:%M", localtime(&lt));

    QByteArray localMsg = msg.toLocal8Bit();
    switch (type) {
    case QtDebugMsg:
        fprintf(fp, "[%s] Debug: %s (%s:%u, %s)\n", time_str, localMsg.constData(), context.file, context.line, context.function);
        break;
    case QtInfoMsg:
        fprintf(fp, "[%s] Info: %s (%s:%u, %s)\n", time_str, localMsg.constData(), context.file, context.line, context.function);
        break;
    case QtWarningMsg:
        fprintf(fp, "[%s] Warning: %s (%s:%u, %s)\n", time_str, localMsg.constData(), context.file, context.line, context.function);
        break;
    case QtCriticalMsg:
        fprintf(fp, "[%s] Critical: %s (%s:%u, %s)\n", time_str, localMsg.constData(), context.file, context.line, context.function);
        break;
    case QtFatalMsg:
        fprintf(fp, "[%s] Fatal: %s (%s:%u, %s)\n", time_str, localMsg.constData(), context.file, context.line, context.function);
        abort();
    }

    fclose(fp);
}

int main(int argc, char *argv[])
{
    qInstallMessageHandler(msg_handle);

    updater_class updater;

    return updater.exec(argc, argv);
}
