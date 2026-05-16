#include <Arduino.h>
#include <WiFi.h>
#include <esp_now.h>

uint8_t broadcast[] = {0xff,0xff,0xff,0xff,0xff,0xff};

void onReceive(const uint8_t *mac, const uint8_t *data, int len) {
  for (int i = 0; i < len; i++) {
    Serial.write(data[i]);
  }
  Serial.write('\n');
}

void setup() {
  Serial.begin(115200);
  while (!Serial) {
    delay(10);
  }

  WiFi.mode(WIFI_STA);
  WiFi.disconnect(true);
  delay(100);

  if (esp_now_init() != ESP_OK) {
    while (true) {
      delay(1000);
    }
  }

  esp_now_peer_info_t peer = {};
  memcpy(peer.peer_addr, broadcast, 6);
  peer.channel = 0;
  peer.encrypt = false;

  if (!esp_now_is_peer_exist(broadcast)) {
    esp_now_add_peer(&peer);
  }

  esp_now_register_recv_cb(onReceive);
}

void loop() {
  static uint8_t buf[256];

  if (Serial.available()) {
    int n = Serial.readBytesUntil('\n', (char*)buf, sizeof(buf)-1);

    if (n > 0) {
      buf[n] = 0;
      esp_now_send(broadcast, buf, n);
    }
  }
}
