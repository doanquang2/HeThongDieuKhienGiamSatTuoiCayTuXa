#include <Arduino.h>
#include "tu_dong.h"
#include "bien.h"
#include "cau_hinh.h"
#include "rs485.h"

void checkAutoSchedule() {
  if (currentState != IDLE) return; // Đang chạy thì không check giờ

  DateTime now = rtc.now();
  // Kiểm tra đúng giờ và giây = 0 để kích hoạt 1 lần
  bool time1 = (now.hour() == startHour1 && now.minute() == startMin1 && now.second() == 0);
  bool time2 = (now.hour() == startHour2 && now.minute() == startMin2 && now.second() == 0);

  if (time1 || time2) {
    Serial.println("Den gio bom tu dong!");
    addToLog("Đến giờ bơm tự động! Bắt đầu quy trình.");
    currentState = START_SEQUENCE;
    currentValveIndex = 1; // Bắt đầu từ van 1
  }
}

void runAutoLogic() {
  switch (currentState) {
    case IDLE:
      break;

    case START_SEQUENCE:
      // Bật nguồn cấp cho Van (Pin 12)
      digitalWrite(PIN_RELAY_SOURCE, HIGH);
      Serial.println("Bat nguon (Pin 12)");
      addToLog("Bật nguồn Van (Pin 12)");
      stateTimer = millis();
      currentState = OPEN_VALVE;
      break;

    case OPEN_VALVE:
      // Đợi nguồn ổn định 1 chút rồi gửi lệnh
      if (millis() - stateTimer > 1000) {
        Serial.printf("Gui lenh mo Van %d\n", currentValveIndex);
        addToLog("Gửi lệnh mở Van " + String(currentValveIndex));
        sendRS485Command(currentValveIndex, true); // Mở van
        feedbackReceived = false;
        stateTimer = millis();
        currentState = WAIT_VALVE_FEEDBACK;
      }
      break;

    case WAIT_VALVE_FEEDBACK:
      // Đợi phản hồi trong tối đa 5s, nếu quá coi như lỗi hoặc bỏ qua
      if (feedbackReceived) {
        Serial.println("Da nhan phan hoi. Chuan bi bom.");
        addToLog("Đã nhận phản hồi từ Van " + String(currentValveIndex));
        // Quy trình: Tắt nguồn (Pin 12) -> Bật bơm (Pin 13)
        digitalWrite(PIN_RELAY_SOURCE, LOW);
        delay(500); // Delay an toàn
        currentState = START_PUMP;
      } else if (millis() - stateTimer > 5000) {
        Serial.println("Timeout doi phan hoi! Van cu chay tiep.");
        addToLog("Timeout phản hồi! Vẫn tiếp tục bơm.");
        digitalWrite(PIN_RELAY_SOURCE, LOW);
        currentState = START_PUMP;
      }
      break;

    case START_PUMP:
      digitalWrite(PIN_RELAY_PUMP, HIGH);
      Serial.println("Bat Bom (Pin 13)");
      addToLog("Bật Bơm (Pin 13) - Đang tưới Van " + String(currentValveIndex));
      stateTimer = millis();
      currentState = PUMPING;
      break;

    case PUMPING:
      // Chạy hết thời gian cài đặt cho van hiện tại
      if (millis() - stateTimer >= (valveRunTimes[currentValveIndex] * 1000)) {
        if (currentValveIndex < 8) {
          // Nếu chưa phải van cuối, chuyển sang quy trình đổi van (Bơm vẫn ON)
          currentState = SWITCH_SOURCE_ON;
        } else {
          // Nếu là van cuối (Van 8), tắt bơm
          currentState = STOP_PUMP;
        }
      }
      break;

    case SWITCH_SOURCE_ON:
      // Bật nguồn cấp cho Van để chuẩn bị mở van kế tiếp
      digitalWrite(PIN_RELAY_SOURCE, HIGH);
      Serial.println("Bat nguon de chuyen van (Bom van chay)");
      stateTimer = millis();
      currentState = SWAP_VALVES;
      break;

    case SWAP_VALVES:
      if (millis() - stateTimer > 1000) { // Đợi nguồn ổn định
        Serial.printf("Chuyen doi: Mo Van %d -> Dong Van %d\n", currentValveIndex + 1, currentValveIndex);
        addToLog("Chuyển đổi: Mở Van " + String(currentValveIndex + 1) + " -> Đóng Van " + String(currentValveIndex));
        
        // 1. Mở van kế tiếp trước để giảm áp lực
        sendRS485Command(currentValveIndex + 1, true); // Mở van kế tiếp
        delay(300); // Delay nhỏ để lệnh RS485 đi hết và Slave kịp xử lý
        
        // 2. Đóng van hiện tại ngay sau đó
        sendRS485Command(currentValveIndex, false); // Đóng van hiện tại
        
        stateTimer = millis();
        currentState = WAIT_SWAP_COMPLETE;
      }
      break;

    case WAIT_SWAP_COMPLETE:
      // Đợi khoảng 5-10s cho van hành trình chạy xong (tùy loại van)
      if (millis() - stateTimer > 5000) {
        currentState = SWITCH_SOURCE_OFF;
        stateTimer = millis();
      }
      break;

    case SWITCH_SOURCE_OFF:
      if (millis() - stateTimer > 1000) { // Đợi lệnh đóng gửi xong
        digitalWrite(PIN_RELAY_SOURCE, LOW); // Tắt nguồn nuôi van
        currentValveIndex++; // Chuyển chỉ số sang van mới
        stateTimer = millis(); // Reset timer cho quá trình PUMPING của van mới
        currentState = PUMPING;
      }
      break;

    case STOP_PUMP:
      digitalWrite(PIN_RELAY_PUMP, LOW);
      Serial.println("Tat Bom");
      addToLog("Tắt Bơm. Chuẩn bị đóng van cuối.");
      // Bật lại nguồn để đóng van cuối cùng
      digitalWrite(PIN_RELAY_SOURCE, HIGH);
      stateTimer = millis();
      currentState = CLOSE_LAST_VALVE;
      break;

    case CLOSE_LAST_VALVE:
      if (millis() - stateTimer > 2000) { // Đợi nguồn lên
        // Đóng van cuối cùng (Van 8)
        sendRS485Command(currentValveIndex, false);
        delay(1000);
        currentState = FINISH;
      }
      break;

    case FINISH:
      Serial.println("Hoan thanh chu trinh.");
      addToLog("Hoàn thành chu trình tưới.");
      digitalWrite(PIN_RELAY_SOURCE, LOW); // Tắt nguồn hoàn toàn
      currentState = IDLE;
      break;
  }
}