// ===== 1 - PACKET BLUEPRINT AND GLOBAL STATE =====
// --> What does a message look like, and how does the board remember what it has already heard?

// Lightweight Bluetooth Low-Energy (BLE) library for ESP32
#include <NimBLEDevice.h>

// 0xFFFF is reserved by the Bluetooth standard for testing/custom protocols.
// We use it as a "team tag" so our boards ignore phones, laptops, and watches in the room.
#define COMPANY_ID 0xFFFF

// Force the compiler to pack variables tightly with NO empty padding bytes between them.
// Total size = 2 + 1 + 1 + 4 = 8 bytes (must stay <= 24 bytes to fit in a BLE broadcast).
#pragma pack(push, 1)
struct MeshPacket {
uint16_t companyId;  // 2 bytes: Project filter tag (0xFFFF)
  uint8_t  senderId;   // 1 byte:  Unique ID (0-255) identifying which ESP32 sent this
  uint8_t  seq;        // 1 byte:  Packet counter (0-255) used to ignore repeated broadcasts
  float    value;      // 4 bytes: The actual data payload (e.g., sensor reading or state)
};
#pragma pack(pop)

MeshPacket myPacket;             // The outgoing packet this specific board will broadcast
uint8_t lastSeenSeq[256] = {0};  // Memory table: stores the last 'seq' heard from each senderId (0-255)

// Global pointers to control the ESP32's BLE transmitter (Advertising) and receiver (Scan)
NimBLEAdvertising* pAdvertising = nullptr;
NimBLEScan* pScan = nullptr;






// ===== 2 - RECEIVER =====
// Event-driven rather than looped-code that runs line-by-line on a timer

// Event handler class: NimBLE automatically calls these functions in the background whenever the radio hears a BLE broadcast.
class PeerScanCallbacks : public NimBLEScanCallbacks {
  
  // Triggered every time ANY nearby BLE device broadcasts a packet
  void onResult(const NimBLEAdvertisedDevice* device) override {
    // 1. Ignore devices that aren't sending custom Manufacturer Data
    if (!device->haveManufacturerData()) return;

    // 2. Grab the raw bytes and verify the length matches our 8-byte MeshPacket
    std::string mfg = device->getManufacturerData();
    if (mfg.size() != sizeof(MeshPacket)) return;

    // 3. Copy the raw bytes into a structured MeshPacket variable so we can read its fields
    MeshPacket incoming;
    memcpy(&incoming, mfg.data(), sizeof(MeshPacket));

    // 4. Filter out other brands' devices (wrong companyId) and ignore our own ID
    if (incoming.companyId != COMPANY_ID || incoming.senderId == myPacket.senderId) {
      return;
    }

    // 5. Deduplication: Transmitters repeat the same packet ~15 times per update.
    // Only process this packet if its sequence number is different from the last one we saw.
    if (incoming.seq != lastSeenSeq[incoming.senderId]) {
      lastSeenSeq[incoming.senderId] = incoming.seq;  // Update our notebook with the new seq

      // Print who sent it, the packet number, the data value, and received signal-strength indication (RSSI) in decibel-millis (mW in dB)
      Serial.printf("[RX] Node 0x%02X | Seq: %3u | Value: %.2f | RSSI: %d dBm\n",
                    incoming.senderId, incoming.seq, incoming.value, device->getRSSI());
    }
  }

// Safety net: If scanning ever stops or times out, immediately restart it
  void onScanEnd(const NimBLEScanResults& results, int reason) override {
    pScan->start(0, false, true);
  }
} scanCallbacks;  // Create one global instance of our callback handler







// ===== 3 - UPDATING OF THE OUTGOING BROADCAST =====

