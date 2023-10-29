#include <ESP8266WiFi.h>
#include <WiFiClient.h>
#include <EEPROM.h>

#define UART_BAUD 115200
#define BUFFER_SIZE 1024
#define SERIAL_TIMEOUT 5
#define ONBOARD_LED 2
#define BLINK_DURATION 20

const char* ssid = "YOUR_SSID";
const char* password = "YOUR_PASSWORD";
const int port = 12345;

EEPROMClass eeprom;
WiFiServer *server;
WiFiClient client;
uint8_t buffer_rx[BUFFER_SIZE];
uint8_t buffer_tx[BUFFER_SIZE];
unsigned long blink_time;
bool blink_triggered;

enum Status
{
  NO_WIFI, NO_CLIENT, READY
} status;

void indicate_status()
{
  unsigned long ms = millis();
  unsigned long blink_duration = ms - blink_time;
  uint8_t led_on = false;
  switch (status)
  {
    case NO_WIFI:
      led_on = (ms >> 8) & 1;
      break;
    case NO_CLIENT:
      led_on = true;
      blink_triggered = false;
      break;
    case READY:
      led_on = blink_triggered ? blink_duration <= BLINK_DURATION : false;
      break;
  }
  if (blink_triggered && blink_duration > BLINK_DURATION * 2) blink_triggered = false;
  digitalWrite(ONBOARD_LED, led_on ? LOW : HIGH);
}

void trigger_blink()
{
  if (!blink_triggered)
  {
    blink_triggered = true;
    blink_time = millis();
  }
}

void setup()
{
  status = Status::NO_WIFI;
  pinMode(ONBOARD_LED, OUTPUT);
  digitalWrite(ONBOARD_LED, HIGH);
  Serial.begin(UART_BAUD);
  Serial.setTimeout(SERIAL_TIMEOUT);
  WiFi.mode(WIFI_STA);
  WiFi.begin(ssid, password);
  server = new WiFiServer(port);
  server->begin();
}

void loop()
{  
  indicate_status();
  if (WiFi.status() != WL_CONNECTED)
  {
    status = Status::NO_WIFI;
    return;
  }
  status = Status::NO_CLIENT;
  if(!client.connected())
  {
    client = server->available();
    if (!client)
    {
      return;
    }
    client.setTimeout(0);
  }
  status = Status::READY;
  size_t bytes_read = client.readBytes(buffer_tx, min(BUFFER_SIZE, Serial.availableForWrite()));
  Serial.write(buffer_tx, bytes_read);
  if (bytes_read) trigger_blink();
  if (Serial.available())
  {
    bytes_read = Serial.readBytes(buffer_rx, min(BUFFER_SIZE, client.availableForWrite()));
    client.write(buffer_rx, bytes_read);
    if (bytes_read) trigger_blink();
  }
}
