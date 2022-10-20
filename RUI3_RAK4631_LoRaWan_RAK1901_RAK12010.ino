#include "rak1901.h"
#include "Light_VEML7700.h" // http://librarymanager/All#Light_veml7700 RAK12010
// http://librarymanager/All#CayenneLPP
#include "Secret.h"

rak1901 rak1901;
Light_VEML7700 VMEL = Light_VEML7700();

#define OTAA_PERIOD (900000)
/*************************************
   LoRaWAN band setting:
     RAK_REGION_EU433
     RAK_REGION_CN470
     RAK_REGION_RU864
     RAK_REGION_IN865
     RAK_REGION_EU868
     RAK_REGION_US915
     RAK_REGION_AU915
     RAK_REGION_KR920
     RAK_REGION_AS923
 *************************************/
#define OTAA_BAND (RAK_REGION_US915)

/** Packet buffer for sending */
uint8_t collected_data[64] = { 0 };

void recvCallback(SERVICE_LORA_RECEIVE_T * data) {
  if (data->BufferSize > 0) {
    Serial.println("Something received!");
    for (int i = 0; i < data->BufferSize; i++) {
      Serial.printf("%x", data->Buffer[i]);
    }
    Serial.print("\r\n");
  }
}

void joinCallback(int32_t status) {
  Serial.printf("Join status: %d\nSending a first packet!\n", status);
  uplink_routine();
}

void sendCallback(int32_t status) {
  if (status == 0) {
    Serial.println("Successfully sent");
  } else {
    Serial.println("Sending failed");
  }
}

void setup() {
  pinMode(WB_IO2, OUTPUT);
  digitalWrite(WB_IO2, HIGH);
  delay(2000);
  Serial.begin(115200, RAK_AT_MODE);
  double t0 = millis();
  while (millis() - t0 < 8000) {
    Serial.write('.');
    delay(500);
  }
  Serial.write('\n');
  Serial.println("RAKwireless LoRaWan OTAA Example");
  delay(500);
  Serial.println("------------------------------------------------------");
  // begin I2C
  Wire.begin();
  // check whether RAK1901 sensor is working
  bool result = rak1901.init();
  Serial.printf("RAK1901 init %s\r\n", result ? "Success" : "Fail");
  if (!result) return;
  rak1901.update(); // The first update is often 0 / 0
  if (!VMEL.begin()) {
    Serial.println("Sensor not found");
    while (1);
  }
  VMEL.setGain(VEML7700_GAIN_1);
  VMEL.setIntegrationTime(VEML7700_IT_800MS);
  Serial.print(F("Gain: "));
  switch (VMEL.getGain()) {
    case VEML7700_GAIN_1: Serial.println("1"); break;
    case VEML7700_GAIN_2: Serial.println("2"); break;
    case VEML7700_GAIN_1_4: Serial.println("1/4"); break;
    case VEML7700_GAIN_1_8: Serial.println("1/8"); break;
  }
  Serial.print(F("Integration Time (ms): "));
  switch (VMEL.getIntegrationTime()) {
    case VEML7700_IT_25MS: Serial.println("25"); break;
    case VEML7700_IT_50MS: Serial.println("50"); break;
    case VEML7700_IT_100MS: Serial.println("100"); break;
    case VEML7700_IT_200MS: Serial.println("200"); break;
    case VEML7700_IT_400MS: Serial.println("400"); break;
    case VEML7700_IT_800MS: Serial.println("800"); break;
  }
  //veml.powerSaveEnable(true);
  //veml.setPowerSaveMode(VEML7700_POWERSAVE_MODE4);
  VMEL.setLowThreshold(10000);
  VMEL.setHighThreshold(20000);
  VMEL.interruptEnable(true);
  // OTAA Device EUI MSB first
  uint8_t node_device_eui[8] = OTAA_DEVEUI;
  // OTAA Application EUI MSB first
  uint8_t node_app_eui[8] = OTAA_APPEUI;
  // OTAA Application Key MSB first
  uint8_t node_app_key[16] = OTAA_APPKEY;
  result = api.lorawan.nwm.set(RAK_LORAWAN);
  Serial.printf("LoRaWan OTAA - set network mode RAK_LORAWAN is %s.\n", result ? "correct" : "incorrect");
  if (!result) return;
  result = api.lorawan.appeui.set(node_app_eui, 8);
  Serial.printf("LoRaWan OTAA - set application EUI is %s.\n", result ? "correct" : "incorrect");
  if (!result) return;
  result = api.lorawan.appkey.set(node_app_key, 16);
  Serial.printf("LoRaWan OTAA - set application key is %s.\n", result ? "correct" : "incorrect");
  if (!result) return;
  result = api.lorawan.deui.set(node_device_eui, 8);
  Serial.printf("LoRaWan OTAA - set device EUI is %s.\n", result ? "correct" : "incorrect");
  if (!result) return;
  result = api.lorawan.band.set(OTAA_BAND);
  Serial.printf("LoRaWan OTAA - set band is %s.\n", result ? "correct" : "incorrect");
  if (!result) return;
  result = api.lorawan.deviceClass.set(RAK_LORA_CLASS_A);
  Serial.printf("LoRaWan OTAA - set device class is %s.\n", result ? "correct" : "incorrect");
  if (!result) return;
  result = api.lorawan.njm.set(RAK_LORA_OTAA);
  // Set the network join mode to OTAA
  Serial.printf("LoRaWan OTAA - set network join mode is %s.\n", result ? "correct" : "incorrect");
  if (!result) return;
  /** Wait for Join success */
  while (api.lorawan.njs.get() == 0) {
    Serial.print("Wait for LoRaWAN join...");
    result = api.lorawan.join();
    // Join Gateway
    Serial.printf("LoRaWan OTAA - join %s.\n", result ? "successful" : "unsuccessful");
    delay(10000);
  }
  result = api.lorawan.adr.set(true);
  Serial.printf("LoRaWan OTAA - set adaptive data rate is %s.\n", result ? "correct" : "incorrect");
  if (!result) return;
  result = api.lorawan.rety.set(3);
  Serial.printf("LoRaWan OTAA - set retry times is %s.\n", result ? "correct" : "incorrect");
  if (!result) return;
  result = api.lorawan.cfm.set(1);
  Serial.printf("LoRaWan OTAA - set confirm mode is %s.\n", result ? "correct" : "incorrect");
  if (!result) return;
  /** Check LoRaWan Status*/
  Serial.printf("Duty cycle is %s\r\n", api.lorawan.dcs.get() ? "ON" : "OFF"); // Check Duty Cycle status
  Serial.printf("Packet is %s\r\n", api.lorawan.cfm.get() ? "CONFIRMED" : "UNCONFIRMED"); // Check Confirm status
  uint8_t assigned_dev_addr[4] = { 0 };
  api.lorawan.daddr.get(assigned_dev_addr, 4);
  Serial.printf("Device Address is %02X%02X%02X%02X\r\n", assigned_dev_addr[0], assigned_dev_addr[1], assigned_dev_addr[2], assigned_dev_addr[3]);  // Check Device Address
  Serial.printf("Uplink period is %ums\r\n", OTAA_PERIOD);
  Serial.println("");
  api.lorawan.registerRecvCallback(recvCallback);
  api.lorawan.registerJoinCallback(joinCallback);
  api.lorawan.registerSendCallback(sendCallback);
  Serial.println("All done!");
}

