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

void setup() {
  Serial.begin(115200);
  RS485.begin(9600, SERIAL_8N1, PIN_RS485_RX, PIN_RS485_TX);

  pinMode(PIN_RELAY_PUMP, OUTPUT);
  pinMode(PIN_RELAY_SOURCE, OUTPUT);
  pinMode(PIN_PRESSURE_SWITCH, INPUT_PULLUP); // Kích hoạt trở kéo lên
  digitalWrite(PIN_RELAY_PUMP, LOW); // Kích mức cao hay thấp tùy phần cứng, giả sử LOW là tắt
  digitalWrite(PIN_RELAY_SOURCE, LOW);

  // Khởi động RTC
  if (!rtc.begin()) {
    Serial.println("Couldn't find RTC");
  }
  if (rtc.lostPower()) {
    rtc.adjust(DateTime(F(__DATE__), F(__TIME__)));
  }

  // Load cài đặt từ bộ nhớ
  loadSettings();

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
  server.on("/toggle_mode", handleToggleMode);
  server.begin();
}

void loop(){
  // Chỉ chạy Blynk khi có mạng
  if (WiFi.status() == WL_CONNECTED) {
    Blynk.run();
  }
  server.handleClient();
  
  // Kiểm tra phản hồi từ Slave qua RS485
  if (RS485.available()) {
    String response = RS485.readStringUntil('\n');
    // Giả sử Slave gửi về: "ACK:V1"
    if (response.startsWith("ACK:V")) {
      int vID = response.substring(5).toInt();
      // Chấp nhận phản hồi của van hiện tại hoặc van kế tiếp (khi đang chuyển)
      if (vID == currentValveIndex || vID == currentValveIndex + 1) {
        feedbackReceived = true;
        Serial.printf("Nhan phan hoi tu Van %d\n", vID);
      }
    }
  }

  if (isAutoMode) {
    checkAutoSchedule();
    runAutoLogic();
  }

  // --- GIÁM SÁT ÁP SUẤT ---
  // Logic: Công tắc áp suất thường mở (NO). Khi áp suất cao -> Đóng lại -> Nối GND (LOW).
  if (digitalRead(PIN_PRESSURE_SWITCH) == LOW && !isPressureFault) {
    delay(200); // Chống nhiễu (Debounce)
    if (digitalRead(PIN_PRESSURE_SWITCH) == LOW) {
      isPressureFault = true;
      isAutoMode = false; // Thoát chế độ tự động
      currentState = IDLE; // Reset trạng thái
      
      digitalWrite(PIN_RELAY_PUMP, LOW); // Ngắt bơm KHẨN CẤP
      digitalWrite(PIN_RELAY_SOURCE, LOW); // Ngắt nguồn van
      
      Serial.println("ALARM: SU CO AP SUAT! Da ngat toan bo.");
      addToLog("<span class='text-danger fw-bold'>SỰ CỐ: Áp suất cao! Hệ thống đã dừng khẩn cấp.</span>");
      addToFaultLog("Áp suất quá cao - Dừng hệ thống"); // Ghi vào lịch sử lỗi
      
      // Gửi sự kiện lên Blynk (Cần tạo Event 'pressure_alert' trong Blynk Console để gửi Email)
      Blynk.logEvent("pressure_alert", "CẢNH BÁO: Áp suất quá cao! Hệ thống đã dừng khẩn cấp. Vui lòng kiểm tra và nhấn Reset.");
    }
  }
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