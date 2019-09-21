SPP Firmware Updater
====================

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
spp-firmware-updater /dev/rfcommX -u firmware.bin
```

### Get device information

```
spp-firmware-updater /dev/rfcommX -i
```

### Reset device

```
spp-firmware-updater /dev/rfcommX -r
```
