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

#include <Arduino.h>

#include "WiFi101OTA.h"

#if defined(ARDUINO_SAMD_ZERO)
  #define BOARD "arduino_zero_edbg"
#elif defined(ARDUINO_SAMD_MKR1000)
  #define BOARD "mkr1000"
#elif defined(ARDUINO_SAMD_MKRZERO)
  #define BOARD "mkrzero"
#else
  #error "Unsupported board!"
#endif

#define BOARD_LENGTH (sizeof(BOARD) - 1)

// Important RFC's for reference:
// - DNS request and response: http://www.ietf.org/rfc/rfc1035.txt
// - Multicast DNS: http://www.ietf.org/rfc/rfc6762.txt

const uint8_t _REQUEST_HEADER[] PROGMEM = {
  0x00, 0x00, // Transaction ID = 0
  0x00, 0x00, // Flags = None
  0x00, 0x01, // Questions = 1 (these 2 bytes are ignored)
  0x00, 0x00, // Answer RRs = 0
  0x00, 0x00, // Authority RRs = 0
  0x00, 0x00  // Additional RRs = 0 (these 2 bytes are ignored)
};

const uint8_t _ARDUINO_SERVICE_REQUEST[] PROGMEM = {
  0x08,
  0x5f, 0x61, 0x72, 0x64, 0x75, 0x69, 0x6e, 0x6f, // _arduino
  0x04,
  0x5f, 0x74, 0x63, 0x70,                         // _tcp
  0x05,
  0x6c, 0x6f, 0x63, 0x61, 0x6c,                   // local
  0x00,
  0x00, 0x0c,                                     // Type = 12 (PTR record)
  0x00, 0x01                                      // Class = 1 (Internet)
};

const uint8_t _ARDUINO_SERVICE_RESPONSE_HEADER[] PROGMEM = {
  0x00, 0x00, // Transaction ID = 0
  0x84, 0x00, // Flags = Response + Authoritative Answer
  0x00, 0x00, // Questions = 0
  0x00, 0x04, // Answers = 4
  0x00, 0x00, // Nameserver Records = 0
  0x00, 0x01  // Additional Records = 1
};

const uint8_t _ADDRESS_RESPONSE_HEADER[] PROGMEM = {
  0x00, 0x00, // Transaction ID = 0
  0x84, 0x00, // Flags = Response + Authoritative Answer
  0x00, 0x00, // Questions = 0
  0x00, 0x01, // Answers = 1
  0x00, 0x00, // Nameserver Records = 0
  0x00, 0x01  // Additional Records = 1
};

const uint8_t _PTR_RECORD[] PROGMEM = {
  0x08,
  0x5f, 0x61, 0x72, 0x64, 0x75, 0x69, 0x6e, 0x6f, // _arduino
  0x04,
  0x5f, 0x74, 0x63, 0x70,                         // _tcp
  0x05,
  0x6c, 0x6f, 0x63, 0x61, 0x6c,                   // local
  0x00,
  0x00, 0x0c,                                     // Type = 12 (PTR record)
  0x00, 0x01,                                     // Class = 1 (Internet)
  0x00, 0x00, 0x0E, 0x10,                         // TTL = 3600 seconds
  0x00, 0x00                                      // Record Length (to be filled in later)
  // ...                                          // Record Data (to be appended later)
};

const uint8_t _TXT_RECORD[] PROGMEM = {
  // ...                                          // Name (to be prepended later)
  0x00, 0x10,                                     // Type = 16 (TXT record)
  0x80, 0x01,                                     // Class = 1 (Internet, with cache flush bit)
  0x00, 0x00, 0x0E, 0x10,                         // TTL = 3600 seconds
  0x00, 0x00                                      // Record Length (to be filled in later)
  // ...                                          // Record Data (to be appended later)
};

