/*
  Copyright (c) 2017 Arduino LLC.  All right reserved.

  This library is free software; you can redistribute it and/or
  modify it under the terms of the GNU Lesser General Public
  License as published by the Free Software Foundation; either
  version 2.1 of the License, or (at your option) any later version.

  This library is distributed in the hope that it will be useful,
  but WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
  See the GNU Lesser General Public License for more details.

  You should have received a copy of the GNU Lesser General Public
  License along with this library; if not, write to the Free Software
  Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA  02110-1301  USA
*/

#ifndef _WIFI101_OTA_H_INCLUDED
#define _WIFI101_OTA_H_INCLUDED

#include <Arduino.h>

#if defined(ARDUINO_SAMD_ZERO)
  #include "WiFi101.h"
  #define BOARD "arduino_zero_edbg"
#elif defined(ARDUINO_SAMD_MKR1000)
  #include "WiFi101.h"
  #define BOARD "mkr1000"
#elif defined(ARDUINO_SAMD_MKRZERO)
  #include "WiFi101.h"
  #define BOARD "mkrzero"
#elif defined(ARDUINO_SAMD_MKRWIFI1010)
  #include "WiFiNINA.h"
  #define BOARD "mkrwifi1010"
#elif defined(ARDUINO_SAMD_NANO_33_IOT)
  #include "WiFiNINA.h"
  #define BOARD "nano_33_iot"
#elif defined(ARDUINO_SAMD_MKRVIDOR4000)
  #include "WiFiNINA.h"
  #define BOARD "mkrvidor4000"
#elif defined(ARDUINO_AVR_UNO_WIFI_REV2) || defined(ARDUINO_MEGAAVR_UNO_WIFI_REV2)
  #include "WiFiNINA.h"
  #define BOARD "uno2018"
#else
  #error "Unsupported board!"
#endif

#include "WiFiUdp.h"

#include "OTAStorage.h"
#include "SDStorage.h"
#include "InternalStorage.h"
#include "SerialFlashStorage.h"
#include "NINAStorage.h"

class WiFiOTAClass {
public:
  WiFiOTAClass();

  void begin(const char* name, const char* password, OTAStorage& storage);
  void poll();

private:
  void pollMdns();
  void pollServer();
  void sendHttpResponse(Client& client, int code, const char* status);
  void flushRequestBody(Client& client, long contentLength);

private:
  String _name;
  String _expectedAuthorization;
  OTAStorage* _storage;
  WiFiServer _server;
  WiFiUDP _mdnsSocket;

  uint32_t _lastMdnsResponseTime;
};

extern WiFiOTAClass WiFiOTA;

#endif
