#include "Arduino.h"
#include "WiFi.h"
#include "esp_camera.h"
#include "soc/soc.h"           // Vô hiệu hóa các vấn đề về brownout
#include "soc/rtc_cntl_reg.h"  // Vô hiệu hóa các vấn đề về brownout
#include "driver/rtc_io.h"
#include <LittleFS.h>
#include <FS.h>
#include <Firebase_ESP_Client.h>
// Cung cấp thông tin về quá trình tạo token
#include <addons/TokenHelper.h>
#include <time.h>

// Thay thế bằng thông tin mạng của bạn
const char* ssid = "P302";
const char* password = "0102030405";

// Chèn API Key của dự án Firebase
#define API_KEY "AIzaSyBFq2fpTW-8RUBJGKF9HG2cx1dXe9R9XTw"

// Chèn Email và Mật khẩu được ủy quyền
#define USER_EMAIL "tungpham010203@gmail.com"
#define USER_PASSWORD "123123"

// Chèn ID bucket lưu trữ Firebase, ví dụ bucket-name.appspot.com
#define STORAGE_BUCKET_ID "aquariummanagement-ef751.appspot.com"

// Tên file ảnh để lưu trong LittleFS
// Bỏ định nghĩa FILE_PHOTO_PATH vì chúng ta sẽ sử dụng biến toàn cục

#define BUCKET_PHOTO "/data/photo.jpg"

// Đường dẫn đến Realtime Database
#define DATABASE_URL "https://aquariummanagement-ef751-default-rtdb.asia-southeast1.firebasedatabase.app/"

// Cấu hình chân cho module camera OV2640 (CAMERA_MODEL_AI_THINKER)
#define PWDN_GPIO_NUM     32
#define RESET_GPIO_NUM    -1
#define XCLK_GPIO_NUM      0
#define SIOD_GPIO_NUM     26
#define SIOC_GPIO_NUM     27
#define Y9_GPIO_NUM       35
#define Y8_GPIO_NUM       34
#define Y7_GPIO_NUM       39
#define Y6_GPIO_NUM       36
#define Y5_GPIO_NUM       21
#define Y4_GPIO_NUM       19
#define Y3_GPIO_NUM       18
#define Y2_GPIO_NUM        5
#define VSYNC_GPIO_NUM    25
#define HREF_GPIO_NUM     23
#define PCLK_GPIO_NUM     22

// Định nghĩa các đối tượng Firebase Data
FirebaseData fbdo;
FirebaseAuth auth;
FirebaseConfig configF;

// Biến toàn cục để lưu tên file ảnh
String filePath;

// Callback cho quá trình tải lên Firebase Storage
void fcsUploadCallback(FCS_UploadStatusInfo info);

// Các biến điều khiển
bool takePhoto = false;
int airPumpSpeed = 0;
bool bigLight = false;
bool waterPump = false;

int airPumpSpeed2 = 0;
bool bigLight2 = false;
bool waterPump2 = false;

unsigned long lastCheck = 0;
unsigned long interval = 5000;  // Kiểm tra mỗi 5 giây

// Chụp ảnh và lưu vào LittleFS
void capturePhotoSaveLittleFS() {
  // Bỏ qua các ảnh đầu tiên vì chất lượng kém
  camera_fb_t* fb = NULL;
  for (int i = 0; i < 4; i++) {
    fb = esp_camera_fb_get();
    esp_camera_fb_return(fb);
    fb = NULL;
  }

  // Chụp ảnh mới
  fb = esp_camera_fb_get();
  if (!fb) {
    Serial.println("Không thể chụp ảnh");
    return;
  }

  // Lấy thời gian hiện tại và định dạng thành hhmmss_ddmmyy
  struct tm timeinfo;
  if (!getLocalTime(&timeinfo)) {
    Serial.println("Không thể lấy thời gian hiện tại");
    esp_camera_fb_return(fb);
    return;
  }
  char timeStr[30];
  strftime(timeStr, sizeof(timeStr), "%H%M%S_%d%m%y.jpg", &timeinfo);

  // Cập nhật tên file với thời gian
  filePath = "/" + String(timeStr);
  Serial.printf("Tên file ảnh: %s\n", filePath.c_str());

  // Lưu ảnh vào LittleFS
  File file = LittleFS.open(filePath, FILE_WRITE);
  if (!file) {
    Serial.println("Không thể mở file để ghi");
  }
  else {
    file.write(fb->buf, fb->len); // payload (ảnh), độ dài payload
    Serial.printf("Ảnh đã được lưu vào %s - Kích thước: %d bytes\n", filePath.c_str(), fb->len);
  }

  // Đóng file và trả lại bộ nhớ cho camera
  file.close();
  esp_camera_fb_return(fb);
}