const uint8_t _SRV_RECORD[] PROGMEM = {
  // ...                                          // Name (to be prepended later)
  0x00, 0x21,                                     // Type = 16 (SRV record)
  0x80, 0x01,                                     // Class = 1 (Internet, with cache flush bit)
  0x00, 0x00, 0x0E, 0x10,                         // TTL = 3600 seconds
  0x00, 0x00,                                     // Record Length (to be filled in later)
  0x00, 0x00,                                     // Record Priority = 0
  0x00, 0x00,                                     // Record Weight = 0
  0xff, 0x00                                      // Record Port = 65280
  // ...                                          // Record Target (to be appended later)
};

// Generate positive response for IPV4 address
const uint8_t _A_RECORD[] PROGMEM = {
  // ...                                          // Name (to be prepended later)
  0x00, 0x01,                                     // Type = 1 (A record/IPV4 address)
  0x80, 0x01,                                     // Class = 1 (Internet)
  0x00, 0x00, 0x0E, 0x10,                         // TTL = 3600 seconds
  0x00, 0x04,                                     // Record Length = 4 bytes
  0x00, 0x00, 0x00, 0x00                          // Record Data (to be filled in later)
};

// Generate negative response for IPV6 address (CC3000 doesn't support IPV6)
const uint8_t _NSEC_RECORD[] PROGMEM = {
  // ...                                          // Name (to be prepended later)
  0x00, 0x2F,                                     // Type = 47 (NSEC, overloaded by MDNS)
  0x80, 0x01,                                     // Class = 1 (Internet, with cache flush bit)
  0x00, 0x00, 0x0E, 0x10,                         // TTL = 3600 seconds
  0x00, 0x08,                                     // Record Length
  0xC0, 0x0C,                                     // Next Domain = Pointer to FQDN
  0x00,                                           // Block Number = 0
  0x04,                                           // Bitmap Length = 4 bytes
  0x40, 0x00, 0x00, 0x00                          // Bitmap Value = Only first bit (A record/IPV4) is set
};

const uint8_t _ARDUINO_DOMAIN[] PROGMEM = {
  0x08,
  0x5f, 0x61, 0x72, 0x64, 0x75, 0x69, 0x6e, 0x6f, // _arduino
  0x04,
  0x5f, 0x74, 0x63, 0x70,                         // _tcp
  0x05,
  0x6c, 0x6f, 0x63, 0x61, 0x6c,                   // local
  0x00
};

const uint8_t _DOMAIN[] PROGMEM = {
  0x05,
  0x6c, 0x6f, 0x63, 0x61, 0x6c,                   // local
  0x00
};

static String base64Encode(const String& in)
{
  static const char* _CODES = "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/=";

  int b;
  String out;
  out.reserve((in.length()) * 4 / 3);

  for (unsigned int i = 0; i < in.length(); i += 3) {
    b = (in.charAt(i) & 0xFC) >> 2;
    out += _CODES[b];

    b = (in.charAt(i) & 0x03) << 4;
    if (i + 1 < in.length()) {
      b |= (in.charAt(i + 1) & 0xF0) >> 4;
      out += _CODES[b];
      b = (in.charAt(i + 1) & 0x0F) << 2;
      if (i + 2 < in.length()) {
         b |= (in.charAt(i + 2) & 0xC0) >> 6;
         out += _CODES[b];
         b = in.charAt(i + 2) & 0x3F;
         out += _CODES[b];
      } else {
        out += _CODES[b];
        out += '=';
      }
    } else {
      out += _CODES[b];
      out += "==";
    }
  }

  return out;
}

WiFiOTAClass::WiFiOTAClass() :
  _storage(NULL),
  _server(65280),
  _arduinoServiceRequestLength(0),
  _minimumResolveRequestLength(0),
  _lastArduinoServiceResponseTime(0)
{
}

