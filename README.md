Easy Firmware Updater
=====================

Firmware update tool for Music Light Cube.

## Dependencies

```
bluez
cmake
pkg-config
qt5-default
qt5serialport-dev
```

## Build

```
mkdir -p build
cd build
cmake ../
make
```

## Usage

```
sudo rfcomm bind hciX XX:XX:XX:XX:XX:XX
```

### Update device firmware

```
easy-firmware-updater /dev/rfcommX update firmware.bin
```

### Reset device

```
easy-firmware-updater /dev/rfcommX reset
```

### Get device information

```
easy-firmware-updater /dev/rfcommX info
```
