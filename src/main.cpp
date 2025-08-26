#include <M5Unified.h>
#include <SD.h>
#include <FS.h>
#include <WiFi.h>
#include <time.h>

// スクロール用の定数
const int textSize = 2;
const int lineHeight = 8 * textSize; // Font0の高さは約8px

String fileName;

// UBX設定コマンドを送信する関数
void sendUBXConfigurationCommands() {
  Serial.println("Sending UBX configuration commands...");
  
  // UBX-CFG-MSG: UBX-ESF-MEAS メッセージを有効化 (Class: 0x10, ID: 0x02)
  uint8_t ubx_cfg_esf_meas[] = {
    0xB5, 0x62, 0x06, 0x01, 0x08, 0x00,  // Header: SYNC1, SYNC2, CLASS, ID, LENGTH
    0x10, 0x02,                          // ESF-MEAS (0x10, 0x02)
    0x00,                                // Rate on I2C
    0x01,                                // Rate on UART1
    0x00,                                // Rate on UART2
    0x00,                                // Rate on USB
    0x00,                                // Rate on SPI
    0x00,                                // Reserved
    0x22, 0x91                           // Checksum
  };
  
  // UBX-CFG-MSG: UBX-ESF-RAW メッセージを有効化 (Class: 0x10, ID: 0x03)
  uint8_t ubx_cfg_esf_raw[] = {
    0xB5, 0x62, 0x06, 0x01, 0x08, 0x00,  // Header
    0x10, 0x03,                          // ESF-RAW (0x10, 0x03)
    0x00, 0x01, 0x00, 0x00, 0x00, 0x00,  // Rates
    0x23, 0x95                           // Checksum
  };
  
  // UBX-CFG-MSG: UBX-ESF-STATUS メッセージを有効化 (Class: 0x10, ID: 0x10)
  uint8_t ubx_cfg_esf_status[] = {
    0xB5, 0x62, 0x06, 0x01, 0x08, 0x00,  // Header
    0x10, 0x10,                          // ESF-STATUS (0x10, 0x10)
    0x00, 0x01, 0x00, 0x00, 0x00, 0x00,  // Rates
    0x30, 0xDE                           // Checksum
  };

  // UBX-CFG-MSG: UBX-NAV-PVT メッセージを有効化 (Class: 0x01, ID: 0x07)
  uint8_t ubx_cfg_nav_pvt[] = {
    0xB5, 0x62, 0x06, 0x01, 0x08, 0x00,  // Header
    0x01, 0x07,                          // NAV-PVT (0x01, 0x07)
    0x00, 0x01, 0x00, 0x00, 0x00, 0x00,  // Rates
    0x17, 0xDA                           // Checksum
  };

  // UBX-CFG-MSG: UBX-NAV-SIG メッセージを有効化 (Class: 0x01, ID: 0x43)
  uint8_t ubx_cfg_nav_sig[] = {
    0xB5, 0x62, 0x06, 0x01, 0x08, 0x00,  // Header
    0x01, 0x43,                          // NAV-SIG (0x01, 0x43)
    0x00, 0x01, 0x00, 0x00, 0x00, 0x00,  // Rates
    0x53, 0x88                           // Checksum
  };

  // UBX-CFG-MSG: UBX-NAV-SAT メッセージを有効化 (Class: 0x01, ID: 0x35) - NAV-SIGの代替
  uint8_t ubx_cfg_nav_sat[] = {
    0xB5, 0x62, 0x06, 0x01, 0x08, 0x00,  // Header
    0x01, 0x35,                          // NAV-SAT (0x01, 0x35)
    0x00, 0x01, 0x00, 0x00, 0x00, 0x00,  // Rates
    0x42, 0x57                           // Checksum
  };

  // UBX-CFG-MSG: UBX-NAV-DOP メッセージを有効化 (Class: 0x01, ID: 0x04)
  uint8_t ubx_cfg_nav_dop[] = {
    0xB5, 0x62, 0x06, 0x01, 0x08, 0x00,  // Header
    0x01, 0x04,                          // NAV-DOP (0x01, 0x04)
    0x00, 0x01, 0x00, 0x00, 0x00, 0x00,  // Rates
    0x14, 0xC1                           // Checksum
  };
  
  Serial2.write(ubx_cfg_esf_meas, sizeof(ubx_cfg_esf_meas));
  delay(100);
  Serial2.write(ubx_cfg_esf_raw, sizeof(ubx_cfg_esf_raw));
  delay(100);
  Serial2.write(ubx_cfg_esf_status, sizeof(ubx_cfg_esf_status));
  delay(100);
  Serial2.write(ubx_cfg_nav_pvt, sizeof(ubx_cfg_nav_pvt));
  delay(100);
  Serial2.write(ubx_cfg_nav_sig, sizeof(ubx_cfg_nav_sig));
  delay(100);
  Serial2.write(ubx_cfg_nav_sat, sizeof(ubx_cfg_nav_sat));
  delay(100);
  Serial2.write(ubx_cfg_nav_dop, sizeof(ubx_cfg_nav_dop));
  delay(100);
  
  Serial.println("UBX configuration commands sent");
}


