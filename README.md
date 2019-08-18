SPP Firmware Updater
====================

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
spp-firmware-updater /dev/rfcommX firmware.bin
```