bool WiFiOTAClass::begin(const char* name, const char* password, OTAStorage& storage)
{
  uint8_t nameLength = strlen(name);

  if (nameLength > 255) {
    // Can only handle names that are upto 255 chars in length
    return false;
  }

  _name = name;
  _name.toLowerCase();
  _expectedAuthorization = "Basic " + base64Encode("arduino:" + String(password));
  _storage = &storage;

  _arduinoServiceRequestLength = sizeof(_REQUEST_HEADER) + sizeof(_ARDUINO_SERVICE_REQUEST);
  _minimumResolveRequestLength = sizeof(_REQUEST_HEADER) + 1 + nameLength + sizeof(_DOMAIN) + 4;

  // Start the HTTP server
  _server.begin();

  // Open the MDNS UDP listening socket on port 5353 with multicast address
  // 224.0.0.251 (0xE00000FB)
  if (!_mdnsSocket.beginMulticast(IPAddress(224, 0, 0, 251), 5353)) {
    return false;
  }

  return true;
}

void WiFiOTAClass::poll()
{
  pollMdns();
  pollServer();
}

bool WiFiOTAClass::pollMdns()
{
  int packetLength = _mdnsSocket.parsePacket();

  int requestSize = 0;

  // Check if the parsed packet is an expected request length
  if (packetLength == _arduinoServiceRequestLength) {
    requestSize = _arduinoServiceRequestLength;
  } else if (packetLength >= _minimumResolveRequestLength) {
    requestSize = _minimumResolveRequestLength;
  }

  if (!requestSize) {
    // It is not, read the full packet in and drop data
    while(_mdnsSocket.available()) {
      _mdnsSocket.read();
    }

    return false;
  }

  // Read up to the minimum request length
  uint8_t request[requestSize];
  _mdnsSocket.read(request, requestSize);

  // Read the rest of the packet in and drop data
  while(_mdnsSocket.available()) {
    _mdnsSocket.read();
  }

  // Check request header
  if (memcmp_P(request, _REQUEST_HEADER, 4) != 0 || memcmp_P(&request[6], &_REQUEST_HEADER[6], 4) != 0) {
    return false;
  }

  // Determine request type
  if (packetLength == _arduinoServiceRequestLength && memcmp_P(&request[sizeof(_REQUEST_HEADER)], _ARDUINO_SERVICE_REQUEST, sizeof(_ARDUINO_SERVICE_REQUEST)) == 0) {
    // Arduino Service Request
    if ((millis() - _lastArduinoServiceResponseTime) < 1000) {
      // Ignore this service request, too frequent
      return false;
    }

    _lastArduinoServiceResponseTime = millis();
    replyToMdnsRequest(0x000c);
  } else {
    // Address Request

    // Define labels for comparison
    uint8_t nameLength = _name.length();

    uint8_t nameLabel[1 + nameLength];
    nameLabel[0] = nameLength;
    memcpy(&nameLabel[1], _name.c_str(), nameLength);

    uint8_t fqdnLabels[sizeof(nameLabel) + sizeof(_DOMAIN)];
    memcpy(&fqdnLabels[0], nameLabel, sizeof(nameLabel));
    memcpy_P(&fqdnLabels[sizeof(nameLabel)], _DOMAIN, sizeof(_DOMAIN));
    
    // Extract type and class
    uint16_t requestQType;
    uint16_t requestQClass;
    memcpy(&requestQType, &request[_minimumResolveRequestLength - 4], sizeof(requestQType));
    memcpy(&requestQClass, &request[_minimumResolveRequestLength - 2], sizeof(requestQClass));
    
    requestQType = _ntohs(requestQType);
    requestQClass = _ntohs(requestQClass);

    // Check the requested address is for me
    if (memcmp(&request[sizeof(_REQUEST_HEADER)], fqdnLabels, sizeof(fqdnLabels)) == 0 && requestQClass == 0x0001) {
      replyToMdnsRequest(requestQType);
    }
  }
}