void setup() {
  auto cfg = M5.config();
  cfg.output_power = true;   // 5VレールON
  cfg.serial_baudrate = 115200;
  M5.begin(cfg);

  M5.Display.setTextSize(textSize);
  M5.Display.setCursor(0, 0);
  M5.Display.setTextColor(TFT_WHITE);
  
  if (!M5.Rtc.isEnabled()) {
    Serial.println("RTC not found.");
    M5.Display.println("RTC not found.");
    for (;;) {
      vTaskDelay(500);
    }
  }
  Serial.println("RTC found.");
  M5.Display.println("RTC found.");

  M5.Rtc.setDateTime( { { 2010, 1, 1 }, { 12, 00, 00 } } );
  // WiFiに接続して時刻同期
  WiFi.begin("guestmode", "Conch6aqua");
  // WiFi.begin("aterm-2c854f-g", "910c86fcfc7a4");
  M5.Display.println("WiFi connecting...");
  int wifiTimeout = 10; // 10秒でタイムアウト
  while (WiFi.status() != WL_CONNECTED && wifiTimeout > 0) {
    delay(1000);
    M5.Display.print(".");
    wifiTimeout--;
  }
  
  if (WiFi.status() == WL_CONNECTED) {
    M5.Display.println("\nWiFi Connected!");
    M5.Display.printf("IP: %s\n", WiFi.localIP().toString().c_str());
    
    // NTPで時刻同期
    M5.Display.print("NTP sync...");

    struct tm timeinfo;
    // configTzTime(NTP_TIMEZONE, NTP_SERVER1, NTP_SERVER2, NTP_SERVER3);
    configTime(0, 0, "pool.ntp.org", "time.nist.gov", "ntp.jst.mfeed.ad.jp");

    while (!getLocalTime(&timeinfo, 1000)) {
      Serial.print('.');
      M5.Display.print(".");
    }
    M5.Display.print("Time synced!\n");
    // // NTPで取得した時刻をRTCに設定
    m5::rtc_datetime_t dt;
    dt.date.year   = timeinfo.tm_year + 1900;
    dt.date.month  = timeinfo.tm_mon + 1;
    dt.date.date   = timeinfo.tm_mday;
    dt.time.hours  = timeinfo.tm_hour;
    dt.time.minutes= timeinfo.tm_min;
    dt.time.seconds= timeinfo.tm_sec;
    M5.Rtc.setDateTime(dt);
    
    M5.Display.printf("RTC set: %04d/%02d/%02d %02d:%02d:%02d\n",
                      dt.date.year, dt.date.month, dt.date.date,
                      dt.time.hours, dt.time.minutes, dt.time.seconds);
    // WiFi切断
    WiFi.disconnect();
  } else {
    M5.Display.println("\nWiFi failed!");
    M5.Display.println("Using RTC time...");

  }
  delay(500);

  // SDカード初期化（SPIピン設定も含む）
  M5.Display.println("--- SD Card Init ---");
  if (!SD.begin(GPIO_NUM_4, SPI, 25000000))
  {
    delay(100);
    M5.Display.clear();
    M5.Display.setCursor(0, 0);
    M5.Display.setTextColor(TFT_RED);
    M5.Display.println("NO SD CARD");
    M5.Display.println("Please insert SD card");
    while (true)
      ;
  }
  M5.Display.setTextColor(TFT_GREEN);
  M5.Display.println("SD init OK");
  if (!SD.exists("/ublox_data"))
  {
    SD.mkdir("/ublox_data");
  }

  // RTCから時刻を読み出してファイル名生成
  auto dt = M5.Rtc.getDateTime();
  char timeFileName[64];
  sprintf(timeFileName, "/ublox_data/%04d%02d%02d_%02d%02d%02d.bin",
          dt.date.year, dt.date.month, dt.date.date,
          dt.time.hours + 9, dt.time.minutes, dt.time.seconds);
  fileName = String(timeFileName);

  //書き込みテスト
  File f = SD.open(fileName, FILE_APPEND);
  if (f) {
    f.println("timestamp,data");  // CSVヘッダー
    f.println("=== UART Viewer Start ===");
    f.close();
    M5.Display.println("SD write OK");
    delay(200);
  } else {
    M5.Display.setCursor(0, 0);
    M5.Display.setTextColor(TFT_RED);
    M5.Display.println("SD write failed!");
    while (true) 
      ;
  }

  M5.Display.setTextColor(TFT_WHITE);
  M5.Display.printf("Current time: %04d/%02d/%02d %02d:%02d:%02d\n",
                    dt.date.year, dt.date.month, dt.date.date,
                    dt.time.hours, dt.time.minutes, dt.time.seconds);
  M5.Display.printf("File: %s\n", fileName.c_str());
  delay(5000);

  // UART2 を GPIO13(RX2), GPIO14(TX2) で開始
  Serial2.begin(460800, SERIAL_8N1, 13, 14);
  
  // u-bloxデバイスにUBXメッセージ有効化コマンドを送信
  M5.Display.println("Configuring u-blox...");
  delay(1000);
  // sendUBXConfigurationCommands();
  delay(1000);
  
  M5.Display.setRotation(1);
  M5.Display.setFont(&fonts::Font0);
  M5.Display.setTextSize(textSize);
  M5.Display.fillScreen(TFT_BLACK);
  M5.Display.setCursor(0, 0);
  M5.Display.setTextColor(TFT_WHITE);
  M5.Display.println("Mode: UBX Data");
  M5.Display.println("==================");
  M5.Display.setTextColor(TFT_CYAN);
  M5.Display.println("SVs Tracked:");
  M5.Display.setTextColor(TFT_GREEN);
  M5.Display.println("SVs Used:");
  M5.Display.setTextColor(TFT_YELLOW);
  M5.Display.println("SV C/N0:");
  M5.Display.setTextColor(TFT_MAGENTA);
  M5.Display.println("TTFF:");
  M5.Display.setTextColor(TFT_WHITE);
  M5.Display.println("Fix Type:");
  M5.Display.println("Lat:");
  M5.Display.println("Lon:");
  M5.Display.println("Alt:");
  M5.Display.println("2D Acc:");
  M5.Display.println("3D Acc:");
  M5.Display.println("PDOP:");
  M5.Display.println("HDOP:");
}



// 受信データバッファ
#include <string>
#include <vector>
std::string rxLine;  // テキストライン用
std::vector<uint8_t> rawBuffer;  // バイナリデータ用

// メッセージタイプ
enum MessageType {
  MSG_UNKNOWN = 0,
  MSG_UBX = 1,
  MSG_NMEA = 2
};

// UBXメッセージ構造体
struct UBXMessage {
  uint8_t sync1;      // 0xB5
  uint8_t sync2;      // 0x62
  uint8_t msgClass;
  uint8_t msgId;
  uint16_t length;
  std::vector<uint8_t> payload;
  uint8_t checkA;
  uint8_t checkB;
  bool valid;
  String msgName;     // メッセージ名
};

// UBXメッセージタイプ識別関数
String getUBXMessageName(uint8_t msgClass, uint8_t msgId) {
  if (msgClass == 0x10) { // ESF Class
    switch (msgId) {
      case 0x14: return "UBX-ESF-ALG";
      case 0x03: return "UBX-ESF-RAW";
      case 0x04: return "UBX-ESF-CAL";
      case 0x15: return "UBX-ESF-INS";
      case 0x02: return "UBX-ESF-MEAS";
      case 0x10: return "UBX-ESF-STATUS";
      default: return "UBX-ESF-UNKNOWN";
    }
  } else if (msgClass == 0x01) { // NAV Class
    switch (msgId) {
      case 0x07: return "UBX-NAV-PVT";
      case 0x35: return "UBX-NAV-SAT";
      case 0x43: return "UBX-NAV-SIG";
      case 0x04: return "UBX-NAV-DOP";
      case 0x3C: return "UBX-NAV-RELPOSNED";
      case 0x03: return "UBX-NAV-STATUS";
      default: return "UBX-NAV-UNKNOWN";
    }
  } else if (msgClass == 0x02) { // RXM Class
    switch (msgId) {
      case 0x15: return "UBX-RXM-RAWX";
      case 0x13: return "UBX-RXM-SFRBX";
      default: return "UBX-RXM-UNKNOWN";
    }
  }
  return String("UBX-") + String(msgClass, HEX) + "-" + String(msgId, HEX);
}

