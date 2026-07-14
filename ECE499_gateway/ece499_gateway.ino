


/**
 * LoRa RX/TX + WiFi AP hosting a hello-world page.
 */

#define HELTEC_POWER_BUTTON
#include <heltec_unofficial.h>
#include <WiFi.h>
#include <WebServer.h>

#define PAUSE               300
#define FREQUENCY           905.2
#define BANDWIDTH           250.0
#define SPREADING_FACTOR    9
#define TRANSMIT_POWER      0

#define AP_SSID             "HeltecBase"
#define AP_PASS             "lora1234"   // >= 8 chars, or use NULL for open

String rxdata;
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

WebServer server(80);

void handleRoot() {
  String html = "<!DOCTYPE html><html><head>"
                "<meta name='viewport' content='width=device-width,initial-scale=1'>"
                "<meta http-equiv='refresh' content='5'>"
                "<title>Heltec Base</title></head><body>"
                "<h1>Hello World</h1>"
                "<p>Packets received: " + String(rxCount) + "</p>"
                "<p>Last packet: " + lastPacket + "</p>"
                "<p>RSSI: " + String(lastRSSI, 2) + " dBm</p>"
                "<p>SNR: " + String(lastSNR, 2) + " dB</p>"
                "</body></html>";
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
  both.printf("Frequency: %.2f MHz\n", FREQUENCY);
  RADIOLIB_OR_HALT(radio.setFrequency(FREQUENCY));
  both.printf("Bandwidth: %.1f kHz\n", BANDWIDTH);
  RADIOLIB_OR_HALT(radio.setBandwidth(BANDWIDTH));
  both.printf("Spreading Factor: %i\n", SPREADING_FACTOR);
  RADIOLIB_OR_HALT(radio.setSpreadingFactor(SPREADING_FACTOR));
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
    radio.readData(rxdata);
    if (_radiolib_status == RADIOLIB_ERR_NONE) {
      lastPacket = rxdata;
      lastRSSI   = radio.getRSSI();
      lastSNR    = radio.getSNR();
      rxCount++;
      both.printf("RX [%s]\n", rxdata.c_str());
      both.printf("  RSSI: %.2f dBm\n", lastRSSI);
      both.printf("  SNR: %.2f dB\n", lastSNR);
    }
    RADIOLIB_OR_HALT(radio.startReceive(RADIOLIB_SX126X_RX_TIMEOUT_INF));
  }
}

void rx() {
  rxFlag = true;
}