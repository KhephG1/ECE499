/**
 * LoRa RX/TX + WiFi AP hosting a sensor readout page.
 *
 * Receives the ECE499_node sensor packet. Every radio parameter and the
 * payload layout come from shared/lora_link.h, which the node builds against
 * too -- see that file before changing anything RF-related here.
 */

#define HELTEC_POWER_BUTTON
#include <Arduino.h>
#include <heltec_unofficial.h>
#include <WiFi.h>
#include <WebServer.h>

#include "lora_link.h"

#define PAUSE               300
#define TRANSMIT_POWER      0

#define AP_SSID             "HeltecBase"
#define AP_PASS             "lora1234"   // >= 8 chars, or use NULL for open

void handleRoot();
void rx();

volatile bool rxFlag = false;
long counter = 0;
uint64_t last_tx = 0;
uint64_t tx_time;
uint64_t minimum_pause;

// Shared state for the web page
volatile long   rxCount   = 0;
String          lastPacket = "(none)";
float           lastRSSI  = 0;
float           lastSNR   = 0;

// Latest decoded sensor packet. sensorValid stays false until a packet of
// LORA_LINK_PAYLOAD_LEN bytes arrives, so the page can distinguish "no sensor
// data yet" from "all readings happen to be zero".
bool     sensorValid = false;
uint16_t scdCo2, bmeCo2eq, bmeVoc, bmePress;
int16_t  scdTemp, scdHum, bmeTemp, bmeHum;
uint8_t  bmeStab;

WebServer server(80);

// The payload is little-endian regardless of the host, so it is unpacked byte
// by byte rather than cast over.
static uint16_t get_u16(const uint8_t *p) {
  return (uint16_t)p[0] | ((uint16_t)p[1] << 8);
}

static int16_t get_i16(const uint8_t *p) {
  return (int16_t)get_u16(p);
}

static void decodeSensorPacket(const uint8_t *p) {
  scdCo2   = get_u16(&p[LORA_PL_OFF_SCD_CO2]);
  scdTemp  = get_i16(&p[LORA_PL_OFF_SCD_TEMP]);
  scdHum   = get_i16(&p[LORA_PL_OFF_SCD_HUM]);
  bmeCo2eq = get_u16(&p[LORA_PL_OFF_BME_CO2EQ]);
  bmeVoc   = get_u16(&p[LORA_PL_OFF_BME_VOC]);
  bmeHum   = get_i16(&p[LORA_PL_OFF_BME_HUM]);
  bmeTemp  = get_i16(&p[LORA_PL_OFF_BME_TEMP]);
  bmePress = get_u16(&p[LORA_PL_OFF_BME_PRESS]);
  bmeStab  = p[LORA_PL_OFF_BME_STAB];
  sensorValid = true;
}

static String row(const String &name, const String &value) {
  return "<tr><td>" + name + "</td><td>" + value + "</td></tr>";
}

void handleRoot() {
  String html = "<!DOCTYPE html><html><head>"
                "<meta name='viewport' content='width=device-width,initial-scale=1'>"
                "<meta http-equiv='refresh' content='5'>"
                "<title>ECE499 Base</title>"
                "<style>body{font-family:sans-serif;margin:1em}"
                "table{border-collapse:collapse}"
                "td{border:1px solid #ccc;padding:4px 8px}</style>"
                "</head><body>"
                "<h1>ECE499 Gateway</h1>"
                "<p>Packets received: " + String(rxCount) + "</p>"
                "<p>Last packet: " + lastPacket + "</p>"
                "<p>RSSI: " + String(lastRSSI, 2) + " dBm &nbsp; "
                "SNR: " + String(lastSNR, 2) + " dB</p>";

  if (sensorValid) {
    html += "<h2>Sensors</h2><table>";
    html += row("CO2 (SCD40)", String(scdCo2) + " ppm");
    html += row("Temp (SCD40)", String(scdTemp / 100.0, 2) + " &deg;C");
    html += row("Humidity (SCD40)", String(scdHum / 100.0, 2) + " %RH");
    html += row("CO2 eq (BME680)", String(bmeCo2eq) + " ppm");
    html += row("Breath VOC (BME680)", String(bmeVoc / 100.0, 2) + " ppm");
    html += row("Temp (BME680)", String(bmeTemp / 100.0, 2) + " &deg;C");
    html += row("Humidity (BME680)", String(bmeHum / 100.0, 2) + " %RH");
    html += row("Pressure (BME680)", String(bmePress / 10.0, 1) + " hPa");
    html += row("BSEC stabilized", bmeStab ? "yes" : "warming up");
    html += "</table>";
  } else {
    html += "<p><i>No sensor packet decoded yet.</i></p>";
  }

  html += "</body></html>";
  server.send(200, "text/html", html);
}

