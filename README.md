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
spp-fw-updater /dev/rfcommX firmware.bin
```
