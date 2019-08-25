#-------------------------------------------------
#
# spp-firmware-updater.pro
#
#  Created on: 2019-08-18 19:00
#      Author: Jack Chen <redchenjs@live.com>
#
#-------------------------------------------------

QT += core serialport

CONFIG += c++17

TARGET = spp-firmware-updater
TEMPLATE = app

SOURCES += app/main.cpp app/updater.cpp
HEADERS += app/updater.h
