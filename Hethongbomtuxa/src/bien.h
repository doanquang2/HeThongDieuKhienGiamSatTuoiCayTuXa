#ifndef BIEN_H
#define BIEN_H

#include <Arduino.h>
#include <RTClib.h>
#include <WebServer.h>
#include <Preferences.h>

// Máy trạng thái cho chế độ AUTO
enum AutoState {
  IDLE,
  START_SEQUENCE,
  OPEN_VALVE,
  WAIT_VALVE_FEEDBACK,
  START_PUMP,
  PUMPING,
  SWITCH_SOURCE_ON,    // Bật nguồn để chuẩn bị chuyển van
  SWAP_VALVES,         // Gửi lệnh mở van mới và đóng van cũ
  WAIT_SWAP_COMPLETE,  // Đợi van chạy hết hành trình
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

extern bool isAutoMode;
extern int currentValveIndex;
extern unsigned long valveRunTimes[9];
extern int startHour1, startMin1;
extern int startHour2, startMin2;
extern AutoState currentState;
extern unsigned long stateTimer;
extern bool feedbackReceived;
extern String webLog;
extern bool isPressureFault; // Biến trạng thái lỗi áp suất
extern String faultLog;      // Biến lưu lịch sử lỗi
void addToLog(String msg);
void addToFaultLog(String msg); // Hàm ghi lỗi riêng

#endif