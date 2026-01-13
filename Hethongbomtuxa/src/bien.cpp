#include "bien.h"

// Khởi tạo đối tượng
RTC_DS3231 rtc;
WebServer server(80);
Preferences preferences;
HardwareSerial RS485(2);
bool rtcFound = false;

// Định nghĩa Mutex
SemaphoreHandle_t logMutex;
SemaphoreHandle_t rtcMutex;
SemaphoreHandle_t rs485Mutex;

// Khởi tạo biến
bool isAutoMode = false;
int currentValveIndex = 0;
unsigned long valveRunTimes[9]; // Index 0 bỏ qua
int startHour1 = 7, startMin1 = 0;
int startHour2 = 17, startMin2 = 0;
AutoState currentState = IDLE;
unsigned long stateTimer = 0;
bool feedbackReceived = false;
int retryCount = 0;
String webLog = "System Ready...<br>";
bool isPressureFault = false;
String faultLog = "";
bool slave1Connected = true;
bool slave2Connected = true;

void addToLog(String msg) {
  String timeStr;
  
  // Bảo vệ truy cập RTC
  if (xSemaphoreTake(rtcMutex, pdMS_TO_TICKS(100)) == pdTRUE) {
    if (rtcFound) {
      DateTime now = rtc.now();
      char buf[10];
      sprintf(buf, "%02d:%02d:%02d", now.hour(), now.minute(), now.second());
      timeStr = String(buf);
    } else {
      timeStr = String(millis() / 1000);
    }
    xSemaphoreGive(rtcMutex);
  }
  
  // Bảo vệ truy cập biến webLog
  if (xSemaphoreTake(logMutex, pdMS_TO_TICKS(100)) == pdTRUE) {
    webLog = "<span class='text-warning'>[" + timeStr + "]</span> " + msg + "<br>" + webLog;
    if (webLog.length() > 2000) webLog = webLog.substring(0, 2000);
    xSemaphoreGive(logMutex);
  }
}

void addToFaultLog(String msg) {
  String timeStr = "Unknown Time";
  if (xSemaphoreTake(rtcMutex, pdMS_TO_TICKS(100)) == pdTRUE) {
    if (rtcFound) {
      DateTime now = rtc.now();
      char buf[20];
      sprintf(buf, "%02d/%02d %02d:%02d:%02d", now.day(), now.month(), now.hour(), now.minute(), now.second());
      timeStr = String(buf);
    }
    xSemaphoreGive(rtcMutex);
  }
  // Thêm dòng mới vào đầu bảng (dạng HTML table row)
  if (xSemaphoreTake(logMutex, pdMS_TO_TICKS(100)) == pdTRUE) {
    faultLog = "<tr><td>" + timeStr + "</td><td class='text-danger fw-bold'>" + msg + "</td></tr>" + faultLog;
    xSemaphoreGive(logMutex);
  }
}