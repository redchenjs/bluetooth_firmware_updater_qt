Bluetooth Firmware Updater
==========================

Bluetooth Firmware Updater using BLE GATT profile.

## Dependencies

```
cmake
qt5-base
qt5-connectivity
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