void WiFiOTAClass::replyToMdnsRequest(uint16_t requestQType)
{
  // Define labels
  uint8_t nameLength = _name.length();

  uint8_t nameLabel[1 + nameLength];
  nameLabel[0] = nameLength;
  memcpy(&nameLabel[1], _name.c_str(), nameLength);

  uint8_t fqdnLabels[sizeof(nameLabel) + sizeof(_DOMAIN)];
  memcpy(&fqdnLabels[0], nameLabel, sizeof(nameLabel));
  memcpy_P(&fqdnLabels[sizeof(nameLabel)], _DOMAIN, sizeof(_DOMAIN));

  uint8_t arduinoLabels[sizeof(nameLabel) + sizeof(_ARDUINO_DOMAIN)];
  memcpy(&arduinoLabels[0], nameLabel, sizeof(nameLabel));
  memcpy_P(&arduinoLabels[sizeof(nameLabel)], _ARDUINO_DOMAIN, sizeof(_ARDUINO_DOMAIN));

  // Build PTR record
  uint8_t ptrRecord[sizeof(_PTR_RECORD) + sizeof(arduinoLabels)];
  memcpy_P(&ptrRecord[0], _PTR_RECORD, sizeof(_PTR_RECORD));
  memcpy(&ptrRecord[sizeof(_PTR_RECORD)], arduinoLabels, sizeof(arduinoLabels));

  uint16_t ptrRecordDataLength = _htons(sizeof(arduinoLabels));
  memcpy(&ptrRecord[sizeof(_PTR_RECORD) - 2], &ptrRecordDataLength, 2);

  // Build SRV record
  uint8_t srvRecord[sizeof(arduinoLabels) + sizeof(_SRV_RECORD) + sizeof(fqdnLabels)];
  memcpy(&srvRecord[0], arduinoLabels, sizeof(arduinoLabels));
  memcpy_P(&srvRecord[sizeof(arduinoLabels)], _SRV_RECORD, sizeof(_SRV_RECORD));
  memcpy(&srvRecord[sizeof(arduinoLabels) + sizeof(_SRV_RECORD)], fqdnLabels, sizeof(fqdnLabels));

  uint16_t srvRecordDataLength = _htons(6 + sizeof(fqdnLabels));
  memcpy(&srvRecord[sizeof(arduinoLabels) + sizeof(_SRV_RECORD) - 8], &srvRecordDataLength, 2);

  // Build TXT record
  uint8_t txtRecordData[50 + BOARD_LENGTH] = {
    13,
    's', 's', 'h', '_', 'u', 'p', 'l', 'o', 'a', 'd', '=', 'n', 'o',
    12,
    't', 'c', 'p', '_', 'c', 'h', 'e', 'c', 'k', '=', 'n', 'o',
    15,
    'a', 'u', 't', 'h', '_', 'u', 'p', 'l', 'o', 'a', 'd', '=', 'y', 'e', 's',
    (6 + BOARD_LENGTH),
    'b', 'o', 'a', 'r', 'd', '=',
  };
  memcpy(&txtRecordData[sizeof(txtRecordData) - BOARD_LENGTH], BOARD, BOARD_LENGTH);

  uint8_t txtRecord[sizeof(arduinoLabels) + sizeof(_TXT_RECORD) + sizeof(txtRecordData)];
  memcpy(&txtRecord[0], arduinoLabels, sizeof(arduinoLabels));
  memcpy_P(&txtRecord[sizeof(arduinoLabels)], _TXT_RECORD, sizeof(_TXT_RECORD));
  memcpy(&txtRecord[sizeof(arduinoLabels) + sizeof(_TXT_RECORD)], txtRecordData, sizeof(txtRecordData));

  uint16_t txtRecordDataLength = _htons(sizeof(txtRecordData));
  memcpy(&txtRecord[sizeof(arduinoLabels) + sizeof(_TXT_RECORD) - 2], &txtRecordDataLength, 2);

  // Build A record
  uint8_t aRecord[sizeof(fqdnLabels) + sizeof(_A_RECORD)];
  memcpy(&aRecord[0], fqdnLabels, sizeof(fqdnLabels));
  memcpy_P(&aRecord[sizeof(fqdnLabels)], _A_RECORD, sizeof(_A_RECORD));

  uint32_t ipAddress = WiFi.localIP();
  memcpy(&aRecord[sizeof(fqdnLabels) + sizeof(_A_RECORD) - 4], &ipAddress, 4);

  // Build NSEC record
  uint8_t nsecRecord[sizeof(fqdnLabels) + sizeof(_NSEC_RECORD)];
  memcpy(&nsecRecord[0], fqdnLabels, sizeof(fqdnLabels));
  memcpy_P(&nsecRecord[sizeof(fqdnLabels)], _NSEC_RECORD, sizeof(_NSEC_RECORD));

   // Write packet
  _mdnsSocket.beginPacket(IPAddress(224, 0, 0, 251), 5353);

  if (requestQType == 0x000c) {
    _mdnsSocket.write(_ARDUINO_SERVICE_RESPONSE_HEADER, sizeof(_ARDUINO_SERVICE_RESPONSE_HEADER));
    _mdnsSocket.write(ptrRecord, sizeof(ptrRecord));
    _mdnsSocket.write(srvRecord, sizeof(srvRecord));
    _mdnsSocket.write(txtRecord, sizeof(txtRecord));
    _mdnsSocket.write(aRecord, sizeof(aRecord));
    _mdnsSocket.write(nsecRecord, sizeof(nsecRecord));
  } else if (requestQType == 0x0001) {
    _mdnsSocket.write(_ADDRESS_RESPONSE_HEADER, sizeof(_ADDRESS_RESPONSE_HEADER));
    _mdnsSocket.write(aRecord, sizeof(aRecord));
    _mdnsSocket.write(nsecRecord, sizeof(nsecRecord));
  } else {
    _mdnsSocket.write(_ADDRESS_RESPONSE_HEADER, sizeof(_ADDRESS_RESPONSE_HEADER));
    _mdnsSocket.write(nsecRecord, sizeof(nsecRecord));
    _mdnsSocket.write(aRecord, sizeof(aRecord));
  }

  _mdnsSocket.endPacket();
}

