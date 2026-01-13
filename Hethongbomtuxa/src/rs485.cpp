#include <Arduino.h>
#include "rs485.h"
#include "bien.h"

// Định nghĩa giao thức (Khớp với Slave)
#define STX           0xAA
#define ETX           0x55
#define CMD_ON        0x01
#define CMD_OFF       0x00

void sendRS485Command(int globalValveID, bool state) {
  uint8_t slaveAddress;
  uint8_t localValveID;

  if (globalValveID >= 1 && globalValveID <= 4) {
    slaveAddress = 1;
    localValveID = globalValveID; // Van 1-4 trên Slave 1
  } else if (globalValveID >= 5 && globalValveID <= 8) {
    slaveAddress = 2;
    localValveID = globalValveID - 4; // Van 5-8 là van 1-4 trên Slave 2
  } else {
    return; // ID van không hợp lệ
  }

  uint8_t cmd = state ? CMD_ON : CMD_OFF;
  
  // AN TOÀN: Gửi thời gian cài đặt + 60s. Nếu Master lỗi/mất lệnh OFF, Slave sẽ tự tắt.
  uint16_t timeVal = state ? (valveRunTimes[globalValveID] + 60) : 0;

  uint8_t frame[8];
  frame[0] = STX;
  frame[1] = slaveAddress;
  frame[2] = cmd;
  frame[3] = localValveID;
  frame[4] = (timeVal >> 8) & 0xFF;
  frame[5] = timeVal & 0xFF;
  
  // Tính Checksum
  uint8_t sum = 0;
  for(int i=1; i<=5; i++) sum += frame[i];
  frame[6] = sum;
  frame[7] = ETX;

  // Dùng Mutex để tránh xung đột nếu nhiều task cùng gửi lệnh
  if (xSemaphoreTake(rs485Mutex, portMAX_DELAY) == pdTRUE) {
    RS485.write(frame, 8);
    RS485.flush();
    // Debug
    Serial.printf("RS485 TX [Slave %d - Van %d] CMD: %02X\n", slaveAddress, localValveID, cmd);
    xSemaphoreGive(rs485Mutex);
  }
}