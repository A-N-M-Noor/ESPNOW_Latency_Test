#include <WiFi.h>
#include <esp_now.h>

uint8_t receiverMAC[] = {0x98, 0xA3, 0x16, 0x85, 0x08, 0x28}; // MAC Address of the other side

struct DataPack {
  int values[6];
  uint32_t timestamp; // us
};

volatile bool replyReceived = false;
uint32_t latency;
DataPack packet;

void OnDataRecv(const esp_now_recv_info_t *info, const uint8_t *incomingData, int len) {
  if (len == sizeof(DataPack)) {
    DataPack recv;
    memcpy(&recv, incomingData, sizeof(recv));
    latency = micros() - recv.timestamp;
    replyReceived = true;
  }
}

void setup() {
  Serial.begin(115200);
  WiFi.mode(WIFI_STA);

  if (esp_now_init() != ESP_OK) {
    Serial.println("Error initializing ESP-NOW");
    return;
  }

  // Add receiver as a peer
  esp_now_peer_info_t peerInfo{};
  memcpy(peerInfo.peer_addr, receiverMAC, 6);
  peerInfo.channel = 0;
  peerInfo.encrypt = false;
  if (esp_now_add_peer(&peerInfo) != ESP_OK) {
    Serial.println("Peer failed...");
    return;
  }

  esp_now_register_recv_cb(OnDataRecv);
  randomSeed(esp_random());
}

void loop() {
  const int numPings = 50;
  uint32_t totalLatency = 0;
  uint32_t minLatency = UINT32_MAX;
  uint32_t maxLatency = 0;
  uint16_t success = 0;

  for (int i = 0; i < numPings; i++) {
    for (int j = 0; j < 6; j++) packet.values[j] = random(-1000, 1000);
    packet.timestamp = micros();
    replyReceived = false;
    
    if (esp_now_send(receiverMAC, (uint8_t *)&packet, sizeof(packet)) != ESP_OK) {
      Serial.println("Send failed...");
      continue;
    }

    unsigned long start = millis();
    while (!replyReceived && millis() - start < 1000) {
      delay(1);
    }

    if (replyReceived) {
      totalLatency += latency;
      if (latency < minLatency) minLatency = latency;
      if (latency > maxLatency) maxLatency = latency;
      Serial.printf("Ping #%d: %lu us\n", i + 1, latency);
      success++;
    } else {
      Serial.printf("Ping #%d: TIMEOUT\n", i + 1);
    }
  }

  float avgLatency = totalLatency / (float)numPings;

  Serial.println("\n===== Latency Test Results =====");
  Serial.printf("Sent %d data packs with %d bytes\n", numPings, sizeof(packet));
  Serial.printf("Min latency: %lu us\n", minLatency);
  Serial.printf("Max latency: %lu us\n", maxLatency);
  Serial.printf("Avg latency: %.2f us\n", avgLatency);
  Serial.printf("Successfull: %lu / %d\n", success, numPings);

  delay(2000); 
 }
