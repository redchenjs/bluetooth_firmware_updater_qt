#-------------------------------------------------
#
# bluetooth-firmware-updater.pro
#
#  Created on: 2019-08-18 19:00
#      Author: Jack Chen <redchenjs@live.com>
#
#-------------------------------------------------

QT += core bluetooth

CONFIG += c++17

TARGET = btfwupd
TEMPLATE = app

SOURCES += src/main.cpp src/fwupd.cpp
HEADERS += src/fwupd.h
