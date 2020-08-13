Bluetooth Firmware Updater
==========================

Bluetooth Firmware Updater using BLE GATT profile.

## Dependencies

```
qt5-base
qt5-connectivity
```

## Build

```
mkdir -p build
cd build
qmake-qt5 ../
make
```

## Usage

### Get device information

```
btfwupd BD_ADDR get-info
```

### Update device firmware

```
btfwupd BD_ADDR update firmware.bin
```

### Reset the device

```
btfwupd BD_ADDR reset
```
