#include <Arduino.h>
#include "tu_dong.h"
#include "bien.h"
#include "cau_hinh.h"
#include "rs485.h"

void checkAutoSchedule() {
  if (currentState != IDLE) return; // Đang chạy thì không check giờ
  if (!rtcFound) return; // Không có RTC thì không thể check giờ

  DateTime now;
  if (xSemaphoreTake(rtcMutex, pdMS_TO_TICKS(100)) == pdTRUE) {
    now = rtc.now();
    xSemaphoreGive(rtcMutex);
  } else return;

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
      // Đợi 3s cho nguồn ổn định rồi mới gửi lệnh
      if (millis() - stateTimer > 3000) {
        Serial.printf("Gui lenh mo Van %d\n", currentValveIndex);
        addToLog("Gửi lệnh mở Van " + String(currentValveIndex));
        sendRS485Command(currentValveIndex, true); // Mở van
        feedbackReceived = false;
        retryCount = 0;
        stateTimer = millis(); // Reset timer cho trạng thái chờ phản hồi
        currentState = WAIT_VALVE_FEEDBACK;
      }
      break;

    case WAIT_VALVE_FEEDBACK:
      // Đợi phản hồi VÀ Đợi đủ 20s cho van chạy hết hành trình (quan trọng)
      if (feedbackReceived && (millis() - stateTimer > 20000)) {
        Serial.println("Da mo van xong (20s). Tat nguon Van, Bat Bom.");
        addToLog("Van " + String(currentValveIndex) + " đã mở hoàn toàn. Bật Bơm.");
        
        // Quy trình: Tắt nguồn (Pin 12) và Bật bơm (Pin 13) đồng thời
        digitalWrite(PIN_RELAY_SOURCE, LOW);
        digitalWrite(PIN_RELAY_PUMP, HIGH);
        
        stateTimer = millis();
        currentState = PUMPING; // Chuyển thẳng sang bơm (Bỏ qua bước START_PUMP cũ)
      } else if (millis() - stateTimer > 20000) { // Tăng timeout lên 40s để chờ Slave
        retryCount++;
        if (retryCount < 3) {
          addToLog("<span class='text-warning'>Timeout Van " + String(currentValveIndex) + ". Thử lại lần " + String(retryCount) + "...</span>");
          sendRS485Command(currentValveIndex, true); // Gửi lại lệnh
          stateTimer = millis(); // Reset timer
        } else {
          int slave = (currentValveIndex <= 4) ? 1 : 2;
          if (slave == 2) {
            slave2Connected = false;
            addToLog("<span class='text-danger'>LỖI: Slave 2 không phản hồi! Dừng hệ thống.</span>");
            addToFaultLog("Mất kết nối Slave 2");
            digitalWrite(PIN_RELAY_SOURCE, LOW);
            currentState = FINISH;
          } else {
            slave1Connected = false;
            addToLog("<span class='text-warning'>Cảnh báo: Slave 1 không phản hồi Van " + String(currentValveIndex) + ". Bỏ qua.</span>");
            addToFaultLog("Lỗi Slave 1 - Van " + String(currentValveIndex));
            // AN TOÀN: Gửi lệnh đóng van hiện tại phòng trường hợp van đã mở nhưng mất gói ACK
            sendRS485Command(currentValveIndex, false);
            currentValveIndex++;
            if (currentValveIndex > 8) {
              digitalWrite(PIN_RELAY_SOURCE, LOW);
              currentState = FINISH;
            } else {
              stateTimer = millis();
              currentState = OPEN_VALVE;
            }
          }
        }
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
      currentState = OPEN_NEXT_VALVE;
      break;

    case OPEN_NEXT_VALVE:
      // Đợi 3s cho nguồn ổn định (theo yêu cầu)
      if (millis() - stateTimer > 3000) { 
        int nextValve = currentValveIndex + 1;
        Serial.printf("Chuyen Van: Dong %d, Mo %d\n", currentValveIndex, nextValve);
        addToLog("Đóng Van " + String(currentValveIndex) + ", Mở Van " + String(nextValve));
        
        // Gửi lệnh Đóng van cũ trước
        sendRS485Command(currentValveIndex, false);
        delay(200); // Delay nhỏ để tránh dính gói tin
        // Gửi lệnh Mở van mới
        sendRS485Command(nextValve, true);

        feedbackReceived = false;
        retryCount = 0;
        stateTimer = millis();
        currentState = WAIT_NEXT_VALVE_FEEDBACK;
      }
      break;

    case WAIT_NEXT_VALVE_FEEDBACK:
      // Chờ phản hồi VÀ chờ đủ 20s cho quá trình đóng/mở hoàn tất
      if (feedbackReceived && (millis() - stateTimer > 20000)) {
        currentState = SWITCH_SOURCE_OFF; // Chuyển thẳng sang tắt nguồn
        stateTimer = millis();
      } else if (millis() - stateTimer > 40000) { 
        retryCount++;
        int nextValve = currentValveIndex + 1;
        if (retryCount < 3) {
          addToLog("<span class='text-warning'>Timeout Van " + String(nextValve) + ". Thử lại lần " + String(retryCount) + "...</span>");
          sendRS485Command(nextValve, true); // Gửi lại lệnh
          stateTimer = millis(); // Reset timer
        } else {
          int slave = (nextValve <= 4) ? 1 : 2;
          if (slave == 2) {
            slave2Connected = false;
            addToLog("<span class='text-danger'>LỖI: Slave 2 không phản hồi Van " + String(nextValve) + ". Dừng.</span>");
            addToFaultLog("Mất kết nối Slave 2");
            currentState = STOP_PUMP;
          } else {
            slave1Connected = false;
            addToLog("<span class='text-warning'>Slave 1 lỗi Van " + String(nextValve) + ". Bỏ qua.</span>");
            addToFaultLog("Lỗi Slave 1 - Van " + String(nextValve));
            // Đóng van hiện tại
            sendRS485Command(currentValveIndex, false);
            // AN TOÀN: Gửi lệnh đóng van kế tiếp phòng trường hợp nó đã mở nhưng mất ACK
            sendRS485Command(nextValve, false);
            currentValveIndex++; // Bỏ qua van lỗi
            if (currentValveIndex < 8) {
              delay(500);
              stateTimer = millis();
              currentState = SWITCH_SOURCE_ON; // Thử van tiếp theo
            } else {
              currentState = STOP_PUMP;
            }
          }
        }
      }
      break;

    case CLOSE_PREV_VALVE:
      // State này không còn dùng nữa vì đã gửi lệnh đóng ở OPEN_NEXT_VALVE
      // Chuyển tiếp sang tắt nguồn
      currentState = SWITCH_SOURCE_OFF;
      stateTimer = millis();
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
      retryCount = 0; // Dùng biến này làm cờ đánh dấu đã gửi lệnh hay chưa
      stateTimer = millis();
      currentState = CLOSE_LAST_VALVE;
      break;

    case CLOSE_LAST_VALVE:
      // 1. Đợi nguồn ổn định 3s
      if (retryCount == 0 && millis() - stateTimer > 3000) {
        sendRS485Command(currentValveIndex, false); // Gửi lệnh đóng
        retryCount = 1; // Đánh dấu đã gửi
      }
      
      // 2. Đợi thêm 20s cho van đóng hẳn (Tổng cộng > 23s từ lúc bật nguồn)
      if (retryCount == 1 && millis() - stateTimer > 23000) {
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