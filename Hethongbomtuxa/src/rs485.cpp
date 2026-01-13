#include <Arduino.h>
#include <ArduinoJson.h>
#include "rs485.h"
#include "bien.h"

void sendRS485Command(int globalValveID, bool state) {
  // Protocol mới: {"a":1, "v":1, "s":1, "g":1}
  // a: địa chỉ slave, v: id van trên slave, s: trạng thái, g: id van toàn cục
  int slaveAddress;
  int localValveID;

  if (globalValveID >= 1 && globalValveID <= 4) {
    slaveAddress = 1;
    localValveID = globalValveID; // Van 1-4 trên Slave 1
  } else if (globalValveID >= 5 && globalValveID <= 8) {
    slaveAddress = 2;
    localValveID = globalValveID - 4; // Van 5-8 là van 1-4 trên Slave 2
  } else {
    return; // ID van không hợp lệ
  }

  StaticJsonDocument<64> doc;
  doc["a"] = slaveAddress;
  doc["v"] = localValveID;
  doc["s"] = state ? 1 : 0;
  doc["g"] = globalValveID; // Gửi ID toàn cục để Slave phản hồi ACK

  serializeJson(doc, RS485);
  RS485.println(); // Kết thúc lệnh
}