// Khởi tạo kết nối WiFi
void initWiFi() {
  WiFi.begin(ssid, password);
  while (WiFi.status() != WL_CONNECTED) {
    delay(1000);
    Serial.println("Đang kết nối WiFi...");
  }
  Serial.println("Đã kết nối WiFi");
}

// Khởi tạo LittleFS
void initLittleFS() {
  if (!LittleFS.begin(true)) {
    Serial.println("Đã xảy ra lỗi khi mount LittleFS");
    ESP.restart();
  }
  else {
    delay(500);
    Serial.println("LittleFS đã được mount thành công");
  }
}

// Khởi tạo camera
void initCamera() {
  // Cấu hình module camera OV2640
  camera_config_t config;
  config.ledc_channel = LEDC_CHANNEL_0;
  config.ledc_timer = LEDC_TIMER_0;
  config.pin_d0 = Y2_GPIO_NUM;
  config.pin_d1 = Y3_GPIO_NUM;
  config.pin_d2 = Y4_GPIO_NUM;
  config.pin_d3 = Y5_GPIO_NUM;
  config.pin_d4 = Y6_GPIO_NUM;
  config.pin_d5 = Y7_GPIO_NUM;
  config.pin_d6 = Y8_GPIO_NUM;
  config.pin_d7 = Y9_GPIO_NUM;
  config.pin_xclk = XCLK_GPIO_NUM;
  config.pin_pclk = PCLK_GPIO_NUM;
  config.pin_vsync = VSYNC_GPIO_NUM;
  config.pin_href = HREF_GPIO_NUM;
  config.pin_sscb_sda = SIOD_GPIO_NUM;
  config.pin_sscb_scl = SIOC_GPIO_NUM;
  config.pin_pwdn = PWDN_GPIO_NUM;
  config.pin_reset = RESET_GPIO_NUM;
  config.xclk_freq_hz = 20000000;
  config.pixel_format = PIXFORMAT_JPEG;
  config.grab_mode = CAMERA_GRAB_LATEST;

  if (psramFound()) {
    config.frame_size = FRAMESIZE_SVGA; // Giảm từ FRAMESIZE_UXGA xuống SVGA
    config.jpeg_quality = 15;           // Tăng từ 10 lên 15
    config.fb_count = 1;
  } else {
    config.frame_size = FRAMESIZE_CIF;  // Giảm xuống CIF nếu không có PSRAM
    config.jpeg_quality = 20;           // Tăng từ 12 lên 20
    config.fb_count = 1;
  }

  // Khởi tạo camera
  esp_err_t err = esp_camera_init(&config);
  if (err != ESP_OK) {
    Serial.printf("Khởi tạo camera thất bại với lỗi 0x%x", err);
    ESP.restart();
  }
}

// Hàm gửi dữ liệu tới Arduino Nano
void sendToNano(int airPumpSpeed, bool bigLight, bool waterPump) {
  char buffer[50];
  sprintf(buffer, "TXofESP32(%d,%d,%d)", airPumpSpeed, bigLight, waterPump);
  Serial.println(buffer);
}

// Hàm nhận dữ liệu từ Arduino Nano
void receiveFromNano() {
  if (Serial.available()) {
    String data = Serial.readStringUntil('\n');
    if (data.startsWith("TXofNANO(")) {
      int values[3];
      sscanf(data.c_str(), "TXofNANO(%d,%d,%d)", &values[0], &values[1], &values[2]);

      airPumpSpeed = values[0];
      bigLight = values[1];
      waterPump = values[2];

      if (airPumpSpeed != airPumpSpeed2 || bigLight != bigLight2 || waterPump != waterPump2) {
        // Update Firebase với giá trị mới
        if (Firebase.RTDB.setInt(&fbdo, "/airPumpSpeed", airPumpSpeed) &&
            Firebase.RTDB.setBool(&fbdo, "/bigLight", bigLight) &&
            Firebase.RTDB.setBool(&fbdo, "/waterPump", waterPump)) {
          Serial.println("Firebase updated successfully");
        } else {
          Serial.println("Firebase update failed");
        }
        airPumpSpeed2 = airPumpSpeed;
        bigLight2 = bigLight;
        waterPump2 = waterPump;
      }
    }
  }
}

