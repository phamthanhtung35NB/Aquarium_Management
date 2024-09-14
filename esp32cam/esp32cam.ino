#include "Arduino.h"
#include "WiFi.h"
#include "esp_camera.h"
#include "soc/soc.h"           // Vô hiệu hóa các vấn đề về brownout
#include "soc/rtc_cntl_reg.h"  // Vô hiệu hóa các vấn đề về brownout
#include "driver/rtc_io.h"
#include <LittleFS.h>
#include <FS.h>
#include <Firebase_ESP_Client.h>
//Cung cấp thông tin về quá trình tạo token
#include <addons/TokenHelper.h>

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
#define FILE_PHOTO_PATH "/photo.jpg"
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

void fcsUploadCallback(FCS_UploadStatusInfo info);

bool takePhoto = false;
int airPumpSpeed = 0;
bool bigLight = false;
bool smallLight = false;
bool waterPump = false;

// Chụp ảnh và lưu vào LittleFS
void capturePhotoSaveLittleFS( void ) {
  // Bỏ qua các ảnh đầu tiên vì chất lượng kém
  camera_fb_t* fb = NULL;
  // Bỏ qua 3 khung hình đầu tiên (có thể tăng/giảm số lượng nếu cần)
  for (int i = 0; i < 4; i++) {
    fb = esp_camera_fb_get();
    esp_camera_fb_return(fb);
    fb = NULL;
  }
    
  // Chụp ảnh mới
  fb = NULL;  
  fb = esp_camera_fb_get();  
  if(!fb) {
    Serial.println("Không thể chụp ảnh");
    return;
  }  

  // Tên file ảnh
  Serial.printf("Tên file ảnh: %s\n", FILE_PHOTO_PATH);
  File file = LittleFS.open(FILE_PHOTO_PATH, FILE_WRITE);

  // Chèn dữ liệu vào file ảnh
  if (!file) {
    Serial.println("Không thể mở file để ghi");
  }
  else {
    file.write(fb->buf, fb->len); // payload (ảnh), độ dài payload
    Serial.print("Ảnh đã được lưu vào ");
    Serial.print(FILE_PHOTO_PATH);
    Serial.print(" - Kích thước: ");
    Serial.print(fb->len);
    Serial.println(" bytes");
  }
  // Đóng file
  file.close();
  esp_camera_fb_return(fb);
}

// Khởi tạo kết nối WiFi
void initWiFi(){
  WiFi.begin(ssid, password);
  while (WiFi.status() != WL_CONNECTED) {
    delay(1000);
    Serial.println("Đang kết nối WiFi...");
  }
  Serial.println("Đã kết nối WiFi");
}

// Khởi tạo LittleFS
void initLittleFS(){
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
void initCamera(){
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
    config.frame_size = FRAMESIZE_UXGA;
    config.jpeg_quality = 10;
    config.fb_count = 1;
  } else {
    config.frame_size = FRAMESIZE_SVGA;
    config.jpeg_quality = 12;
    config.fb_count = 1;
  }
  // Khởi tạo camera
  esp_err_t err = esp_camera_init(&config);
  if (err != ESP_OK) {
    Serial.printf("Khởi tạo camera thất bại với lỗi 0x%x", err);
    ESP.restart();
  } 
}

void setup() {
  // Khởi tạo cổng Serial để debug
  Serial.begin(115200);
  initWiFi();
  initLittleFS();
  // Tắt 'brownout detector'
  WRITE_PERI_REG(RTC_CNTL_BROWN_OUT_REG, 0);
  initCamera();

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
}

void loop() {
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
    if (Firebase.RTDB.getBool(&fbdo, "/smallLight")) {
      smallLight = fbdo.boolData();
    }
    if (Firebase.RTDB.getBool(&fbdo, "/waterPump")) {
      waterPump = fbdo.boolData();
    }

    // In ra các giá trị đã đọc được
    Serial.println("Giá trị đọc được từ Firebase:");
    Serial.printf("takePhoto: %s\n", takePhoto ? "true" : "false");
    Serial.printf("airPumpSpeed: %d\n", airPumpSpeed);
    Serial.printf("bigLight: %s\n", bigLight ? "true" : "false");
    Serial.printf("smallLight: %s\n", smallLight ? "true" : "false");
    Serial.printf("waterPump: %s\n", waterPump ? "true" : "false");

    if (takePhoto) {
      Serial.println("Bắt đầu chụp ảnh...");
      capturePhotoSaveLittleFS();
      Serial.print("Đang tải ảnh lên Firebase Storage... ");

      if (Firebase.Storage.upload(&fbdo, STORAGE_BUCKET_ID, FILE_PHOTO_PATH, mem_storage_type_flash, BUCKET_PHOTO, "image/jpeg", fcsUploadCallback)){
        Serial.printf("\nURL Tải xuống: %s\n", fbdo.downloadURL().c_str());
        
        // Đặt lại trạng thái takePhoto
        if (Firebase.RTDB.setBool(&fbdo, "/takePhoto", false)) {
          Serial.println("Đã đặt lại trạng thái takePhoto thành false");
        } else {
          Serial.println("Lỗi khi đặt lại trạng thái takePhoto");
        }
      }
      else {
        Serial.println("Lỗi khi tải ảnh lên: " + fbdo.errorReason());
      }
    }
  }
  delay(5000);  // Đợi 5 giây trước khi kiểm tra lại
}

// Hàm callback cho quá trình tải lên Firebase Storage
void fcsUploadCallback(FCS_UploadStatusInfo info){
    if (info.status == firebase_fcs_upload_status_init){
        Serial.printf("Đang tải lên file %s (%d) đến %s\n", info.localFileName.c_str(), info.fileSize, info.remoteFileName.c_str());
    }
    else if (info.status == firebase_fcs_upload_status_upload)
    {
        Serial.printf("Đã tải lên %d%s, Thời gian đã trôi qua %d ms\n", (int)info.progress, "%", info.elapsedTime);
    }
    else if (info.status == firebase_fcs_upload_status_complete)
    {
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
    }
    else if (info.status == firebase_fcs_upload_status_error){
        Serial.printf("Tải lên thất bại, %s\n", info.errorMsg.c_str());
    }
}