void setup() {
  heltec_ve(true);
  delay(100);
  heltec_display_power(true);
  heltec_setup();

  both.println("Radio init");
  RADIOLIB_OR_HALT(radio.begin());
  radio.setDio1Action(rx);
  both.printf("Freq: %.1f MHz\n", LORA_LINK_FREQ_MHZ);
  RADIOLIB_OR_HALT(radio.setFrequency(LORA_LINK_FREQ_MHZ));
  both.printf("BW: %.1f kHz\n", LORA_LINK_BW_KHZ);
  RADIOLIB_OR_HALT(radio.setBandwidth(LORA_LINK_BW_KHZ));
  both.printf("SF: %i  CR: 4/%i\n", LORA_LINK_SF, LORA_LINK_CR_DENOM);
  RADIOLIB_OR_HALT(radio.setSpreadingFactor(LORA_LINK_SF));
  // Never set previously, so this silently sat at RadioLib's 4/7 default
  // while the node transmitted 4/5.
  RADIOLIB_OR_HALT(radio.setCodingRate(LORA_LINK_CR_DENOM));
  RADIOLIB_OR_HALT(radio.setSyncWord(LORA_LINK_SYNC_WORD));
  RADIOLIB_OR_HALT(radio.setPreambleLength(LORA_LINK_PREAMBLE_SYMB));
  both.printf("TX power: %i dBm\n", TRANSMIT_POWER);
  RADIOLIB_OR_HALT(radio.setOutputPower(TRANSMIT_POWER));
  RADIOLIB_OR_HALT(radio.startReceive(RADIOLIB_SX126X_RX_TIMEOUT_INF));

  // --- WiFi AP ---
  WiFi.mode(WIFI_AP);
  WiFi.softAP(AP_SSID, AP_PASS);
  IPAddress ip = WiFi.softAPIP();
  both.printf("AP: %s\n", AP_SSID);
  both.printf("IP: %s\n", ip.toString().c_str());

  server.on("/", handleRoot);
  server.begin();
  both.println("HTTP up");
}

void loop() {
  heltec_loop();
  server.handleClient();          // must be called often

  bool tx_legal = millis() > last_tx + minimum_pause;
  if ((PAUSE && tx_legal && millis() - last_tx > (PAUSE * 1000)) || button.isSingleClick()) {
    if (!tx_legal) {
      both.printf("Legal limit, wait %i sec.\n",
                  (int)((minimum_pause - (millis() - last_tx)) / 1000) + 1);
      return;
    }
    both.printf("TX [%s] ", String(counter).c_str());
    radio.clearDio1Action();
    heltec_led(50);
    tx_time = millis();
    RADIOLIB(radio.transmit(String(counter++).c_str()));
    tx_time = millis() - tx_time;
    heltec_led(0);
    if (_radiolib_status == RADIOLIB_ERR_NONE) {
      both.printf("OK (%i ms)\n", (int)tx_time);
    } else {
      both.printf("fail (%i)\n", _radiolib_status);
    }
    minimum_pause = tx_time * 100;
    last_tx = millis();
    radio.setDio1Action(rx);
    RADIOLIB_OR_HALT(radio.startReceive(RADIOLIB_SX126X_RX_TIMEOUT_INF));
  }

  if (rxFlag) {
    rxFlag = false;

    uint8_t buf[RADIOLIB_SX126X_MAX_PACKET_LENGTH];
    size_t len = radio.getPacketLength();
    if (len > sizeof(buf)) {
      len = sizeof(buf);
    }

    // Read as bytes rather than into a String: the sensor payload is binary
    // and contains zero bytes, which a String would treat as a terminator.
    int16_t status = radio.readData(buf, len);
    if (status == RADIOLIB_ERR_NONE) {
      lastRSSI = radio.getRSSI();
      lastSNR  = radio.getSNR();
      rxCount++;

      if (len == LORA_LINK_PAYLOAD_LEN) {
        decodeSensorPacket(buf);
        lastPacket = "sensor packet (" + String(len) + " B)";
        // The OLED only fits a few lines, so the full set goes to serial.
        both.printf("RX sensors\n");
        both.printf("  CO2 %u ppm\n", scdCo2);
        both.printf("  T %.2f C\n", scdTemp / 100.0);
        Serial.printf("  RH %.2f %%  P %.1f hPa\n", scdHum / 100.0, bmePress / 10.0);
        Serial.printf("  CO2eq %u ppm  VOC %.2f ppm\n", bmeCo2eq, bmeVoc / 100.0);
        Serial.printf("  BME T %.2f C  RH %.2f %%  stab %u\n",
                      bmeTemp / 100.0, bmeHum / 100.0, bmeStab);
      } else {
        // Not a sensor packet -- another board's traffic, or the node and this
        // build disagreeing on the payload length. Show it as text so the
        // mismatch is visible rather than silently dropped.
        String text;
        for (size_t i = 0; i < len; i++) {
          text += isprint(buf[i]) ? (char)buf[i] : '.';
        }
        lastPacket = text + " (" + String(len) + " B, expected " +
                     String(LORA_LINK_PAYLOAD_LEN) + ")";
        both.printf("RX [%s] %u B\n", text.c_str(), (unsigned)len);
      }
      both.printf("  RSSI %.1f SNR %.1f\n", lastRSSI, lastSNR);
    }
    RADIOLIB_OR_HALT(radio.startReceive(RADIOLIB_SX126X_RX_TIMEOUT_INF));
  }
}

void rx() {
  rxFlag = true;
}