void uplink_routine() {
  /** Payload of Uplink */
  uint8_t data_len = 0;
  float t = rak1901.temperature();
  float h = rak1901.humidity();
  data_len = 0;
  uint8_t b = (uint8_t)t;
  collected_data[data_len++] = b;
  b = (t - b) * 100;
  collected_data[data_len++] = b;
  b = (uint8_t)h;
  collected_data[data_len++] = b;
  b = (h - b) * 100;
  collected_data[data_len++] = b;

  float lx = VMEL.readLux();
  b = (uint8_t)lx;
  collected_data[data_len++] = b;
  b = (lx - b) * 100;
  collected_data[data_len++] = b;

  Serial.printf(
    "Data Packet: %02x %02x %02x %02x %02x %02x\n",
    collected_data[0], collected_data[1], collected_data[2],
    collected_data[3], collected_data[4], collected_data[5]
  );
  if (rak1901.update()) {
    /** Send the data package */
    if (api.lorawan.send(data_len, collected_data, 2, true, 1)) {
      Serial.println("Requested uplink.");
    } else {
      Serial.println("Sending failed.");
    }
  } else {
    Serial.println("Please plug in the RAK1901 sensor and reboot");
    while (1);
  }
}

void loop() {
  static uint64_t last = 0;
  static uint64_t elapsed;
  if ((elapsed = millis() - last) > OTAA_PERIOD) {
    uplink_routine();
    last = millis();
  }
  // Serial.printf("Try sleep %ums..", OTAA_PERIOD);
  api.system.sleep.all(OTAA_PERIOD);
  // Serial.println("Wakeup..");
}
