# WiFi101 OTA library

### `WiFiOTA.begin()`

#### Description

Initializes the WiFi101 OTA library specifying board's network name, password and storage media

#### Syntax

```
WiFiOTA.begin(name, password, storage);
```

#### Parameters

**name:** the board's name to visualize in the network ports list

**password:** the password to insert for the remote upload

**storage:** the storage media to be used. Can be one between:

- InternalStorage
- SDStorage

#### Example

```
#include <SPI.h>
#include <WiFi101.h>
#include <WiFi101OTA.h>

char ssid[] = "yourNetwork";      // your network SSID (name)
char pass[] = "secretPassword";   // your network password

int status = WL_IDLE_STATUS;

void setup() {
  //Initialize serial:
  Serial.begin(9600);

  // check for the presence of the shield:
  if (WiFi.status() == WL_NO_SHIELD) {
    Serial.println("WiFi shield not present");
    // don't continue:
    while (true);
  }

  // attempt to connect to WiFi network:
  while ( status != WL_CONNECTED) {
    Serial.print("Attempting to connect to SSID: ");
    Serial.println(ssid);
    // Connect to WPA/WPA2 network. Change this line if using open or WEP network:
    status = WiFi.begin(ssid, pass);
  }

  // start the WiFi OTA library with internal (flash) based storage
  WiFiOTA.begin("Arduino", "password", InternalStorage);
}

void loop() {
  // check for WiFi OTA updates
  WiFiOTA.poll();
}
```

### `WiFiOTA.poll()`

#### Description

Checks for WiFi OTA updates

#### Syntax

```
WiFiOTA.poll();
```

#### Parameters

None

#### Example

```
#include <SPI.h>
#include <WiFi101.h>
#include <WiFi101OTA.h>

char ssid[] = "yourNetwork";      // your network SSID (name)
char pass[] = "secretPassword";   // your network password

int status = WL_IDLE_STATUS;

void setup() {
  //Initialize serial:
  Serial.begin(9600);

  // check for the presence of the shield:
  if (WiFi.status() == WL_NO_SHIELD) {
    Serial.println("WiFi shield not present");
    // don't continue:
    while (true);
  }

  // attempt to connect to WiFi network:
  while ( status != WL_CONNECTED) {
    Serial.print("Attempting to connect to SSID: ");
    Serial.println(ssid);
    // Connect to WPA/WPA2 network. Change this line if using open or WEP network:
    status = WiFi.begin(ssid, pass);
  }

  // start the WiFi OTA library with internal (flash) based storage
  WiFiOTA.begin("Arduino", "password", InternalStorage);
}

void loop() {
  // check for WiFi OTA updates
  WiFiOTA.poll();
}
```