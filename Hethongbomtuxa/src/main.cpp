#include <Arduino.h>
#include <WiFi.h>
#include <WebServer.h>
#include <Wire.h>
#include <RTClib.h>
#include "cau_hinh.h"
#include "bien.h"
#include "rs485.h"
#include "tu_dong.h"
#include "web.h"
#include <BlynkSimpleEsp32.h>

#define PIN_BUTTON_AUTO 15 // Chân nút nhấn giữ: Nhấn (LOW) = Auto, Nhả (HIGH) = Manual
int lastButtonState = HIGH; // Lưu trạng thái nút nhấn

// Hàm kiểm tra áp suất với cơ chế chống nhiễu (Debounce) không chặn (Non-blocking)
void checkPressureSensor() {
  if (isPressureFault) return; // Đã lỗi rồi thì không check nữa

  static unsigned long debounceTimer = 0;
  static bool debouncing = false;
  const unsigned long DEBOUNCE_DELAY = 1000; // Thời gian xác nhận lỗi (1 giây liên tục)

  if (digitalRead(PIN_PRESSURE_SWITCH) == LOW) {
    if (!debouncing) {
      debouncing = true;
      debounceTimer = millis();
    } else if (millis() - debounceTimer >= DEBOUNCE_DELAY) {
      // Xác nhận lỗi thật sự sau thời gian debounce
      isPressureFault = true;
      isAutoMode = false; 
      currentState = IDLE; 
      
      digitalWrite(PIN_RELAY_PUMP, LOW); 
      digitalWrite(PIN_RELAY_SOURCE, LOW); 
      
      Serial.println("ALARM: SU CO AP SUAT! Da ngat toan bo.");
      addToLog("<span class='text-danger fw-bold'>SỰ CỐ: Áp suất cao! Hệ thống đã dừng khẩn cấp.</span>");
      addToFaultLog("Áp suất quá cao - Dừng hệ thống"); 
      
      if (Blynk.connected()) {
        Blynk.logEvent("pressure_alert", "CẢNH BÁO: Áp suất quá cao! Hệ thống đã dừng khẩn cấp. Vui lòng kiểm tra và nhấn Reset.");
      }
      debouncing = false;
    }
  } else {
    debouncing = false; // Nếu tín hiệu trở lại HIGH, hủy đếm giờ
  }
}

// Hàm API trả về cài đặt hiện tại cho Web (JSON)
void handleGetSettings() {
  String json = "{";
  json += "\"h1\":" + String(startHour1) + ",";
  json += "\"m1\":" + String(startMin1) + ",";
  json += "\"h2\":" + String(startHour2) + ",";
  json += "\"m2\":" + String(startMin2) + ",";
  for(int i=1; i<=8; i++) {
    json += "\"t" + String(i) + "\":" + String(valveRunTimes[i]);
    if(i < 8) json += ",";
  }
  json += "}";
  server.send(200, "application/json", json);
}