// NMEAメッセージ構造体
struct NMEAMessage {
  String sentence;    // 完全な文字列
  String talker;      // GP, GL, GN等
  String msgType;     // GGA, RMC等
  std::vector<String> fields;
  bool valid;
};

// メッセージパーサーの状態
enum ParserState {
  WAITING_SYNC,
  PARSING_UBX_HEADER,
  PARSING_UBX_PAYLOAD,
  PARSING_UBX_CHECKSUM,
  PARSING_NMEA
};

// パーサー状態変数
ParserState parserState = WAITING_SYNC;
std::vector<uint8_t> messageBuffer;
UBXMessage currentUBX;
NMEAMessage currentNMEA;
String nmeaBuffer;

// NMEA GGAデータ構造体
struct GGAData {
  String time;        // 時刻 (HHMMSS.SS)
  double latitude;    // 緯度 (度)
  char latDir;        // 緯度方向 (N/S)
  double longitude;   // 経度 (度)
  char lonDir;        // 経度方向 (E/W)
  int quality;        // 品質インジケータ
  int numSat;         // 使用衛星数
  double hdop;        // 水平精度
  double altitude;    // 高度
  bool valid;         // データ有効性
};

// NMEA GSAデータ構造体
struct GSAData {
  String mode;        // M: Manual, A: Automatic
  int fixType;        // 1: 測位不可, 2: 2D測位, 3: 3D測位
  double pdop;        // Position dilution of precision
  double hdop;        // Horizontal dilution of precision
  double vdop;        // Vertical dilution of precision
  bool valid;         // データ有効性
};

// UBXデータ統合構造体
struct UBXState {
  // 時刻情報 (NAV-PVTから取得)
  uint16_t year;          // 年
  uint8_t month;          // 月
  uint8_t day;            // 日
  uint8_t hour;           // 時
  uint8_t minute;         // 分
  uint8_t second;         // 秒
  
  int svs_tracked;        // from NAV-SIG count
  int svs_used;           // from NAV-PVT.numSV
  float cn0_avg_dbhz;     // avg of NAV-SIG.cno[]
  float ttff_s;           // NAV-PVT.ttff / 1000.0
  int fix_type;           // NAV-PVT.fixType
  double lat_deg, lon_deg;// NAV-PVT lat/lon / 1e7
  float alt_m;            // NAV-PVT.hMSL / 1000.0
  float acc2d_m;          // NAV-PVT.hAcc / 1000.0
  float acc3d_m;          // sqrt(hAcc^2 + vAcc^2) / 1000.0
  float pdop, hdop;       // NAV-DOP / 100.0
  bool valid;             // データ有効性
};

// データパース用の構造体
struct ParsedData {
  String timestamp;
  String field1;
  String field2; 
  String field3;
  bool valid;
};


// 表示モード
enum DisplayMode {
  MODE_UBX = 0,      // UBX情報表示（1ページ目）
  MODE_GGA = 1,      // GGA情報表示（2ページ目）
  MODE_HEX = 2,      // 16進表示
  MODE_UBX_RAW = 3,  // UBXメッセージ表示
  MODE_NMEA = 4,     // NMEAメッセージ表示
  MODE_PARSED = 5,   // パースデータ表示
  MODE_FIELD1 = 6,   // フィールド1のみ
  MODE_FIELD2 = 7,   // フィールド2のみ
  MODE_FIELD3 = 8    // フィールド3のみ
};

DisplayMode currentMode = MODE_UBX;  // デフォルトをUBXモードに変更
ParsedData lastParsedData;
GGAData lastGGA;
GSAData lastGSA;
UBXState ubxState;

// UBX NAV-STATUSメッセージパーサー
void parseUBXNavSTATUS(const std::vector<uint8_t>& payload) {
  if (payload.size() < 16) return; // NAV-STATUSは16バイト
  uint8_t gpsFix = payload[4]; // GPSfix (0: no fix, 2: 2D, 3: 3D, etc)
  uint8_t flags = payload[5];  // flags
  uint32_t ttff = (payload[8]) | (payload[9] << 8) | (payload[10] << 16) | (payload[11] << 24); // ms (offset 8-11)
  uint32_t msss = (payload[12]) | (payload[13] << 8) | (payload[14] << 16) | (payload[15] << 24); // ms since startup (offset 12-15)
  ubxState.ttff_s = ttff / 1000.0f;
  Serial.printf("[DEBUG] NAV-STATUS: gpsFix=%d, flags=0x%02X, TTFF=%u ms (%.2f s), MSSS=%u ms\n", gpsFix, flags, ttff, ubxState.ttff_s, msss);
}

// GGA表示の初期化フラグ
bool ggaDisplayInitialized = false;
bool ubxDisplayInitialized = false;

// UBX表示を初期化する関数
void initUBXDisplay() {
  M5.Display.fillScreen(TFT_BLACK);
  M5.Display.setCursor(0, 0);
  M5.Display.setTextColor(TFT_WHITE);
  M5.Display.println("Mode: UBX Data");
  M5.Display.println("==================");
  M5.Display.setTextColor(TFT_GREEN);
  M5.Display.println("Time:");
  M5.Display.setTextColor(TFT_CYAN);
  M5.Display.println("SVs Tracked:");
  M5.Display.setTextColor(TFT_GREEN);
  M5.Display.println("SVs Used:");
  M5.Display.setTextColor(TFT_YELLOW);
  M5.Display.println("SV C/N0:");
  M5.Display.setTextColor(TFT_MAGENTA);
  M5.Display.println("TTFF:");
  M5.Display.setTextColor(TFT_WHITE);
  M5.Display.println("Fix Type:");
  M5.Display.println("Lat:");
  M5.Display.println("Lon:");
  M5.Display.println("Alt:");
  M5.Display.println("2D Acc:");
  M5.Display.println("3D Acc:");
  ubxDisplayInitialized = true;
}

