# WiFi101 OTA library

This library allows you to have utilize OTA features [MKR1000](https://store-usa.arduino.cc/products/arduino-mkr1000-wifi) and the [WiFi Shield 101](https://docs.arduino.cc/retired/shields/arduino-wifi-shield-101). The sketch to be loaded Over The Air can be stored on two media:

- the internal flash;
- an external SD card;

In the first case the new sketch can't exceed the half of the total memory (256/2 = 128kB) while in the second case you have all the MCU's flash available.

To use this library

```
#include <SPI.h>
#include <SD.h>
#include <WiFi101.h>
#include <WiFi101OTA.h>
#include <SDU.h>
```