void setup() {
  Serial.begin(115200);
  
  // Khởi tạo các Mutex (Bắt buộc vì các file khác đang sử dụng chúng)
  logMutex = xSemaphoreCreateMutex();
  rtcMutex = xSemaphoreCreateMutex();
  rs485Mutex = xSemaphoreCreateMutex();

  RS485.begin(9600, SERIAL_8N1, PIN_RS485_RX, PIN_RS485_TX);

  pinMode(PIN_RELAY_PUMP, OUTPUT);
  pinMode(PIN_RELAY_SOURCE, OUTPUT);
  pinMode(PIN_PRESSURE_SWITCH, INPUT_PULLUP); // Kích hoạt trở kéo lên
  pinMode(PIN_BUTTON_AUTO, INPUT_PULLUP);     // Nút nhấn Auto (Lưu ý: Cần đổi chân Bơm nếu đang dùng chân 13)
  digitalWrite(PIN_RELAY_PUMP, LOW); // Kích mức cao hay thấp tùy phần cứng, giả sử LOW là tắt
  digitalWrite(PIN_RELAY_SOURCE, LOW);

  // Khởi động RTC
  rtcFound = rtc.begin();
  if (!rtcFound) {
    Serial.println("Couldn't find RTC");
  }
  if (rtcFound && rtc.lostPower()) {
    rtc.adjust(DateTime(F(__DATE__), F(__TIME__)));
  }

  // Load cài đặt từ bộ nhớ
  loadSettings();

  // Đọc trạng thái nút nhấn ban đầu để cài đặt chế độ
  lastButtonState = digitalRead(PIN_BUTTON_AUTO);
  if (lastButtonState == LOW) {
    isAutoMode = true;
  } else {
    isAutoMode = false;
  }

  // --- KẾT NỐI WIFI & CẤU HÌNH IP TĨNH THÔNG MINH ---
  Serial.print("Dang ket noi WiFi...");
  WiFi.mode(WIFI_STA);
  WiFi.begin(WIFI_SSID, WIFI_PASS);
  
  // Đợi kết nối để lấy thông tin mạng (Gateway, Subnet)
  int retry = 0;
  while (WiFi.status() != WL_CONNECTED && retry < 20) {
    delay(500);
    Serial.print(".");
    retry++;
  }

  if (WiFi.status() == WL_CONNECTED) {
    Serial.println("\nDa ket noi DHCP!");
    Serial.print("Gateway hien tai: "); Serial.println(WiFi.gatewayIP());
    
    // Cấu hình IP Tĩnh dựa trên Gateway vừa dò được
    IPAddress ip = WiFi.localIP();
    IPAddress gateway = WiFi.gatewayIP();
    IPAddress subnet = WiFi.subnetMask();
    
    ip[3] = STATIC_IP_SUFFIX; // Đổi đuôi IP thành số cố định (ví dụ .200)
    IPAddress dns(8, 8, 8, 8); // Thêm DNS Google để sửa lỗi blynk.cloud
    
    if(WiFi.config(ip, gateway, subnet, dns)) {
      Serial.print("Da chuyen sang IP Tinh: "); Serial.println(WiFi.localIP());
    }
  }

  // Kết nối Blynk (Dùng config vì WiFi đã kết nối ở trên)
  Blynk.config(BLYNK_AUTH_TOKEN);
  Blynk.connect(); // Kết nối tới server Blynk
  
  // Webserver
  server.on("/", handleRoot);
  server.on("/save", handleSave);
  server.on("/control", handleControl);
  server.on("/set_time", handleSetTime);
  server.on("/status", handleStatus);
  server.on("/get_settings", handleGetSettings); // Đăng ký API lấy cài đặt
  server.on("/toggle_mode", handleToggleMode);
  server.begin();
}

void loop(){
  // Chỉ chạy Blynk khi có mạng
  if (WiFi.status() == WL_CONNECTED) {
    Blynk.run();
  }
  server.handleClient();
  
  // --- XỬ LÝ NÚT NHẤN AUTO (LATCHING) ---
  int currentButtonState = digitalRead(PIN_BUTTON_AUTO);
  if (currentButtonState != lastButtonState) {
    delay(50); // Chống nhiễu (Debounce)
    if (digitalRead(PIN_BUTTON_AUTO) == currentButtonState) {
      lastButtonState = currentButtonState;
      
      if (currentButtonState == LOW) {
        // Nút nhấn ĐÓNG (GND) -> Chuyển sang AUTO
        if (!isAutoMode) {
          isAutoMode = true;
          addToLog("Chuyển sang TỰ ĐỘNG (Nút cứng)");
          if (Blynk.connected()) Blynk.virtualWrite(V0, 1); // Đồng bộ Blynk ON
        }
      } else {
        // Nút nhấn MỞ (Hở) -> Chuyển sang MANUAL
        if (isAutoMode) {
          isAutoMode = false;
          currentState = IDLE; // Reset trạng thái tự động
          digitalWrite(PIN_RELAY_PUMP, LOW);
          digitalWrite(PIN_RELAY_SOURCE, LOW);
          addToLog("Chuyển sang THỦ CÔNG (Nút cứng)");
          if (Blynk.connected()) Blynk.virtualWrite(V0, 0); // Đồng bộ Blynk OFF
        }
      }
    }
  }

  // Kiểm tra phản hồi từ Slave qua RS485
  while (RS485.available() >= 8) { // Đợi đủ khung 8 byte
    if (RS485.peek() != 0xAA) { // 0xAA là STX
      RS485.read(); // Bỏ qua byte rác
      continue;
    }

    uint8_t rxBuf[8];
    RS485.readBytes(rxBuf, 8);

    // Kiểm tra cấu trúc khung: STX, ETX và Checksum
    if (rxBuf[0] == 0xAA && rxBuf[7] == 0x55) {
      uint8_t sum = 0;
      for(int i=1; i<=5; i++) sum += rxBuf[i];
      
      if (sum == rxBuf[6] && rxBuf[2] == 0xFF) { // 0xFF là CMD_ACK
        int sID = rxBuf[1];
        // Lưu ý: Slave code hiện tại không gửi về ID Van trong gói ACK, chỉ gửi Slave ID.
        // Ta xác nhận dựa trên Slave ID khớp với Slave đang điều khiển.
        
        int expectedSlave = (currentValveIndex <= 4) ? 1 : 2;
        
        // Nếu nhận được ACK từ đúng Slave đang chờ
        if (sID == expectedSlave) {
          feedbackReceived = true;
          if (sID == 1) slave1Connected = true;
          if (sID == 2) slave2Connected = true;
          Serial.printf("Nhan ACK tu Slave %d\n", sID);
        }
      }
    }
  }

  if (isAutoMode) {
    checkAutoSchedule();
    runAutoLogic();
  }

  // --- GIÁM SÁT ÁP SUẤT (NON-BLOCKING) ---
  checkPressureSensor();
}