// GGA表示を初期化する関数
void initGGADisplay() {
  M5.Display.fillScreen(TFT_BLACK);
  M5.Display.setCursor(0, 0);
  M5.Display.setTextColor(TFT_WHITE);
  M5.Display.println("Mode: GPS GGA Data");
  M5.Display.println("=====================");
  M5.Display.setTextColor(TFT_CYAN);
  M5.Display.println("Time:");
  M5.Display.setTextColor(TFT_GREEN);
  M5.Display.println("Lat: ");
  M5.Display.setTextColor(TFT_YELLOW);
  M5.Display.println("Lon: ");
  M5.Display.setTextColor(TFT_MAGENTA);
  M5.Display.println("Sats:");
  M5.Display.setTextColor(TFT_WHITE);
  M5.Display.println("Alt: ");
  M5.Display.println("HDOP:");
  M5.Display.println("Fix: ");
  M5.Display.println("Mode:");  // GSA測位ステータス行を追加
  ggaDisplayInitialized = true;
}

// UBX数値のみを更新する関数
void updateUBXValues() {
  if (!ubxDisplayInitialized) {
    initUBXDisplay();
  }
  
  if (ubxState.valid) {
    // 数値表示エリア全体を一括クリア（範囲を拡大して確実にリフレッシュ）
    M5.Display.fillRect(150, 2 * lineHeight, 170, 11 * lineHeight, TFT_BLACK);
    
    // 時刻更新（3行目）- 一番上に表示（年なし）
    M5.Display.setCursor(150, 2 * lineHeight);
    M5.Display.setTextColor(TFT_GREEN);
    if (ubxState.year > 1900) {  // 有効な年のチェック
      M5.Display.printf("%02d/%02d %02d:%02d:%02d", 
                       ubxState.month, ubxState.day,
                       ubxState.hour, ubxState.minute, ubxState.second);
    } else {
      M5.Display.printf("No Time");
    }
    
    // SVs Tracked更新（4行目）
    M5.Display.setCursor(150, 3 * lineHeight);
    M5.Display.setTextColor(TFT_CYAN);
    M5.Display.printf("%d", ubxState.svs_tracked);
    
    // SVs Used更新（5行目）
    M5.Display.setCursor(150, 4 * lineHeight);
    M5.Display.setTextColor(TFT_GREEN);
    M5.Display.printf("%d", ubxState.svs_used);
    
    // SV C/N0更新（6行目）
    M5.Display.setCursor(150, 5 * lineHeight);
    M5.Display.setTextColor(TFT_YELLOW);
    M5.Display.printf("%.1f dB-Hz", ubxState.cn0_avg_dbhz);
    
    // TTFF更新（7行目）
    M5.Display.setCursor(150, 6 * lineHeight);
    M5.Display.setTextColor(TFT_MAGENTA);
    M5.Display.printf("%.1f s", ubxState.ttff_s);
    
    // Fix Type更新（8行目）
    M5.Display.setCursor(150, 7 * lineHeight);
    M5.Display.setTextColor(ubxState.fix_type > 1 ? TFT_GREEN : TFT_RED);
    M5.Display.printf("%s", 
                     ubxState.fix_type == 0 ? "NoFix" :
                     ubxState.fix_type == 1 ? "Dead-reckoning" :
                     ubxState.fix_type == 2 ? "2D" :
                     ubxState.fix_type == 3 ? "3D" : 
                     ubxState.fix_type == 4 ? "GNSS+DR" : "time only fix");
    
    // Lat更新（9行目）
    M5.Display.setCursor(150, 8 * lineHeight);
    M5.Display.setTextColor(TFT_WHITE);
    M5.Display.printf("%.6f", ubxState.lat_deg);
    
    // Lon更新（10行目）
    M5.Display.setCursor(150, 9 * lineHeight);
    M5.Display.setTextColor(TFT_WHITE);
    M5.Display.printf("%.6f", ubxState.lon_deg);
    
    // Alt更新（11行目）
    M5.Display.setCursor(150, 10 * lineHeight);
    M5.Display.setTextColor(TFT_WHITE);
    M5.Display.printf("%.1f m", ubxState.alt_m);
    
    // 2D Acc更新（12行目）
    M5.Display.setCursor(150, 11 * lineHeight);
    M5.Display.setTextColor(TFT_WHITE);
    M5.Display.printf("%.1f m", ubxState.acc2d_m);
    
    // 3D Acc更新（13行目）
    M5.Display.setCursor(150, 12 * lineHeight);
    M5.Display.setTextColor(TFT_WHITE);
    M5.Display.printf("%.1f m", ubxState.acc3d_m);
  } else {
    // データが無効な場合は「No UBX Data」表示
    M5.Display.fillRect(0, 3 * lineHeight, 320, 2 * lineHeight, TFT_BLACK);
    M5.Display.setCursor(0, 3 * lineHeight);
    M5.Display.setTextColor(TFT_RED);
    M5.Display.println("No UBX Data");
    M5.Display.setTextColor(TFT_WHITE);
    M5.Display.println("Waiting for UBX...");
  }
}

// GGA数値のみを更新する関数
void updateGGAValues() {
  if (!ggaDisplayInitialized) {
    initGGADisplay();
  }
  
  if (lastGGA.valid) {
    // 数値表示エリア全体を一括クリア（最も効率的）
    M5.Display.fillRect(45, 2 * lineHeight, 275, 8 * lineHeight, TFT_BLACK);
    
    // 時刻更新（3行目）
    M5.Display.setCursor(60, 2 * lineHeight);
    M5.Display.setTextColor(TFT_CYAN);
    M5.Display.printf("%s UTC", lastGGA.time.c_str());
    
    // 緯度更新（4行目）
    M5.Display.setCursor(50, 3 * lineHeight);
    M5.Display.setTextColor(TFT_GREEN);
    M5.Display.printf("%.6f %c", lastGGA.latitude, lastGGA.latDir);
    
    // 経度更新（5行目）
    M5.Display.setCursor(50, 4 * lineHeight);
    M5.Display.setTextColor(TFT_YELLOW);
    M5.Display.printf("%.6f %c", lastGGA.longitude, lastGGA.lonDir);
    
    // 衛星数更新（6行目）
    M5.Display.setCursor(60, 5 * lineHeight);
    M5.Display.setTextColor(TFT_MAGENTA);
    M5.Display.printf("%d", lastGGA.numSat);
    
    // 高度更新（7行目）
    M5.Display.setCursor(50, 6 * lineHeight);
    M5.Display.setTextColor(TFT_WHITE);
    M5.Display.printf("%.1f m", lastGGA.altitude);
    
    // HDOP更新（8行目）
    M5.Display.setCursor(60, 7 * lineHeight);
    M5.Display.setTextColor(TFT_WHITE);
    M5.Display.printf("%.1f", lastGGA.hdop);
    
    // Fix品質更新（9行目）
    M5.Display.setCursor(50, 8 * lineHeight);
    M5.Display.setTextColor(lastGGA.quality > 1 ? TFT_GREEN : TFT_RED);
    M5.Display.printf("%s", 
                     lastGGA.quality == 0 ? "No Fix" :
                     lastGGA.quality == 1 ? "GPS" :
                     lastGGA.quality == 2 ? "DGPS" : "RTK");
                     
    // GSA測位ステータス更新（10行目）
    M5.Display.setCursor(60, 9 * lineHeight);
    if (lastGSA.valid) {
      M5.Display.setTextColor(lastGSA.fixType == 3 ? TFT_GREEN : 
                             lastGSA.fixType == 2 ? TFT_YELLOW : TFT_RED);
      M5.Display.printf("%s", 
                       lastGSA.fixType == 1 ? "No Fix" :
                       lastGSA.fixType == 2 ? "2D Fix" : "3D Fix");
    } else {
      M5.Display.setTextColor(TFT_WHITE);
      M5.Display.printf("No GSA");
    }
  } else {
    // データが無効な場合は「No GPS Data」表示
    M5.Display.fillRect(0, 3 * lineHeight, 320, 2 * lineHeight, TFT_BLACK);  // エリアをクリア
    M5.Display.setCursor(0, 3 * lineHeight);
    M5.Display.setTextColor(TFT_RED);
    M5.Display.println("No GPS Data");
    M5.Display.setTextColor(TFT_WHITE);
    M5.Display.println("Waiting for GGA...");
  }
}

