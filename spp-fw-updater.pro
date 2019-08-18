#-------------------------------------------------
#
# spp-fw-updater.pro
#
#  Created on: 2019-08-18 19:00
#      Author: Jack Chen <redchenjs@live.com>
#
#-------------------------------------------------

QT += core serialport

CONFIG += c++17

TARGET = spp-fw-updater
TEMPLATE = app

SOURCES += main.cpp updater.cpp
HEADERS += updater.h
