#include "bien.h"

// Khởi tạo đối tượng
RTC_DS3231 rtc;
WebServer server(80);
Preferences preferences;
HardwareSerial RS485(2);

// Khởi tạo biến
bool isAutoMode = false;
int currentValveIndex = 0;
unsigned long valveRunTimes[9]; // Index 0 bỏ qua
int startHour1 = 7, startMin1 = 0;
int startHour2 = 17, startMin2 = 0;
AutoState currentState = IDLE;
unsigned long stateTimer = 0;
bool feedbackReceived = false;
String webLog = "System Ready...<br>";
bool isPressureFault = false;
String faultLog = "";

void addToLog(String msg) {
  DateTime now = rtc.now();
  char timeStr[10];
  sprintf(timeStr, "%02d:%02d:%02d", now.hour(), now.minute(), now.second());
  webLog = "<span class='text-warning'>[" + String(timeStr) + "]</span> " + msg + "<br>" + webLog;
  if (webLog.length() > 2000) webLog = webLog.substring(0, 2000);
}

void addToFaultLog(String msg) {
  DateTime now = rtc.now();
  char timeStr[20];
  sprintf(timeStr, "%02d/%02d %02d:%02d:%02d", now.day(), now.month(), now.hour(), now.minute(), now.second());
  // Thêm dòng mới vào đầu bảng (dạng HTML table row)
  faultLog = "<tr><td>" + String(timeStr) + "</td><td class='text-danger fw-bold'>" + msg + "</td></tr>" + faultLog;
}