// Hàm khởi tạo thời gian sử dụng NTP và đặt múi giờ
void initTime() {
  const long gmtOffset_sec = 7 * 3600;  // GMT+7 cho Việt Nam
  const int daylightOffset_sec = 0;
  configTime(gmtOffset_sec, daylightOffset_sec, "pool.ntp.org", "time.nist.gov");

  struct tm timeinfo;
  if (!getLocalTime(&timeinfo)) {
    Serial.println("Không thể đồng bộ thời gian với NTP");
    return;
  }
  Serial.println("Đã đồng bộ thời gian thành công");
}

// Hàm setup
void setup() {
  // Khởi tạo cổng Serial để debug
  Serial.begin(9600);
  initWiFi();
  initLittleFS();

  // Tắt 'brownout detector'
  WRITE_PERI_REG(RTC_CNTL_BROWN_OUT_REG, 0);

  // Khởi tạo camera
  initCamera();

  // Khởi tạo thời gian
  initTime();

  // Cấu hình Firebase
  // Gán API key
  configF.api_key = API_KEY;
  // Gán URL của Realtime Database
  configF.database_url = DATABASE_URL;
  // Gán thông tin đăng nhập người dùng
  auth.user.email = USER_EMAIL;
  auth.user.password = USER_PASSWORD;
  // Gán hàm callback cho quá trình tạo token dài hạn
  configF.token_status_callback = tokenStatusCallback; // xem addons/TokenHelper.h

  Firebase.begin(&configF, &auth);
  Firebase.reconnectWiFi(true);

  if (Firebase.ready()) {
    // Đọc giá trị từ Realtime Database
    if (Firebase.RTDB.getBool(&fbdo, "/takePhoto")) {
      takePhoto = fbdo.boolData();
    }
    if (Firebase.RTDB.getInt(&fbdo, "/airPumpSpeed")) {
      airPumpSpeed = fbdo.intData();
    }
    if (Firebase.RTDB.getBool(&fbdo, "/bigLight")) {
      bigLight = fbdo.boolData();
    }
    if (Firebase.RTDB.getBool(&fbdo, "/waterPump")) {
      waterPump = fbdo.boolData();
    }

    // In ra các giá trị đã đọc được
    Serial.println("Giá trị đọc được từ Firebase:");
    Serial.printf("takePhoto: %s\n", takePhoto ? "true" : "false");
    Serial.printf("airPumpSpeed: %d\n", airPumpSpeed);
    Serial.printf("bigLight: %s\n", bigLight ? "true" : "false");
    Serial.printf("waterPump: %s\n", waterPump ? "true" : "false");

    if (airPumpSpeed != airPumpSpeed2 || bigLight != bigLight2 || waterPump != waterPump2) {
      // Gửi trạng thái hiện tại tới Arduino Nano
      sendToNano(airPumpSpeed, bigLight, waterPump);
      airPumpSpeed2 = airPumpSpeed;
      bigLight2 = bigLight;
      waterPump2 = waterPump;
    }
  }

  // Kiểm tra dung lượng trống của LittleFS
  checkLittleFSSpace();
  delay(100);
  // formatLittleFS();
  // delay(100);
  // checkLittleFSSpace();
}