void WiFiOTAClass::pollServer()
{
  WiFiClient client = _server.available();

  if (client) {
    String request = client.readStringUntil('\n');
    request.trim();

    String header;
    long contentLength = -1;
    String authorization;

    do {
      header = client.readStringUntil('\n');
      header.trim();

      if (header.startsWith("Content-Length: ")) {
        header.remove(0, 16);

        contentLength = header.toInt();
      } else if (header.startsWith("Authorization: ")) {
        header.remove(0, 15);

        authorization = header;
      }
    } while (header != "");

    if (request != "POST /sketch HTTP/1.1") {
      flushRequestBody(client, contentLength);
      sendHttpResponse(client, 404, "Not Found");
      return;
    }

    if (_expectedAuthorization != authorization) {
      flushRequestBody(client, contentLength);
      sendHttpResponse(client, 401, "Unauthorized");
      return;
    }

    if (contentLength <= 0) {
      sendHttpResponse(client, 400, "Bad Request");
      return;
    }

    if (_storage == NULL || !_storage->open()) {
      flushRequestBody(client, contentLength);
      sendHttpResponse(client, 500, "Internal Server Error");
      return;
    }

    if (contentLength > _storage->maxSize()) {
      _storage->close();
      flushRequestBody(client, contentLength);
      sendHttpResponse(client, 413, "Payload Too Large");
      return;
    }

    long read = 0;

    while (client.connected() && read < contentLength) {
      if (client.available()) {
        read++;

        _storage->write((char)client.read());
      }
    }

    _storage->close();

    if (read == contentLength) {
      sendHttpResponse(client, 200, "OK");

      delay(250);

      // apply the update
      _storage->apply();

      while (true);
    } else {
      _storage->clear();

      client.stop();
    }
  }
}

void WiFiOTAClass::sendHttpResponse(Client& client, int code, const char* status)
{
  while (client.available()) {
    client.read();
  }

  client.print("HTTP/1.1 ");
  client.print(code);
  client.print(" ");
  client.println(status);
  client.println("Connection: close");
  client.println();
  client.stop();
}

void WiFiOTAClass::flushRequestBody(Client& client, long contentLength)
{
  long read = 0;

  while (client.connected() && read < contentLength) {
    if (client.available()) {
      read++;

      client.read();
    }
  }
}

WiFiOTAClass WiFiOTA;
