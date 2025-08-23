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
  
  Serial2.write(ubx_cfg_esf_meas, sizeof(ubx_cfg_esf_meas));
  delay(100);
  Serial2.write(ubx_cfg_esf_raw, sizeof(ubx_cfg_esf_raw));
  delay(100);
  Serial2.write(ubx_cfg_esf_status, sizeof(ubx_cfg_esf_status));
  delay(100);
  Serial2.write(ubx_cfg_nav_pvt, sizeof(ubx_cfg_nav_pvt));
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
  WiFi.begin("******", "******"); // SSIDとパスワードを設定
  M5.Display.println("WiFi connecting...");
  int wifiTimeout = 5; // 5秒でタイムアウト
  while (WiFi.status() != WL_CONNECTED && wifiTimeout > 0) {
    delay(1000);
    M5.Display.print(".");
    wifiTimeout--;
  }
  
  if (WiFi.status() == WL_CONNECTED) {
    M5.Display.println("\nWiFi Connected!");
    M5.Display.printf("IP: %s\n", WiFi.localIP().toString().c_str());
    
    // NTPで時刻同期
    M5.Display.println("NTP sync...");

    struct tm timeinfo;
    // configTzTime(NTP_TIMEZONE, NTP_SERVER1, NTP_SERVER2, NTP_SERVER3);
    configTime(0, 0, "pool.ntp.org", "time.nist.gov", "ntp.jst.mfeed.ad.jp");

    while (!getLocalTime(&timeinfo, 1000)) {
      Serial.print('.');
      M5.Display.print(".");
    }
    M5.Display.println("Time synced!");
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
  M5.Display.println("\n--- SD Card Init ---");
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
          dt.time.hours, dt.time.minutes, dt.time.seconds);
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
  M5.Display.printf("File: %04d%02d%02d_%02d%02d%02d.bin\n",
                    dt.date.year, dt.date.month, dt.date.date,
                    dt.time.hours, dt.time.minutes, dt.time.seconds);
  delay(5000);

  // UART2 を GPIO13(RX2), GPIO14(TX2) で開始
  Serial2.begin(115200, SERIAL_8N1, 13, 14);
  
  // u-bloxデバイスにUBXメッセージ有効化コマンドを送信
  M5.Display.println("Configuring u-blox...");
  delay(1000);
  sendUBXConfigurationCommands();
  delay(1000);
  
  M5.Display.setRotation(1);
  M5.Display.setFont(&fonts::Font0);
  M5.Display.setTextSize(textSize);
  M5.Display.fillScreen(TFT_BLACK);
  M5.Display.setCursor(0, 0);
  M5.Display.println("UART Viewer start...");
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
      case 0x3C: return "UBX-NAV-RELPOSNED";
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
  MODE_RAW = 0,      // 生データ表示
  MODE_HEX = 1,      // 16進表示
  MODE_UBX = 2,      // UBXメッセージ表示
  MODE_NMEA = 3,     // NMEAメッセージ表示
  MODE_PARSED = 4,   // パースデータ表示
  MODE_FIELD1 = 5,   // フィールド1のみ
  MODE_FIELD2 = 6,   // フィールド2のみ
  MODE_FIELD3 = 7    // フィールド3のみ
};

DisplayMode currentMode = MODE_RAW;
ParsedData lastParsedData;

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
        Serial.println("UBX: SYNC1 detected"); // デバッグ
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
        
        Serial.printf("UBX: Message complete - Class:0x%02X ID:0x%02X Len:%d Valid:%s\n", 
                     currentUBX.msgClass, currentUBX.msgId, currentUBX.length, 
                     currentUBX.valid ? "OK" : "ERR"); // デバッグ
        
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

// メッセージパーサー（統合）
MessageType parseMessage(uint8_t byte) {
  // UBXチェック
  MessageType ubxResult = parseUBXByte(byte);
  if (ubxResult != MSG_UNKNOWN) return ubxResult;
  
  // NMEAチェック
  MessageType nmeaResult = parseNMEAByte(byte);
  return nmeaResult;
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
    case MODE_RAW:
      M5.Display.println("Mode: Raw Data");
      break;
    case MODE_HEX:
      M5.Display.println("Mode: Hex Display");
      break;
    case MODE_UBX:
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
  // ボタンチェック
  M5.update();
  if (M5.BtnA.wasPressed()) {
    currentMode = (DisplayMode)((currentMode + 1) % 8);
    M5.Display.fillScreen(TFT_BLACK);
    updateDisplay();
  }
  
  // 受信があれば画面とUSBシリアルに表示＆SD保存
  while (Serial2.available()) {
    uint8_t c = Serial2.read();
    Serial.write(c);            // PC側シリアルにも流す
    
    // 全データをバイナリバッファに保存（フィルタリングなし）
    rawBuffer.push_back(c);
    
    // メッセージパーサー統合処理
    MessageType msgType = parseMessage(c);
    
    // 完成したメッセージの処理
    if (msgType == MSG_UBX) {
      if (currentMode == MODE_RAW) {
        // RAWモードでUBXメッセージ名とサイズを表示
        int nextY = M5.Display.getCursorY() + lineHeight;
        if (nextY >= M5.Display.height()) {
          M5.Display.fillScreen(TFT_BLACK);
          M5.Display.setCursor(0, 0);
          updateDisplay();
        } else {
          M5.Display.setCursor(0, nextY);
        }
        M5.Display.setTextColor(TFT_CYAN);  // UBXメッセージは水色で表示
        M5.Display.printf("%s Size: %d\n", currentUBX.msgName.c_str(), currentUBX.length + 8);
        M5.Display.setTextColor(TFT_WHITE); // 色をリセット
      } else if (currentMode == MODE_UBX) {
        M5.Display.fillScreen(TFT_BLACK);
        updateDisplay();
        M5.Display.printf("UBX: Class=0x%02X ID=0x%02X Len=%d %s\n", 
                         currentUBX.msgClass, currentUBX.msgId, currentUBX.length,
                         currentUBX.valid ? "OK" : "ERR");
      }
    } else if (msgType == MSG_NMEA) {
      if (currentMode == MODE_RAW) {
        // RAWモードでNMEAメッセージを表示
        int nextY = M5.Display.getCursorY() + lineHeight;
        if (nextY >= M5.Display.height()) {
          M5.Display.fillScreen(TFT_BLACK);
          M5.Display.setCursor(0, 0);
          updateDisplay();
        } else {
          M5.Display.setCursor(0, nextY);
        }
        M5.Display.setTextColor(TFT_GREEN);  // NMEAメッセージは緑色で表示
        M5.Display.printf("NMEA: %s %s\n", currentNMEA.talker.c_str(), currentNMEA.msgType.c_str());
        M5.Display.setTextColor(TFT_WHITE);  // 色をリセット
      } else if (currentMode == MODE_NMEA) {
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
      } else {
        // パースモード：特定データのみ表示
        M5.Display.fillScreen(TFT_BLACK);
        updateDisplay();
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