// Hàm loop
void loop() {
  // Kiểm tra Firebase sau mỗi khoảng thời gian đã định
  if (millis() - lastCheck > interval) {
    lastCheck = millis();

    if (Firebase.ready()) {
      // Đọc giá trị từ Firebase
      if (Firebase.RTDB.getBool(&fbdo, "/takePhoto")) {
        takePhoto = fbdo.boolData();
      }
      if (Firebase.RTDB.getInt(&fbdo, "/airPumpSpeed")) {
        airPumpSpeed = fbdo.intData();
      }
      if (Firebase.RTDB.getBool(&fbdo, "/bigLight")) {
        bigLight = fbdo.boolData();
      }
      if (Firebase.RTDB.getBool(&fbdo, "/waterPump")) {
        waterPump = fbdo.boolData();
      }

      // Gửi trạng thái tới Arduino Nano nếu có thay đổi
      if (airPumpSpeed != airPumpSpeed2 || bigLight != bigLight2 || waterPump != waterPump2) {
        sendToNano(airPumpSpeed, bigLight, waterPump);
        airPumpSpeed2 = airPumpSpeed;
        bigLight2 = bigLight;
        waterPump2 = waterPump;
      }

      // Chụp ảnh nếu cần
      if (takePhoto) {
        Serial.println("Bắt đầu chụp ảnh...");
        capturePhotoSaveLittleFS();
        Serial.print("Đang tải ảnh lên Firebase Storage... ");

        if (Firebase.Storage.upload(&fbdo, STORAGE_BUCKET_ID, filePath.c_str(), mem_storage_type_flash, filePath.c_str(), "image/jpeg", fcsUploadCallback)) {
          Serial.printf("\nURL Tải xuống: %s\n", fbdo.downloadURL().c_str());
          if (Firebase.RTDB.setBool(&fbdo, "/takePhoto", false)) {
            Serial.println("Đã đặt lại trạng thái takePhoto thành false");
          } else {
            Serial.println("Lỗi khi đặt lại trạng thái takePhoto");
          }
        } else {
          Serial.println("Lỗi khi tải ảnh lên: " + fbdo.errorReason());
        }
      }
    }
  }

  // Kiểm tra cập nhật từ Arduino Nano
  receiveFromNano();
}

// Hàm callback cho quá trình tải lên Firebase Storage
void fcsUploadCallback(FCS_UploadStatusInfo info) {
  if (info.status == firebase_fcs_upload_status_init) {
    Serial.printf("Đang tải lên file %s (%d) đến %s\n", info.localFileName.c_str(), info.fileSize, info.remoteFileName.c_str());
  }
  else if (info.status == firebase_fcs_upload_status_upload) {
    Serial.printf("Đã tải lên %d%%, Thời gian đã trôi qua %d ms\n", (int)info.progress, info.elapsedTime);
  }
  else if (info.status == firebase_fcs_upload_status_complete) {
    Serial.println("Tải lên hoàn tất\n");
    FileMetaInfo meta = fbdo.metaData();
    Serial.printf("Tên: %s\n", meta.name.c_str());
    Serial.printf("Bucket: %s\n", meta.bucket.c_str());
    Serial.printf("Loại nội dung: %s\n", meta.contentType.c_str());
    Serial.printf("Kích thước: %d\n", meta.size);
    Serial.printf("Thế hệ: %lu\n", meta.generation);
    Serial.printf("Metageneration: %lu\n", meta.metageneration);
    Serial.printf("ETag: %s\n", meta.etag.c_str());
    Serial.printf("CRC32: %s\n", meta.crc32.c_str());
    Serial.printf("Tokens: %s\n", meta.downloadTokens.c_str());
    Serial.printf("URL Tải xuống: %s\n\n", fbdo.downloadURL().c_str());

    // Xóa file sau khi tải lên thành công
    if (LittleFS.exists(filePath)) {
      if (LittleFS.remove(filePath)) {
        Serial.printf("Đã xóa file %s sau khi tải lên thành công.\n", filePath.c_str());
      } else {
        Serial.printf("Không thể xóa file %s.\n", filePath.c_str());
      }
    }
  }
  else if (info.status == firebase_fcs_upload_status_error) {
    Serial.printf("Tải lên thất bại, %s\n", info.errorMsg.c_str());
  }
}

// Hàm kiểm tra dung lượng trống của LittleFS
void checkLittleFSSpace() {
  unsigned long totalBytes = LittleFS.totalBytes();
  unsigned long usedBytes = LittleFS.usedBytes();

  Serial.printf("Dung lượng khả dụng: %lu bytes\n", totalBytes - usedBytes);
  Serial.printf("Dung lượng đã sử dụng: %lu bytes\n", usedBytes);
}
void formatLittleFS() {
  if (LittleFS.format()) {
    Serial.println("Định dạng LittleFS thành công.");
  } else {
    Serial.println("Định dạng LittleFS thất bại.");
  }
}