// UBXチェックサム計算
void calculateUBXChecksum(const std::vector<uint8_t>& data, uint8_t& ckA, uint8_t& ckB) {
  ckA = 0; ckB = 0;
  for (size_t i = 2; i < data.size() - 2; i++) { // SYNC1,SYNC2と最後の2バイト(チェックサム)を除外
    ckA += data[i];
    ckB += ckA;
  }
}

// UBXメッセージパーサー
MessageType parseUBXByte(uint8_t byte) {
  static int ubxIndex = 0;
  static uint16_t expectedLength = 0;
  
  switch (parserState) {
    case WAITING_SYNC:
      if (byte == 0xB5) {
        messageBuffer.clear();
        messageBuffer.push_back(byte);
        parserState = PARSING_UBX_HEADER;
        ubxIndex = 1;
      }
      break;
      
    case PARSING_UBX_HEADER:
      messageBuffer.push_back(byte);
      ubxIndex++;
      if (ubxIndex == 2 && byte != 0x62) {
        // SYNC2が正しくない
        parserState = WAITING_SYNC;
        return MSG_UNKNOWN;
      }
      if (ubxIndex == 6) {
        // LENGTH取得完了
        expectedLength = (messageBuffer[5] << 8) | messageBuffer[4];
        if (expectedLength == 0) {
          parserState = PARSING_UBX_CHECKSUM;
        } else {
          parserState = PARSING_UBX_PAYLOAD;
        }
        ubxIndex = 0;
      }
      break;
      
    case PARSING_UBX_PAYLOAD:
      messageBuffer.push_back(byte);
      ubxIndex++;
      if (ubxIndex >= expectedLength) {
        parserState = PARSING_UBX_CHECKSUM;
        ubxIndex = 0;
      }
      break;
      
    case PARSING_UBX_CHECKSUM:
      messageBuffer.push_back(byte);
      ubxIndex++;
      if (ubxIndex >= 2) {
        // UBXメッセージ完成
        currentUBX.sync1 = messageBuffer[0];
        currentUBX.sync2 = messageBuffer[1];
        currentUBX.msgClass = messageBuffer[2];
        currentUBX.msgId = messageBuffer[3];
        currentUBX.length = expectedLength;
        currentUBX.msgName = getUBXMessageName(currentUBX.msgClass, currentUBX.msgId);
        
        // チェックサム検証
        uint8_t ckA, ckB;
        calculateUBXChecksum(messageBuffer, ckA, ckB);
        currentUBX.checkA = messageBuffer[messageBuffer.size()-2];
        currentUBX.checkB = messageBuffer[messageBuffer.size()-1];
        currentUBX.valid = (ckA == currentUBX.checkA && ckB == currentUBX.checkB);
        
        parserState = WAITING_SYNC;
        return MSG_UBX;
      }
      break;
  }
  return MSG_UNKNOWN;
}

// NMEAメッセージパーサー
MessageType parseNMEAByte(uint8_t byte) {
  if (byte == '$') {
    nmeaBuffer = "$";
    parserState = PARSING_NMEA;
  } else if (parserState == PARSING_NMEA) {
    nmeaBuffer += (char)byte;
    if (byte == '\n') {
      // NMEAメッセージ完成
      currentNMEA.sentence = nmeaBuffer;
      currentNMEA.sentence.trim();
      
      // 基本的な妥当性チェック
      if (currentNMEA.sentence.startsWith("$") && currentNMEA.sentence.indexOf('*') > 0) {
        currentNMEA.valid = true;
        // タイプ抽出
        int commaPos = currentNMEA.sentence.indexOf(',');
        if (commaPos > 0) {
          String header = currentNMEA.sentence.substring(1, commaPos);
          if (header.length() >= 5) {
            currentNMEA.talker = header.substring(0, 2);
            currentNMEA.msgType = header.substring(2);
          }
        }
      } else {
        currentNMEA.valid = false;
      }
      
      parserState = WAITING_SYNC;
      return MSG_NMEA;
    }
  }
  return MSG_UNKNOWN;
}

// UBX NAV-PVTメッセージパーサー
void parseUBXNavPVT(const std::vector<uint8_t>& payload) {
  if (payload.size() < 92) {
    return;  // NAV-PVTは92バイト
  }
  
  // 時刻情報を取得 (offset 4-9)
  ubxState.year = (payload[5] << 8) | payload[4];      // year (2 bytes, offset 4-5)
  ubxState.month = payload[6];                         // month (1 byte, offset 6)
  ubxState.day = payload[7];                           // day (1 byte, offset 7)
  ubxState.hour = payload[8];                          // hour (1 byte, offset 8)
  ubxState.minute = payload[9];                        // minute (1 byte, offset 9)
  ubxState.second = payload[10];                       // second (1 byte, offset 10)
  
  // numSV (1 byte, offset 23)
  ubxState.svs_used = payload[23];
  
  // fixType (1 byte, offset 20)
  ubxState.fix_type = payload[20];
  
  // lon (4 bytes, offset 24) - 1e-7 degrees
  int32_t lon_raw = (payload[27] << 24) | (payload[26] << 16) | (payload[25] << 8) | payload[24];
  ubxState.lon_deg = lon_raw / 1e7;
  
  // lat (4 bytes, offset 28) - 1e-7 degrees
  int32_t lat_raw = (payload[31] << 24) | (payload[30] << 16) | (payload[29] << 8) | payload[28];
  ubxState.lat_deg = lat_raw / 1e7;
  
  // hMSL (4 bytes, offset 36) - mm
  int32_t hMSL_raw = (payload[39] << 24) | (payload[38] << 16) | (payload[37] << 8) | payload[36];
  ubxState.alt_m = hMSL_raw / 1000.0f;
  
  // hAcc (4 bytes, offset 40) - mm
  uint32_t hAcc_raw = (payload[43] << 24) | (payload[42] << 16) | (payload[41] << 8) | payload[40];
  ubxState.acc2d_m = hAcc_raw / 1000.0f;
  
  // vAcc (4 bytes, offset 44) - mm
  uint32_t vAcc_raw = (payload[47] << 24) | (payload[46] << 16) | (payload[45] << 8) | payload[44];
  float vAcc_m = vAcc_raw / 1000.0f;
  ubxState.acc3d_m = sqrtf(ubxState.acc2d_m * ubxState.acc2d_m + vAcc_m * vAcc_m);
  
  ubxState.valid = true;
}

