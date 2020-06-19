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

### Update device firmware

```
btfwupd BD_ADDR update firmware.bin
```

### Reset device

```
btfwupd BD_ADDR reset
```

### Get device information

```
btfwupd BD_ADDR info
```
