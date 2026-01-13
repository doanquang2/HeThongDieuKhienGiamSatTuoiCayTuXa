#include <Arduino.h>
#include "web.h"
#include "bien.h"
#include "cau_hinh.h"
#include "rs485.h"
#include "web_interface.h"

void loadSettings() {
  preferences.begin("irrigation", false);
  startHour1 = preferences.getInt("h1", 7);
  startMin1 = preferences.getInt("m1", 0);
  startHour2 = preferences.getInt("h2", 17);
  startMin2 = preferences.getInt("m2", 0);
  
  for(int i=1; i<=8; i++) {
    char key[5];
    sprintf(key, "t%d", i);
    valveRunTimes[i] = preferences.getULong(key, 60); // Mặc định 60s
  }
  preferences.end();
}

void handleRoot() {
  server.send_P(200, "text/html", index_html);
}

void handleSetTime() {
  if (server.hasArg("datetime")) {
    String dt = server.arg("datetime"); // Format: 2023-10-25T14:30
    // Parse đơn giản
    int y = dt.substring(0, 4).toInt();
    int m = dt.substring(5, 7).toInt();
    int d = dt.substring(8, 10).toInt();
    int h = dt.substring(11, 13).toInt();
    int min = dt.substring(14, 16).toInt();
    
    rtc.adjust(DateTime(y, m, d, h, min, 0));
    Serial.println("Da cap nhat thoi gian RTC: " + dt);
  }
  server.sendHeader("Location", "/");
  server.send(303);
}

void handleToggleMode() {
  isAutoMode = !isAutoMode;
  if (!isAutoMode) {
    currentState = IDLE;
    digitalWrite(PIN_RELAY_PUMP, LOW);
    digitalWrite(PIN_RELAY_SOURCE, LOW);
    addToLog("Chuyển sang chế độ THỦ CÔNG");
  } else {
    addToLog("Chuyển sang chế độ TỰ ĐỘNG");
  }
  server.send(200, "text/plain", "OK");
}

void handleStatus() {
  DateTime now = rtc.now();
  char timeStr[10];
  sprintf(timeStr, "%02d:%02d", now.hour(), now.minute());

  String json = "{";
  json += "\"time\":\"" + String(timeStr) + "\",";
  json += "\"isAuto\":" + String(isAutoMode ? "true" : "false") + ",";
  json += "\"pump\":" + String(digitalRead(PIN_RELAY_PUMP)) + ",";
  json += "\"source\":" + String(digitalRead(PIN_RELAY_SOURCE)) + ",";
  json += "\"fault\":" + String(isPressureFault ? "true" : "false") + ",";
  
  // Xác định van đang mở (chỉ khi đang chạy Auto và không ở trạng thái chờ)
  int activeValve = (isAutoMode && currentState != IDLE && currentState != FINISH) ? currentValveIndex : 0;
  json += "\"activeValve\":" + String(activeValve) + ",";
  
  json += "\"log\":\"" + webLog + "\"";
  json += "}";

  server.send(200, "application/json", json);
}

void handleSave() {
  preferences.begin("irrigation", false);
  if(server.hasArg("h1")) { startHour1 = server.arg("h1").toInt(); preferences.putInt("h1", startHour1); }
  if(server.hasArg("m1")) { startMin1 = server.arg("m1").toInt(); preferences.putInt("m1", startMin1); }
  if(server.hasArg("h2")) { startHour2 = server.arg("h2").toInt(); preferences.putInt("h2", startHour2); }
  if(server.hasArg("m2")) { startMin2 = server.arg("m2").toInt(); preferences.putInt("m2", startMin2); }

  for(int i=1; i<=8; i++) {
    String argName = "t" + String(i);
    if(server.hasArg(argName)) {
      valveRunTimes[i] = server.arg(argName).toInt();
      char key[5]; sprintf(key, "t%d", i); preferences.putULong(key, valveRunTimes[i]);
    }
  }
  preferences.end();
  
  // Redirect về trang chủ sau khi lưu
  server.sendHeader("Location", "/");
  server.send(303);
}

void handleControl() {
  // Chỉ cho phép điều khiển khi không ở chế độ Auto
  if (server.hasArg("dev") && server.hasArg("state")) {
    String dev = server.arg("dev");
    int state = server.arg("state").toInt();
    
    if (dev == "reset_fault") {
      isPressureFault = false;
      addToLog("Đã Reset lỗi từ Web.");
    } else if (dev == "clear_fault_log") {
      faultLog = ""; // Xóa lịch sử lỗi
    } else if (isPressureFault) {
      // Nếu đang lỗi thì không cho điều khiển gì khác
    } else if (!isAutoMode) {
      // Các lệnh điều khiển bình thường
      if (dev == "pump") {
      digitalWrite(PIN_RELAY_PUMP, state);
    } else if (dev == "source") {
      digitalWrite(PIN_RELAY_SOURCE, state);
    } else if (dev == "valve" && server.hasArg("id")) {
      int id = server.arg("id").toInt();
      sendRS485Command(id, state);
    }
    }
  }
  
  if (server.hasArg("ajax")) {
    server.send(200, "text/plain", "OK");
  } else {
    server.sendHeader("Location", "/");
    server.send(303);
  }
}