// UBX NAV-SIGメッセージパーサー
void parseUBXNavSIG(const std::vector<uint8_t>& payload) {
  if (payload.size() < 8) return;  // 最小サイズチェック

  // numSigs (1 byte, offset 5)
  uint8_t numSigs = payload[5];
  ubxState.svs_tracked = numSigs;

  float sum_cno = 0;
  int valid_sigs = 0;

  // デバッグ: payload内容を16進で表示
  Serial.print("[DEBUG] NAV-SIG payload: ");
  for (size_t i = 0; i < payload.size(); ++i) {
    Serial.printf("%02X ", payload[i]);
  }
  Serial.println();
  Serial.printf("[DEBUG] numSigs: %d\n", numSigs);

  // 各信号のデータ構造（16バイト/信号）
  for (int i = 0; i < numSigs && (8 + i*16 + 15) < payload.size(); i++) {
    int offset = 8 + i * 16;
    // cno (1 byte, offset+6)
    uint8_t cno = payload[offset + 6];
    Serial.printf("[DEBUG] sig %d: cno=%d\n", i, cno);
    if (cno > 0) {
      sum_cno += cno;
      valid_sigs++;
    }
  }

  ubxState.cn0_avg_dbhz = (valid_sigs > 0) ? sum_cno / valid_sigs : 0;
  Serial.printf("[DEBUG] svs_tracked: %d, cn0_avg_dbhz: %.2f\n", ubxState.svs_tracked, ubxState.cn0_avg_dbhz);
}

// UBX NAV-SATメッセージパーサー（NAV-SIGの代替）
void parseUBXNavSAT(const std::vector<uint8_t>& payload) {
  if (payload.size() < 8) return;  // 最小サイズチェック
  
  // numSvs (1 byte, offset 5)
  uint8_t numSvs = payload[5];
  // NAV-SATではsvs_trackedとcn0_avg_dbhzは更新しない（NAV-SIGに任せる）
}

// UBX NAV-DOPメッセージパーサー
void parseUBXNavDOP(const std::vector<uint8_t>& payload) {
  if (payload.size() < 18) return;  // NAV-DOPは18バイト
  
  // pDOP (2 bytes, offset 10) - 0.01 units
  uint16_t pDOP_raw = (payload[11] << 8) | payload[10];
  ubxState.pdop = pDOP_raw / 100.0f;
  
  // hDOP (2 bytes, offset 14) - 0.01 units  
  uint16_t hDOP_raw = (payload[15] << 8) | payload[14];
  ubxState.hdop = hDOP_raw / 100.0f;
}

// メッセージパーサー（統合）
MessageType parseMessage(uint8_t byte) {
  // UBXチェック
  MessageType ubxResult = parseUBXByte(byte);
  if (ubxResult != MSG_UNKNOWN) return ubxResult;
  
  // NMEAチェック
  MessageType nmeaResult = parseNMEAByte(byte);
  return nmeaResult;
}

// NMEA度分秒を度に変換
double convertDMSToDegrees(double dms) {
  int degrees = (int)(dms / 100);
  double minutes = dms - (degrees * 100);
  return degrees + (minutes / 60.0);
}

// GGAメッセージパーサー
GGAData parseGGA(const String& sentence) {
  GGAData gga;
  gga.valid = false;
  gga.latitude = 0;
  gga.longitude = 0;
  gga.quality = 0;
  gga.numSat = 0;
  gga.hdop = 0;
  gga.altitude = 0;
  gga.latDir = 'N';
  gga.lonDir = 'E';
  
  if (!sentence.startsWith("$") || sentence.indexOf("GGA") == -1) {
    return gga;
  }
  
  // カンマで分割
  std::vector<String> fields;
  int start = 0;
  int end = 0;
  while ((end = sentence.indexOf(',', start)) != -1) {
    fields.push_back(sentence.substring(start, end));
    start = end + 1;
  }
  // 最後のフィールド（チェックサム含む）
  String lastField = sentence.substring(start);
  int asterisk = lastField.indexOf('*');
  if (asterisk != -1) {
    lastField = lastField.substring(0, asterisk);
  }
  fields.push_back(lastField);
  
  if (fields.size() < 15) return gga;  // GGAは最低15フィールド必要
  
  // フィールドをパース
  gga.time = fields[1];               // フィールド1: 時刻
  
  if (fields[2].length() > 0) {       // フィールド2: 緯度
    gga.latitude = convertDMSToDegrees(fields[2].toDouble());
    if (fields[3].length() > 0) {
      gga.latDir = fields[3].charAt(0); // フィールド3: 緯度方向
    }
  }
  
  if (fields[4].length() > 0) {       // フィールド4: 経度
    gga.longitude = convertDMSToDegrees(fields[4].toDouble());
    if (fields[5].length() > 0) {
      gga.lonDir = fields[5].charAt(0); // フィールド5: 経度方向
    }
  }
  
  gga.quality = fields[6].toInt();    // フィールド6: 品質
  gga.numSat = fields[7].toInt();     // フィールド7: 使用衛星数
  
  if (fields[8].length() > 0) {
    gga.hdop = fields[8].toDouble();  // フィールド8: HDOP
  }
  
  if (fields[9].length() > 0) {
    gga.altitude = fields[9].toDouble(); // フィールド9: 高度
  }
  
  gga.valid = (gga.quality > 0 && gga.numSat > 0);
  return gga;
}