// Packages a new data value into our MeshPacket and updates the live BLE broadcast
void updateBroadcast(float newValue) {
  myPacket.seq++;            // Increment packet counter (rolls over naturally from 255 -> 0)
  myPacket.value = newValue; // Store the latest sensor/robot value in our packet

  // Build a fresh BLE advertising payload container
  NimBLEAdvertisementData advData;

  // Set mandatory Bluetooth flags: General Discoverable + BLE-only (no Classic Bluetooth)
  advData.setFlags(BLE_HS_ADV_F_DISC_GEN | BLE_HS_ADV_F_BREDR_UNSUP);

  // Convert our 8-byte MeshPacket struct into raw bytes and attach as Manufacturer Data
  advData.setManufacturerData(std::string((char*)&myPacket, sizeof(myPacket)));

  // Load the new payload into the transmitter and swap it live without stopping the radio
  pAdvertising->setAdvertisementData(advData);
  pAdvertising->refreshAdvertisingData();
}

// ===== 4 - RADIO AND IDENTITY INITIALISATION =====

void setup() {
  Serial.begin(115200);  // Start USB serial communication at 115,200 bits/sec

  // --- 1. Automatic Unique ID Generation ---
  // Read the permanent 48-bit factory MAC address burned into the ESP32's silicon
  uint64_t chipMac = ESP.getEfuseMac();
  // Combine two bytes of the MAC into a single 8-bit ID (0-255) so all 10 boards can share identical code
  uint8_t id = ((chipMac >> 40) ^ (chipMac >> 32)) & 0xFF;
  if (id == 0) id = 1;  // Reserve 0x00 so our first packet (seq = 1) never matches the initial 0 array

  myPacket.companyId = COMPANY_ID;
  myPacket.senderId  = id;
  myPacket.seq       = 0;

  // --- 2. Initialize BLE Radio Hardware ---
  NimBLEDevice::init("");    // Empty string = broadcast no device name (saves bytes in our 31-byte packet)
  NimBLEDevice::setPower(9); // Set transmitter power to maximum (+9 dBm) for best indoor range

  // --- 3. Configure Broadcaster (Transmitter) ---
  pAdvertising = NimBLEDevice::getAdvertising();
  pAdvertising->setConnectableMode(BLE_GAP_CONN_MODE_NON);  // Broadcast-only ("megaphone" mode, no pairing)
  // BLE timing uses 0.625ms units: 160 = 100ms, 240 = 150ms. A range adds jitter to avoid radio collisions.
  pAdvertising->setMinInterval(160);
  pAdvertising->setMaxInterval(240);
  updateBroadcast(0.0f);  // Load initial packet data before turning on the transmitter
  pAdvertising->start();  // Start broadcasting in the background

  // --- 4. Configure Scanner (Receiver) ---
  pScan = NimBLEDevice::getScan();
  // Attach our callback handler; 'true' (wantDuplicates) is required to see updated payloads from known nodes
  pScan->setScanCallbacks(&scanCallbacks, true);
  pScan->setActiveScan(false);   // Passive scan: listen silently without sending extra reply packets
  pScan->setInterval(100);       // Scan cycle repeats every 100ms
  pScan->setWindow(60);          // Listen for 60ms per cycle, leaving 40ms free for outgoing broadcasts
  pScan->setMaxResults(0);       // Callback-only mode: don't store past devices in RAM (prevents memory leaks)
  pScan->start(0, false, true);  // 0 = scan continuously forever

  // Print startup banner so the student knows their board's auto-assigned ID
  Serial.printf("Node 0x%02X online (Broadcasting & Scanning)\n", myPacket.senderId);
}






// ===== 5 - MAIN LOOP =====

void loop() {
  // 'static' keeps this variable's value saved between loops instead of resetting it to 0
  static uint32_t lastTx = 0;

  // Non-blocking 2-second timer: checks elapsed time without freezing motors or sensors like delay() would
  if (millis() - lastTx >= 2000) {
    lastTx = millis();  // Reset timer for the next 2-second interval

    // --- STUDENT CUSTOMIZATION POINT ---
    // Replace this simulated reading with a real sensor (e.g., analogRead, distance, or robot state)
    float simulatedSensor = random(200, 300) / 10.0f;

    // Pack the new reading into our MeshPacket and hot-swap it into the live BLE broadcast
    updateBroadcast(simulatedSensor);

    // Log what we just transmitted so we can compare it against neighboring boards' [RX] logs
    Serial.printf("[TX] Node 0x%02X | Seq: %3u | Value: %.2f\n",
                  myPacket.senderId, myPacket.seq, simulatedSensor);
  }
}