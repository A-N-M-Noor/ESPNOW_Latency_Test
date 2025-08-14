#include <WiFi.h>
#include <esp_now.h>

struct DataPack {
  int values[6];
  uint32_t timestamp; // us
};

void OnDataRecv(const esp_now_recv_info_t *recv_info, const uint8_t *incomingData, int len) {
  if (len != sizeof(DataPack)) return;

  DataPack vals;
  memcpy(&vals, incomingData, sizeof(vals));

  Serial.print("Incoming Values: ");
  for (int i = 0; i < 6; i++) {
    Serial.print(vals.values[i]);
    Serial.print(" ");
  }
  Serial.println();

  esp_now_peer_info_t peerInfo{};
  memcpy(peerInfo.peer_addr, recv_info->src_addr, 6);
  peerInfo.channel = 0;
  peerInfo.encrypt = false;
  esp_err_t addStatus = esp_now_add_peer(&peerInfo);
  if (addStatus != ESP_OK && addStatus != ESP_ERR_ESPNOW_EXIST) {
    Serial.println("Peer failed...");
    return;
  }

  esp_err_t stat = esp_now_send(recv_info->src_addr, (uint8_t *)&vals, sizeof(vals));
  if (stat == ESP_OK) {
    Serial.println("Sent back...");
  } else {
    Serial.println("Failed to send data...");
  }
}

void setup() {
  Serial.begin(115200);
  WiFi.mode(WIFI_STA);

  Serial.print("MAC: ");
  Serial.println(WiFi.macAddress());

  if (esp_now_init() != ESP_OK) {
    Serial.println("Error initializing ESP-NOW");
    return;
  }

  esp_now_register_recv_cb(OnDataRecv);
}

void loop() {
  
}
