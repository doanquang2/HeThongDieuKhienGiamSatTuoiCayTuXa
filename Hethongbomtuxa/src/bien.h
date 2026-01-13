#ifndef BIEN_H
#define BIEN_H

#include <Arduino.h>
#include <RTClib.h>
#include <WebServer.h>
#include <Preferences.h>
#include <freertos/FreeRTOS.h>
#include <freertos/semphr.h>

// Máy trạng thái cho chế độ AUTO
enum AutoState {
  IDLE,
  START_SEQUENCE,
  OPEN_VALVE,
  WAIT_VALVE_FEEDBACK,
  START_PUMP,
  PUMPING,
  SWITCH_SOURCE_ON,    // Bật nguồn để chuẩn bị chuyển van
  OPEN_NEXT_VALVE,     // Mở van kế tiếp
  WAIT_NEXT_VALVE_FEEDBACK, // Đợi phản hồi van kế tiếp
  CLOSE_PREV_VALVE,    // Đóng van cũ
  SWITCH_SOURCE_OFF,   // Tắt nguồn nuôi van
  STOP_PUMP,           // Chỉ tắt bơm khi hết van 8
  CLOSE_LAST_VALVE,    // Đóng van cuối cùng
  FINISH
};

// Khai báo extern để các file khác có thể sử dụng
extern RTC_DS3231 rtc;
extern WebServer server;
extern Preferences preferences;
extern HardwareSerial RS485;
extern bool rtcFound; // Biến kiểm tra trạng thái RTC

// Mutex cho FreeRTOS
extern SemaphoreHandle_t logMutex;   // Bảo vệ biến webLog/faultLog
extern SemaphoreHandle_t rtcMutex;   // Bảo vệ I2C RTC
extern SemaphoreHandle_t rs485Mutex; // Bảo vệ RS485 Serial

extern bool isAutoMode;
extern int currentValveIndex;
extern unsigned long valveRunTimes[9];
extern int startHour1, startMin1;
extern int startHour2, startMin2;
extern AutoState currentState;
extern unsigned long stateTimer;
extern bool feedbackReceived;
extern int retryCount;
extern String webLog;
extern bool isPressureFault; // Biến trạng thái lỗi áp suất
extern String faultLog;      // Biến lưu lịch sử lỗi
extern bool slave1Connected;
extern bool slave2Connected;
void addToLog(String msg);
void addToFaultLog(String msg); // Hàm ghi lỗi riêng

#endif