// GSAメッセージパーサー
GSAData parseGSA(const String& sentence) {
  GSAData gsa;
  gsa.valid = false;
  gsa.fixType = 1;  // デフォルト：測位不可
  gsa.pdop = 0;
  gsa.hdop = 0;
  gsa.vdop = 0;
  
  if (!sentence.startsWith("$") || sentence.indexOf("GSA") == -1) {
    return gsa;
  }
  
  // カンマで分割
  std::vector<String> fields;
  int start = 0;
  int end = 0;
  while ((end = sentence.indexOf(',', start)) != -1) {
    fields.push_back(sentence.substring(start, end));
    start = end + 1;
  }
  // 最後のフィールド（チェックサム含む）
  String lastField = sentence.substring(start);
  int asterisk = lastField.indexOf('*');
  if (asterisk != -1) {
    lastField = lastField.substring(0, asterisk);
  }
  fields.push_back(lastField);
  
  if (fields.size() < 18) return gsa;  // GSAは最低18フィールド必要
  
  // フィールドをパース
  gsa.mode = fields[1];               // フィールド1: モード (M/A)
  gsa.fixType = fields[2].toInt();    // フィールド2: Fix Type (1/2/3)
  
  // PDOP, HDOP, VDOP (フィールド15, 16, 17)
  if (fields[15].length() > 0) {
    gsa.pdop = fields[15].toDouble();
  }
  if (fields[16].length() > 0) {
    gsa.hdop = fields[16].toDouble();
  }
  if (fields[17].length() > 0) {
    gsa.vdop = fields[17].toDouble();
  }
  
  gsa.valid = (gsa.fixType >= 1 && gsa.fixType <= 3);
  return gsa;
}

// CSVデータをパースする関数
ParsedData parseCSVLine(const String& line) {
  ParsedData data;
  data.valid = false;
  
  if (line.length() == 0) return data;
  
  int commaCount = 0;
  for (int i = 0; i < line.length(); i++) {
    if (line.charAt(i) == ',') commaCount++;
  }
  
  if (commaCount >= 2) { // 最低3つのフィールドが必要
    int start = 0;
    int end = line.indexOf(',', start);
    if (end != -1) {
      data.field1 = line.substring(start, end);
      start = end + 1;
      end = line.indexOf(',', start);
      if (end != -1) {
        data.field2 = line.substring(start, end);
        start = end + 1;
        data.field3 = line.substring(start);
        data.timestamp = String(millis());
        data.valid = true;
      }
    }
  }
  
  return data;
}

// 表示内容を更新する関数
void updateDisplay() {
  M5.Display.setCursor(0, 0);
  M5.Display.setTextColor(TFT_WHITE);
  
  switch (currentMode) {
    case MODE_UBX:
      if (!ubxDisplayInitialized) {
        initUBXDisplay();
      }
      break;
      
    case MODE_GGA:
      if (!ggaDisplayInitialized) {
        initGGADisplay();
      }
      break;
      
    case MODE_HEX:
      M5.Display.println("Mode: Hex Display");
      break;
      
    case MODE_UBX_RAW:
      M5.Display.println("Mode: UBX Messages");
      if (currentUBX.valid) {
        M5.Display.printf("Name: %s\n", currentUBX.msgName.c_str());
        M5.Display.printf("Class: 0x%02X, ID: 0x%02X\n", currentUBX.msgClass, currentUBX.msgId);
        M5.Display.printf("Length: %d bytes\n", currentUBX.length);
        M5.Display.printf("Checksum: %s\n", currentUBX.valid ? "OK" : "ERROR");
      }
      break;
      
    case MODE_NMEA:
      M5.Display.println("Mode: NMEA Messages");
      if (currentNMEA.valid) {
        M5.Display.printf("Talker: %s\n", currentNMEA.talker.c_str());
        M5.Display.printf("Type: %s\n", currentNMEA.msgType.c_str());
        M5.Display.printf("Sentence: %s\n", currentNMEA.sentence.c_str());
      }
      break;
      
    case MODE_PARSED:
      M5.Display.println("Mode: Parsed");
      if (lastParsedData.valid) {
        M5.Display.printf("F1: %s\n", lastParsedData.field1.c_str());
        M5.Display.printf("F2: %s\n", lastParsedData.field2.c_str());
        M5.Display.printf("F3: %s\n", lastParsedData.field3.c_str());
      }
      break;
      
    case MODE_FIELD1:
      M5.Display.println("Mode: Field 1");
      if (lastParsedData.valid) {
        M5.Display.setTextSize(3);
        M5.Display.printf("%s\n", lastParsedData.field1.c_str());
        M5.Display.setTextSize(textSize);
      }
      break;
      
    case MODE_FIELD2:
      M5.Display.println("Mode: Field 2");
      if (lastParsedData.valid) {
        M5.Display.setTextSize(3);
        M5.Display.printf("%s\n", lastParsedData.field2.c_str());
        M5.Display.setTextSize(textSize);
      }
      break;
      
    case MODE_FIELD3:
      M5.Display.println("Mode: Field 3");
      if (lastParsedData.valid) {
        M5.Display.setTextSize(3);
        M5.Display.printf("%s\n", lastParsedData.field3.c_str());
        M5.Display.setTextSize(textSize);
      }
      break;
  }
}

