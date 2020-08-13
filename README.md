Bluetooth Firmware Updater
==========================

Bluetooth Firmware Updater using BLE GATT profile.

## Dependencies

```
cmake
pkg-config
qt5-default
qt5connectivity-dev
```

## Build

```
mkdir -p build
cd build
cmake ../
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