// --- BLYNK HANDLERS ---
BLYNK_WRITE(V0) { // Nút chuyển chế độ Auto/Manu
  isAutoMode = param.asInt();
  if (!isAutoMode) {
    // Reset trạng thái nếu chuyển về Manu
    currentState = IDLE;
    digitalWrite(PIN_RELAY_PUMP, LOW);
    digitalWrite(PIN_RELAY_SOURCE, LOW);
  }
}

BLYNK_WRITE(V1) { // Nút bơm Manual
  if (!isAutoMode) digitalWrite(PIN_RELAY_PUMP, param.asInt());
}

BLYNK_WRITE(V2) { // Nút nguồn Manual
  if (!isAutoMode) digitalWrite(PIN_RELAY_SOURCE, param.asInt());
}

// Các nút V11 -> V18 để bật tắt van thủ công
BLYNK_WRITE(V11) { if(!isAutoMode) sendRS485Command(1, param.asInt()); }
BLYNK_WRITE(V12) { if(!isAutoMode) sendRS485Command(2, param.asInt()); }
BLYNK_WRITE(V13) { if(!isAutoMode) sendRS485Command(3, param.asInt()); }
BLYNK_WRITE(V14) { if(!isAutoMode) sendRS485Command(4, param.asInt()); }
BLYNK_WRITE(V15) { if(!isAutoMode) sendRS485Command(5, param.asInt()); }
BLYNK_WRITE(V16) { if(!isAutoMode) sendRS485Command(6, param.asInt()); }
BLYNK_WRITE(V17) { if(!isAutoMode) sendRS485Command(7, param.asInt()); }
BLYNK_WRITE(V18) { if(!isAutoMode) sendRS485Command(8, param.asInt()); }

// Nút Reset Lỗi trên Blynk (V3)
BLYNK_WRITE(V3) {
  if (param.asInt() == 1) {
    if (isPressureFault) {
      isPressureFault = false;
      addToLog("Đã Reset hệ thống từ Blynk.");
    }
  }
}

// Đồng bộ thời gian từ Blynk (Sử dụng Widget Time Input trên V4)
BLYNK_WRITE(V4) {
  long timestamp = param.asLong();
  // Kiểm tra timestamp hợp lệ và RTC đã sẵn sàng
  if (timestamp > 1000000000 && rtcFound) { 
    rtc.adjust(DateTime(timestamp)); // Cập nhật thời gian cho RTC
    
    DateTime now = rtc.now();
    char timeStr[50];
    sprintf(timeStr, "Đồng bộ thời gian từ Blynk: %02d/%02d %02d:%02d", now.day(), now.month(), now.hour(), now.minute());
    Serial.println(timeStr);
    addToLog(timeStr);
  }
}

// Cài đặt Lịch 1 từ Blynk (Time Input - V5)
BLYNK_WRITE(V5) {
  long t = param[0].asLong(); // Thời gian tính bằng giây từ đầu ngày
  int h = t / 3600;
  int m = (t % 3600) / 60;
  
  if (h >= 0 && h < 24 && m >= 0 && m < 60) {
    startHour1 = h;
    startMin1 = m;
    preferences.begin("irrigation", false);
    preferences.putInt("h1", startHour1);
    preferences.putInt("m1", startMin1);
    preferences.end();
    addToLog("Cập nhật Lịch 1 từ Blynk: " + String(h) + ":" + String(m));
  }
}

// Cài đặt Lịch 2 từ Blynk (Time Input - V6)
BLYNK_WRITE(V6) {
  long t = param[0].asLong();
  int h = t / 3600;
  int m = (t % 3600) / 60;
  
  if (h >= 0 && h < 24 && m >= 0 && m < 60) {
    startHour2 = h;
    startMin2 = m;
    preferences.begin("irrigation", false);
    preferences.putInt("h2", startHour2);
    preferences.putInt("m2", startMin2);
    preferences.end();
    addToLog("Cập nhật Lịch 2 từ Blynk: " + String(h) + ":" + String(m));
  }
}