void loop() {
  // 定期的なボタンチェック（1ms毎）
  static unsigned long lastButtonCheck = 0;
  if (millis() - lastButtonCheck >= 1) {
    lastButtonCheck = millis();
  // ボタンチェック
  M5.update();
  if (M5.BtnA.wasPressed()) {
    currentMode = (DisplayMode)((currentMode + 1) % 9);  // 9モードに変更
    ggaDisplayInitialized = false;  // 表示モード変更時に初期化フラグをリセット
    ubxDisplayInitialized = false;  // UBX表示初期化フラグもリセット
    M5.Display.fillScreen(TFT_BLACK);
    updateDisplay();
  }
  
  if (M5.BtnB.wasPressed()) {
    M5.Display.fillScreen(TFT_RED);
    delay(200);
    M5.Display.fillScreen(TFT_BLACK);
    updateDisplay();
  }
  
  if (M5.BtnC.wasPressed()) {
    M5.Display.fillScreen(TFT_BLUE);
    delay(200);
    M5.Display.fillScreen(TFT_BLACK);
    updateDisplay();
  }
  } // ボタンチェック終了
  
  // 受信があれば画面とUSBシリアルに表示＆SD保存
  // ボタン応答性を保つため、一度に処理するバイト数を制限
  // NAV-PVT(92バイト)等の大きなUBXメッセージに対応するため100バイトに設定
  int processedBytes = 0;
  while (Serial2.available() && processedBytes < 100) {  // 最大100バイトずつ処理
    uint8_t c = Serial2.read();
    // Serial.write(c);            // PC側シリアルにも流す（デバッグのため一時停止）
    
    // 全データをバイナリバッファに保存（フィルタリングなし）
    rawBuffer.push_back(c);
    
    // メッセージパーサー統合処理
    MessageType msgType = parseMessage(c);
    
    // 完成したメッセージの処理
    if (msgType == MSG_UBX) {
      Serial.printf("UBX: %s (Class=0x%02X ID=0x%02X) %s\n", 
                   currentUBX.msgName.c_str(), currentUBX.msgClass, currentUBX.msgId,
                   currentUBX.valid ? "OK" : "ERR");
      
      // UBXメッセージのペイロードを取得
      if (currentUBX.valid && messageBuffer.size() >= 8) {
        std::vector<uint8_t> payload(messageBuffer.begin() + 6, messageBuffer.end() - 2);
        
        // メッセージタイプに応じてパース
        if (currentUBX.msgClass == 0x01) {  // NAV Class
          switch (currentUBX.msgId) {
            case 0x07:  // NAV-PVT
              Serial.printf("  -> Parsing NAV-PVT (%d bytes payload)...\n", payload.size());
              parseUBXNavPVT(payload);
              if (currentMode == MODE_UBX) {
                updateUBXValues();
              }
              break;
            case 0x43:  // NAV-SIG
              Serial.printf("  -> Parsing NAV-SIG (%d bytes payload)...\n", payload.size());
              parseUBXNavSIG(payload);
              if (currentMode == MODE_UBX) {
                updateUBXValues();
              }
              break;
            case 0x35:  // NAV-SAT (NAV-SIGの代替)
              Serial.printf("  -> Parsing NAV-SAT (%d bytes payload)...\n", payload.size());
              parseUBXNavSAT(payload);
              if (currentMode == MODE_UBX) {
                updateUBXValues();
              }
              break;
            case 0x04:  // NAV-DOP
              Serial.printf("  -> Parsing NAV-DOP (%d bytes payload)...\n", payload.size());
              parseUBXNavDOP(payload);
              if (currentMode == MODE_UBX) {
                updateUBXValues();
              }
              break;
            case 0x03:  // NAV-STATUS
              Serial.printf("  -> Parsing NAV-STATUS (%d bytes payload)...\n", payload.size());
              parseUBXNavSTATUS(payload);
              break;
          }
        }
      }
      
      // UBX_RAWモードの場合は生データ表示
      if (currentMode == MODE_UBX_RAW) {
        M5.Display.fillScreen(TFT_BLACK);
        updateDisplay();
        M5.Display.printf("UBX: Class=0x%02X ID=0x%02X Len=%d %s\n", 
                         currentUBX.msgClass, currentUBX.msgId, currentUBX.length,
                         currentUBX.valid ? "OK" : "ERR");
      }
    } else if (msgType == MSG_NMEA) {
      // GGAメッセージの特別処理
      if (currentNMEA.msgType == "GGA") {
        GGAData gga = parseGGA(currentNMEA.sentence);
        if (gga.valid) {
          lastGGA = gga;
          // GGAモードの場合は数値のみ更新
          if (currentMode == MODE_GGA) {
            updateGGAValues();
          }
        }
      }
      
      // GSAメッセージの特別処理
      if (currentNMEA.msgType == "GSA") {
        GSAData gsa = parseGSA(currentNMEA.sentence);
        if (gsa.valid) {
          lastGSA = gsa;
          // GGAモードの場合は数値のみ更新
          if (currentMode == MODE_GGA) {
            updateGGAValues();
          }
        }
      }
      
      if (currentMode == MODE_NMEA) {
        M5.Display.fillScreen(TFT_BLACK);
        updateDisplay();
        M5.Display.printf("NMEA: %s %s\n", currentNMEA.talker.c_str(), currentNMEA.msgType.c_str());
        M5.Display.printf("%s\n", currentNMEA.sentence.c_str());
      }
    }
    
    // テキスト処理（全文字を保持、改行で区切り）
    rxLine += (char)c;
    if (c == '\n') {
      // 改行でテキストライン完成
      String line = String(rxLine.c_str());
      line.trim(); // 改行文字等を削除
      
      // データをパース（印字可能文字のみの場合）
      ParsedData parsed = parseCSVLine(line);
      if (parsed.valid) {
        lastParsedData = parsed;
      }
      
      // 表示モードに応じて画面更新
      if (currentMode == MODE_HEX) {
        // 16進表示モード：最新の受信データを16進で表示
        int nextY = M5.Display.getCursorY() + lineHeight;
        if (nextY >= M5.Display.height()) {
          M5.Display.fillScreen(TFT_BLACK);
          M5.Display.setCursor(0, 0);
          updateDisplay();
        } else {
          M5.Display.setCursor(0, nextY);
        }
        // 受信したバイトを16進表示
        for (size_t i = 0; i < rxLine.length(); i++) {
          M5.Display.printf("%02X ", (uint8_t)rxLine[i]);
        }
        M5.Display.println();
      }
      rxLine.clear();
    } else if (currentMode == MODE_HEX) {
      // 16進表示モード：改行なしでもリアルタイム表示
      M5.Display.printf("%02X ", c);
      if (M5.Display.getCursorX() > M5.Display.width() - 30) {
        M5.Display.println();
        if (M5.Display.getCursorY() > M5.Display.height() - 20) {
          M5.Display.fillScreen(TFT_BLACK);
          M5.Display.setCursor(0, 0);
          updateDisplay();
        }
      }
    }
    
    // 処理したバイト数をインクリメント
    processedBytes++;
    
    // バイナリバッファが一定サイズになったら即座に保存（UBX対応）
    if (rawBuffer.size() >= 64) {  // UBXメッセージ対応のため小さなサイズで保存
      File binFile = SD.open(fileName, FILE_APPEND);
      if (binFile) {
        binFile.write(rawBuffer.data(), rawBuffer.size());
        binFile.close();
        rawBuffer.clear();
      }
    }
  }
  
  // ループ最後に残ったバッファデータを保存（UBXメッセージ取りこぼし防止）
  static unsigned long lastSave = 0;
  if (millis() - lastSave > 100 && rawBuffer.size() > 0) {  // 100ms毎にチェック
    File binFile = SD.open(fileName, FILE_APPEND);
    if (binFile) {
      binFile.write(rawBuffer.data(), rawBuffer.size());
      binFile.close();
      rawBuffer.clear();
      lastSave = millis();
    }
  }
}
