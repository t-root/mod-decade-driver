#include <Arduino.h>
#include "HardwareSerial.h"
#include "DFRobotDFPlayerMini.h"
#include <ArduinoJson.h>
#include <WiFi.h>
#include <WebServer.h>
#include <Preferences.h>
#include <LittleFS.h>
#include <SPIFFS.h>

// Disable brownout detector to prevent resets
#include "soc/soc.h"
#include "soc/rtc_cntl_reg.h"

//////////////////////////////////////////////////////
// DFPLAYER (ESP32-C3: dùng UART1, ví dụ TX=GPIO21, RX=GPIO20)
//////////////////////////////////////////////////////
HardwareSerial mySerial(1);
DFRobotDFPlayerMini player;

// DynamicJsonDocument size: tăng để parse sound.json lớn
// Nếu bạn dùng quá nhiều card và gặp lỗi "NoMemory", tăng thêm giá trị này.
static DynamicJsonDocument soundConfig(150000);

// sound.json mặc định (dùng khi lần đầu nạp code, chưa có dữ liệu trong Preferences)
const char defaultSoundJson[] PROGMEM = R"rawliteral(
{
  "basic": {
    "boot_sound": "011.mp3",
    "kamen_ride": "006.mp3",
    "attack_ride": "007.mp3",
    "final_attack_ride": "008.mp3",
    "final_form_ride": "009.mp3",
    "touch": "010.mp3",
    "in_card": "001.mp3",
    "out_card": "002.mp3",
    "open": "003.mp3",
    "close": "004.mp3",
    "error": "005.mp3"
  },
  "bmg": {
    "decade": "001.mp3"
  },
  "voice": {
    "decade": ["001.mp3", "002.mp3", "003.mp3", "004.mp3"]
  },
  "card": {
    "01101110101": {
      "name": "Decade",
      "type": "kamen_ride",
      "voice": "decade",
      "bmg": "001.mp3",
      "file": "0010.mp3"
    },
    "11100111111": {
      "name": "w",
      "type": "kamen_ride",
      "voice": "decade",
      "bmg": "001.mp3",
      "file": "0011.mp3"
    },
    "11101000011": {
      "name": "OOO",
      "type": "kamen_ride",
      "voice": "decade",
      "bmg": "001.mp3",
      "file": "0012.mp3"
    },
    "11101001101": {
      "name": "Fourze",
      "type": "kamen_ride",
      "voice": "decade",
      "bmg": "001.mp3",
      "file": "0013.mp3"
    },
    "11101011111": {
      "name": "Wizard",
      "type": "kamen_ride",
      "voice": "decade",
      "bmg": "001.mp3",
      "file": "0014.mp3"
    },
    "11110010101": {
      "name": "Gami",
      "type": "kamen_ride",
      "voice": "decade",
      "bmg": "001.mp3",
      "file": "0015.mp3"
    },
    "11110110111": {
      "name": "Drive",
      "type": "kamen_ride",
      "voice": "decade",
      "bmg": "001.mp3",
      "file": "0016.mp3"
    },
    "00000100101": {
      "name": "Gost",
      "type": "kamen_ride",
      "voice": "decade",
      "bmg": "001.mp3",
      "file": "0017.mp3"
    },
    "00011000111": {
      "name": "Ex-Aid",
      "type": "kamen_ride",
      "voice": "decade",
      "bmg": "001.mp3",
      "file": "0018.mp3"
    },
    "00111011101": {
      "name": "Build",
      "type": "kamen_ride",
      "voice": "decade",
      "bmg": "001.mp3",
      "file": "0019.mp3"
    },
    "00111101101": {
      "name": "Zi-O",
      "type": "kamen_ride",
      "voice": "decade",
      "bmg": "001.mp3",
      "file": "0020.mp3"
    },
    "10111110111": {
      "name": "Blade",
      "type": "final_form_ride",
      "voice": "decade",
      "bmg": "001.mp3",
      "file": "0031.mp3"
    },
    "00000110111": {
      "name": "Den-O",
      "type": "final_form_ride",
      "voice": "decade",
      "bmg": "001.mp3",
      "file": "0034.mp3"
    },
    "11101010101": {
      "name": "Decade",
      "type": "final_form_ride",
      "voice": "decade",
      "bmg": "001.mp3",
      "file": "0036.mp3"
    },
    "10011101101": {
      "name": "w",
      "type": "final_form_ride",
      "voice": "decade",
      "bmg": "001.mp3",
      "file": "0037.mp3"
    },
    "10100000111": {
      "name": "OOO",
      "type": "final_form_ride",
      "voice": "decade",
      "bmg": "001.mp3",
      "file": "0038.mp3"
    },
    "10100110011": {
      "name": "Fourze",
      "type": "final_form_ride",
      "voice": "decade",
      "bmg": "001.mp3",
      "file": "0039.mp3"
    },
    "10110010011": {
      "name": "Wizard",
      "type": "final_form_ride",
      "voice": "decade",
      "bmg": "001.mp3",
      "file": "0040.mp3"
    },
    "11000011011": {
      "name": "Gaim",
      "type": "final_form_ride",
      "voice": "decade",
      "bmg": "001.mp3",
      "file": "0041.mp3"
    },
    "11010000001": {
      "name": "drive",
      "type": "final_form_ride",
      "voice": "decade",
      "bmg": "001.mp3",
      "file": "0042.mp3"
    },
    "11011011111": {
      "name": "Gost",
      "type": "final_form_ride",
      "voice": "decade",
      "bmg": "001.mp3",
      "file": "0043.mp3"
    },
    "11011110111": {
      "name": "Ex-Aid",
      "type": "final_form_ride",
      "voice": "decade",
      "bmg": "001.mp3",
      "file": "0044.mp3"
    },
    "11100010011": {
      "name": "Build",
      "type": "final_form_ride",
      "voice": "decade",
      "bmg": "001.mp3",
      "file": "0045.mp3"
    },
    "00001111011": {
      "name": "Zi-O",
      "type": "final_form_ride",
      "voice": "decade",
      "bmg": "001.mp3",
      "file": "0046.mp3"
    },
    "10000010111": {
      "name": "BOKUNI TSURRRETE MIRU",
      "type": "attack_ride",
      "voice": "decade",
      "bmg": "001.mp3",
      "file": "0053.mp3"
    },
    "10001000111": {
      "name": "TSUPPARI",
      "type": "final_attack_ride",
      "voice": "decade",
      "bmg": "001.mp3",
      "file": "0054.mp4"
    },
    "00101110011": {
      "name": "HELSEL RIDER",
      "type": "attack_ride",
      "voice": "decade",
      "bmg": "001.mp3",
      "file": "0055.mp3"
    },
    "01101000001": {
      "name": "SLASH",
      "type": "attack_ride",
      "voice": "decade",
      "bmg": "001.mp3",
      "file": "0056.mp3"
    },
    "01100110011": {
      "name": "ILLUSION",
      "type": "attack_ride",
      "voice": "decade",
      "bmg": "001.mp3",
      "file": "0057.mp3"
    },
    "01101010011": {
      "name": "BLAST",
      "type": "attack_ride",
      "voice": "decade",
      "bmg": "001.mp3",
      "file": "0058.mp3"
    },
    "01101100011": {
      "name": "INVISIBLE",
      "type": "attack_ride",
      "voice": "decade",
      "bmg": "001.mp3",
      "file": "0059.mp3"
    },
    "00000010101": {
      "name": "REKKA DAIZANTOU",
      "type": "attack_ride",
      "voice": "decade",
      "bmg": "001.mp3",
      "file": "0060.mp3"
    },
    "00000001101": {
      "name": "Decade complete From",
      "type": "kamen_ride",
      "voice": "decade",
      "bmg": "001.mp3",
      "file": "0061.mp3"
    },
    "01001010001": {
      "name": "W Cyclone Joke Xtreme",
      "type": "kamen_ride",
      "voice": "decade",
      "bmg": "001.mp3",
      "file": "0062.mp3"
    },
    "00010001110": {
      "name": "GRANDZI-O",
      "type": "kamen_ride",
      "voice": "decade",
      "bmg": "001.mp3",
      "file": "0063.mp3"
    }
  }
}
)rawliteral";

//////////////////////////////////////////////////////
// PINS - ESP32-C3 Super Mini mapping
// Chú ý: bạn cần đấu dây theo đúng chân mới này:
//  - S1_PIN  -> GPIO2
//  - S2_PIN  -> GPIO3
//  - L1   -> GPIO4
//  - L2   -> GPIO5
//  - TOUCH   -> GPIO6 (dùng như digital input)
//  - LED_R   -> GPIO7
//  - LED_G   -> GPIO8
//  - LED_B   -> GPIO9
//  - DFPlayer TX -> GPIO20 (RX của ESP32-C3)
//  - DFPlayer RX -> GPIO21 (TX của ESP32-C3)
//////////////////////////////////////////////////////
#define S1_PIN 1
#define S2_PIN 3
#define L1 4
#define L2 5
#define TOUCH_PIN 6

//////////////////////////////////////////////////////
// LED (ESP32-C3 GPIO)
//////////////////////////////////////////////////////
#define LED_R 7
#define LED_G 8
#define LED_B 9

#define RED_CH 0
#define GREEN_CH 1
#define BLUE_CH 2

const uint8_t colors[][3] = {
  {255,0,0},{0,255,0},{0,0,255},
  {255,255,0},{255,0,255},{0,255,255}
};

// Led mode: COLOR = use single applied color for all effects
//           RGB   = automatic cycling through palette
enum LedMode { MODE_COLOR = 0, MODE_RGB = 1 };

LedMode ledMode = MODE_RGB; // default
uint8_t curR = 255, curG = 0, curB = 0; // current applied color (used when MODE_COLOR)

bool ledOn=false;
int colorIdx=0;
unsigned long lastColor=0;
bool typePlayingPulse=false;  // Flag: đang phát type (LED pulse)
unsigned long typePulseStart=0;  // Thời điểm bắt đầu pulse
bool ledFadingOut=false;  // Flag: LED đang fade out
unsigned long ledFadeStart=0;  // Thời điểm bắt đầu fade out
bool mp3Playing=false;  // Flag: đang phát MP3 file
unsigned long mp3StartTime=0;  // Thời điểm bắt đầu phát MP3
// LED random cho MP3/BMG: luôn dao động sáng/mờ trong suốt thời gian phát
bool longAudioLedActive=false;           // Đang chạy hiệu ứng random cho MP3/BMG
unsigned long longAudioLastChange=0;     // Lần đổi độ sáng gần nhất
unsigned long longAudioSegmentDuration=0;// Thời gian giữ 1 trạng thái sáng (1-3s)
int longAudioBrightness=255;             // Độ sáng hiện tại (10-255)
// LED preset cycling bằng touch (khi không giữ L2)
int ledPresetIndex = 0;
const int LED_PRESET_COUNT = 8;          // red, green, blue, yellow, magenta, cyan, white, rgb
bool touchInitialized = false;           // tránh trigger touch lần đầu khi mới boot

//////////////////////////////////////////////////////
// STATE
//////////////////////////////////////////////////////
bool capturing=false;
String bits="";
int countBit=0;
int prev_s1=-1;

int savedFileNumber=-1;

bool btn17Held=false;
bool closePlaying=false;
bool openPlaying=false;  // Flag: đang phát "open"
bool mp3PlayingFlag=false;  // Flag: đang phát MP3 file (cho event handler)

// Touch and voice state
bool voicePlaying=false;    // Flag: đang phát voice
int currentVoiceIndex=0;    // Index hiện tại trong voice array
String currentVoiceType=""; // Voice type của card hiện tại (decade, kiva, blade)
bool touchLastState=HIGH;   // Trạng thái touch trước đó (HIGH = không chạm)
unsigned long lastTouchChange=0; // Time of last touch change
unsigned long lastVoicePlay=0;    // Time of last voice play (to prevent spam)
unsigned long touchHoldStart=0;   // Time when touch started being held
bool bmgPlaying=false;      // Flag: đang phát BMG file
bool longPressTriggered=false; // Flag to prevent multiple long press triggers
String currentCardId="";    // ID của card hiện tại

// State machine để đợi "in_card" phát xong
enum PlayState { IDLE, WAITING_FOR_IN_CARD, HANDLING_CARD };
PlayState playState = IDLE;
String pendingCardId = "";  // Lưu card ID chờ "in_card" phát xong
bool inCardFinished = false;  // Flag: "in_card" đã phát xong chưa

//////////////////////////////////////////////////////
// CARD LOG (serial + web)
//////////////////////////////////////////////////////
const int CARD_LOG_SIZE = 32;     // lưu tối đa 32 lần quét gần nhất
String cardLogBits[CARD_LOG_SIZE];
String cardLogId[CARD_LOG_SIZE];
unsigned long cardLogTime[CARD_LOG_SIZE];
int cardLogIndex = 0;
int cardLogCount = 0;

void addCardLog(const String &rawBits, const String &cardId){
  cardLogBits[cardLogIndex] = rawBits;
  cardLogId[cardLogIndex]   = cardId;
  cardLogTime[cardLogIndex] = millis();

  cardLogIndex = (cardLogIndex + 1) % CARD_LOG_SIZE;
  if(cardLogCount < CARD_LOG_SIZE) cardLogCount++;
}

void clearCardLog(){
  cardLogIndex = 0;
  cardLogCount = 0;
  for(int i = 0; i < CARD_LOG_SIZE; i++){
    cardLogBits[i] = "";
    cardLogId[i]   = "";
    cardLogTime[i] = 0;
  }
}

//////////////////////////////////////////////////////
// WEB / WIFI / PERSIST
//////////////////////////////////////////////////////
const char* AP_SSID = "Driver Decade";
const char* AP_PASS = "decade123";

WebServer server(80);
Preferences pref;
static String gNote = "Nhận mod đồ chơi liên hệ 0123456789 không nghe máy là do đang ỉa. By Bảo Đang ỈaỈa";

// Preferences schema/build key:
// Whenever this value changes, the firmware will clear the entire Preferences namespace once.
// Bump this when you want "fresh" settings after flashing new code.
static const uint32_t PREF_SCHEMA_VERSION = 20260325; // YYYYMMDD
static const char* PREF_BUILD_ID = __DATE__ " " __TIME__;
enum FSBackend { FS_NONE = 0, FS_LITTLEFS = 1, FS_SPIFFS = 2 };
static FSBackend fsBackend = FS_NONE;

static bool ensureSoundFsReady(){
  if(fsBackend != FS_NONE) return true;
  Serial.println("FS: trying LittleFS...");
  if(LittleFS.begin()){
    fsBackend = FS_LITTLEFS;
    Serial.println("FS: LittleFS ready");
    return true;
  }
  Serial.println("FS: LittleFS failed, trying SPIFFS...");
  if(SPIFFS.begin(true)){
    fsBackend = FS_SPIFFS;
    Serial.println("FS: SPIFFS ready");
    return true;
  }
  Serial.println("FS: LittleFS+SPIFFS both failed");
  return false;
}

static void handleNote(){
  // GET /note            -> returns current note (text/plain)
  // GET /note?note=...   -> set note, returns JSON
  // Also supported via /note=... (handled in handleNotFound)
  if(server.hasArg("note")){
    String v = server.arg("note");
    v.trim();
    // Allow disabling popup
    if(v.length() == 0) v = "no";
    gNote = v;
    pref.putString("note", gNote);
    server.send(200, "application/json", String("{\"success\":true,\"note\":") + "\"" + gNote + "\"}");
    return;
  }
  server.send(200, "text/plain", gNote);
}

// Forward decl for /note=... legacy path
static String urlDecode(const String& in);

static void handleNotFound(){
  // Support legacy style: /note=NEW_NOTE
  String uri = server.uri();
  if(uri.startsWith("/note=")){
    String v = uri.substring(String("/note=").length());
    v.replace("+", " ");
    v = urlDecode(v);
    v.trim();
    if(v.length() == 0) v = "no";
    gNote = v;
    pref.putString("note", gNote);
    server.send(200, "application/json", String("{\"success\":true,\"note\":") + "\"" + gNote + "\"}");
    return;
  }
  server.send(404, "text/plain", "Not found");
}

// Minimal URL decoder for /note=... path variant
static String urlDecode(const String& in){
  String out;
  out.reserve(in.length());
  for(size_t i = 0; i < in.length(); i++){
    char c = in[i];
    if(c == '%' && i + 2 < in.length()){
      char h1 = in[i+1];
      char h2 = in[i+2];
      auto hexVal = [](char h)->int{
        if(h >= '0' && h <= '9') return h - '0';
        if(h >= 'a' && h <= 'f') return 10 + (h - 'a');
        if(h >= 'A' && h <= 'F') return 10 + (h - 'A');
        return -1;
      };
      int v1 = hexVal(h1);
      int v2 = hexVal(h2);
      if(v1 >= 0 && v2 >= 0){
        out += char((v1 << 4) | v2);
        i += 2;
        continue;
      }
    }
    out += c;
  }
  return out;
}

unsigned long apStartMillis = 0;
const unsigned long AP_TIMEOUT = 30000; // 30s
bool apActive = false; // true while AP is running and hasn't been auto-stopped

//////////////////////////////////////////////////////
// LED helpers
//////////////////////////////////////////////////////
void setupLED(){
  ledcSetup(RED_CH,5000,8);
  ledcSetup(GREEN_CH,5000,8);
  ledcSetup(BLUE_CH,5000,8);

  ledcAttachPin(LED_R,RED_CH);
  ledcAttachPin(LED_G,GREEN_CH);
  ledcAttachPin(LED_B,BLUE_CH);
}

void setColor(int r,int g,int b){
  // clamp 0-255 just in case
  r = constrain(r, 0, 255);
  g = constrain(g, 0, 255);
  b = constrain(b, 0, 255);
  ledcWrite(RED_CH, r);
  ledcWrite(GREEN_CH, g);
  ledcWrite(BLUE_CH, b);
}

// Forward declarations for preset helpers
void applyColorAndSave(uint8_t r, uint8_t g, uint8_t b);
void setModeRGBAndSave();

// Áp dụng preset LED theo index (dùng cùng mapping với /preset trên web)
// 0: red, 1: green, 2: blue, 3: yellow, 4: magenta, 5: cyan, 6: white, 7: rgb(auto)
void applyLedPresetByIndex(int idx){
  idx = (idx % LED_PRESET_COUNT + LED_PRESET_COUNT) % LED_PRESET_COUNT;
  switch(idx){
    case 0: // red
      applyColorAndSave(255,0,0);
      break;
    case 1: // green
      applyColorAndSave(0,255,0);
      break;
    case 2: // blue
      applyColorAndSave(0,0,255);
      break;
    case 3: // yellow
      applyColorAndSave(255,255,0);
      break;
    case 4: // magenta
      applyColorAndSave(255,0,255);
      break;
    case 5: // cyan
      applyColorAndSave(0,255,255);
      break;
    case 6: // white
      applyColorAndSave(255,255,255);
      break;
    case 7: // rgb mode
    default:
      setModeRGBAndSave();
      break;
  }
  ledPresetIndex = idx;
}

// Apply a single RGB color and persist as MODE_COLOR
void applyColorAndSave(uint8_t r, uint8_t g, uint8_t b){
  curR = r; curG = g; curB = b;
  ledMode = MODE_COLOR;
  pref.putUShort("led_mode", (uint16_t)ledMode);
  pref.putUShort("led_r", (uint16_t)curR);
  pref.putUShort("led_g", (uint16_t)curG);
  pref.putUShort("led_b", (uint16_t)curB);
  // make sure LED visible with new color
  ledOn = true;
  setColor(curR, curG, curB);
}

// Set RGB automatic cycling mode and persist
void setModeRGBAndSave(){
  ledMode = MODE_RGB;
  pref.putUShort("led_mode", (uint16_t)ledMode);
  // make sure LED visible in cycle
  ledOn = true;
  colorIdx = 0;
  lastColor = millis();
}

void ledStart(){
  ledOn=true;
  colorIdx=0;
  lastColor=millis();
}

void ledStop(){
  ledOn=false;
  setColor(0,0,0);
}

void updateLED(){
  if(!ledOn) return;

  // If mode is RGB and not playing MP3, cycle through palette
  if(ledMode == MODE_RGB){
    if(millis()-lastColor>400){
      colorIdx=(colorIdx+1)%6;
      if(!mp3Playing){
        setColor(colors[colorIdx][0],colors[colorIdx][1],colors[colorIdx][2]);
      }
      lastColor=millis();
    }
  } else {
    // MODE_COLOR: keep applied color during normal cycling state
    if(!mp3Playing){
      setColor(curR, curG, curB);
    }
  }
}

void updateTypePulse(){
  if(!typePlayingPulse) return;
  
  unsigned long elapsed = millis() - typePulseStart;
  int brightness;
  
  int cyclePeriod = 2000;  // 2s
  int pos = elapsed % cyclePeriod;
  
  if(pos < 1000){
    brightness = (pos * 255) / 1000;
  } else {
    brightness = ((2000 - pos) * 255) / 1000;
  }
  
  uint8_t r,g,b;
  if(ledMode == MODE_RGB){
    r = (colors[colorIdx][0] * brightness) / 255;
    g = (colors[colorIdx][1] * brightness) / 255;
    b = (colors[colorIdx][2] * brightness) / 255;
  } else {
    r = (curR * brightness) / 255;
    g = (curG * brightness) / 255;
    b = (curB * brightness) / 255;
  }
  setColor(r,g,b);
}

void updateLedFadeOut(){
  if(!ledFadingOut) return;

  unsigned long elapsed = millis() - ledFadeStart;
  // Thời gian fade out LED (giảm xuống cho ngắn hơn, tránh kéo dài ảnh hưởng cảm giác đọc thẻ)
  int fadeDuration = 500;  // 400ms

  if(elapsed >= fadeDuration){
    setColor(0, 0, 0);
    ledFadingOut=false;
    ledOn=false;
    return;
  }

  int brightness = (255 * (fadeDuration - elapsed)) / fadeDuration;

  uint8_t r,g,b;
  if(ledMode == MODE_RGB){
    r = (colors[colorIdx][0] * brightness) / 255;
    g = (colors[colorIdx][1] * brightness) / 255;
    b = (colors[colorIdx][2] * brightness) / 255;
  } else {
    r = (curR * brightness) / 255;
    g = (curG * brightness) / 255;
    b = (curB * brightness) / 255;
  }
  setColor(r,g,b);
}

void updateMp3Led(){
  // Áp dụng cho cả MP3 và BMG: nếu không có cái nào đang phát thì thôi
  if(!mp3Playing && !bmgPlaying) return;

  unsigned long now = millis();

  // Nếu mới bắt đầu phát: khởi tạo hiệu ứng
  if(!longAudioLedActive){
    longAudioLedActive = true;
    longAudioLastChange = now;
    // Đổi trạng thái trong khoảng <= 1s (0.2s - 1s)
    longAudioSegmentDuration = random(200, 1001);
    longAudioBrightness = random(10, 256);         // 10-255
  }

  // Khi hết 1 segment thì random trạng thái mới (vẫn trong 0.2s - 1s)
  if(now - longAudioLastChange >= longAudioSegmentDuration){
    longAudioBrightness = random(10, 256);
    longAudioSegmentDuration = random(200, 1001);
    longAudioLastChange = now;
  }

  // Apply brightness cho màu hiện tại
  uint8_t r,g,b;
  if(ledMode == MODE_RGB){
    r = (colors[colorIdx][0] * longAudioBrightness) / 255;
    g = (colors[colorIdx][1] * longAudioBrightness) / 255;
    b = (colors[colorIdx][2] * longAudioBrightness) / 255;
  } else {
    r = (curR * longAudioBrightness) / 255;
    g = (curG * longAudioBrightness) / 255;
    b = (curB * longAudioBrightness) / 255;
  }
  setColor(r,g,b);
}

//////////////////////////////////////////////////////
// HELPERS
//////////////////////////////////////////////////////
int fileToNum(const char* f){
  return atoi(f);
}

//////////////////////////////////////////////////////
// PLAY BASE (folder 01)
//////////////////////////////////////////////////////
void playBase(const char* key){
  const char* file = nullptr;
  if(soundConfig.containsKey("basic") && soundConfig["basic"].containsKey(key)){
    file = soundConfig["basic"][key];
  }
  if(!file) return;

  int n=fileToNum(file);

  Serial.printf("playBase: %s -> %d\n",key,n);

  player.playFolder(1,n);
}

//////////////////////////////////////////////////////
// PLAY ROOT FILE -> USE /MP3 via playMp3Folder()
//////////////////////////////////////////////////////
void playCardRoot(int num){
  // Switch to MP3 folder playback to use numeric filenames reliably:
  // /MP3/0001.mp3  -> playMp3Folder(1)
  Serial.printf("Play MP3/%04d.mp3\n", num);

  // play from /MP3 folder (stable mapping by number)
  player.playMp3Folder(num);

  // Start MP3 playback LED control
  // Bật LED rõ ràng ngay khi bắt đầu phát MP3:
  // - MODE_COLOR: dùng đúng màu đang set
  // - MODE_RGB: chọn ngẫu nhiên 1 màu trong bảng colors
  ledFadingOut = false;   // huỷ fade nếu đang chạy
  ledOn = true;
  if(ledMode == MODE_COLOR){
    setColor(curR, curG, curB);
  } else {
    colorIdx = random(0, 6);
    setColor(colors[colorIdx][0], colors[colorIdx][1], colors[colorIdx][2]);
  }

  mp3Playing = true;
  mp3PlayingFlag = true;
  mp3StartTime = millis();
}

//////////////////////////////////////////////////////
// PLAY VOICE FILE from /02/ folder
//////////////////////////////////////////////////////
void playVoiceFile(){
  Serial.printf("PLAY_VOICE: Entered function, voiceType='%s'\n", currentVoiceType.c_str());

  if(currentVoiceType.length() == 0){
    Serial.println("PLAY_VOICE: No voice type set");
    return;
  }

  // Check if voice type exists in config
  if(!soundConfig.containsKey("voice") || !soundConfig["voice"].containsKey(currentVoiceType)){
    Serial.printf("PLAY_VOICE: Voice type '%s' not found in config\n", currentVoiceType.c_str());
    return;
  }

  JsonArray voiceArray = soundConfig["voice"][currentVoiceType];
  int arraySize = voiceArray.size();

  Serial.printf("PLAY_VOICE: Voice array size=%d, currentIndex=%d\n", arraySize, currentVoiceIndex);

  if(arraySize == 0){
    Serial.println("PLAY_VOICE: Voice array is empty");
    return;
  }

  // Get current voice file
  const char* voiceFile = voiceArray[currentVoiceIndex];

  Serial.printf("PLAY_VOICE: Playing file '%s' at index %d\n", voiceFile, currentVoiceIndex);

  // Try to play from /02/ folder first (voice files)
  int fileNum = fileToNum(voiceFile);
  Serial.printf("PLAY_VOICE: Attempting to play voice from /02/%03d.mp3 (folder 2, file %d)\n", fileNum, fileNum);

  // Play from folder 02 - DFPlayer will handle if file doesn't exist
  player.playFolder(2, fileNum);

  voicePlaying = true;

  // Move to next index, loop back to 0 when finished
  currentVoiceIndex = (currentVoiceIndex + 1) % arraySize;
  Serial.printf("PLAY_VOICE: Next index will be %d\n", currentVoiceIndex);
}

//////////////////////////////////////////////////////
// PLAY BMG FILE from /03/ folder (long press)
//////////////////////////////////////////////////////
void playBmgFile(){
  if(currentCardId.length() == 0) return;

  if(!soundConfig.containsKey("card") || !soundConfig["card"].containsKey(currentCardId)){
    return;
  }

  const char* bmgName = soundConfig["card"][currentCardId]["bmg"];
  if(!bmgName){
    return;
  }

  // Look up the actual file from bmg section
  const char* bmgFile = nullptr;
  if(soundConfig.containsKey("bmg") && soundConfig["bmg"].containsKey(bmgName)){
    bmgFile = soundConfig["bmg"][bmgName];
  }

  if(!bmgFile){
    return;
  }

  int fileNum = fileToNum(bmgFile);
  player.playFolder(3, fileNum);
  bmgPlaying = true;
}

//////////////////////////////////////////////////////
// RANDOM TYPE RESOLUTION
//////////////////////////////////////////////////////
static bool typeHasRandomPrefix(const String &t){
  // Arduino String::startsWith may not be available on every core, so do it manually.
  const String prefix = "random_";
  if(t.length() < prefix.length()) return false;
  return t.substring(0, prefix.length()) == prefix;
}

static bool cardTypeIsRandom(const String &t){
  return t == "random_all" || typeHasRandomPrefix(t);
}

// If requestedCardId.type is random_* / random_all:
// - random_* selects a card from cards whose type matches the underlying base type
// - random_all selects a card from all non-random cards (fallback to any card)
// Then returns resolved card's type/voice/bmg/file.
bool resolveCardForPlayback(
  const String &requestedCardId,
  String &resolvedCardId,
  String &resolvedType,
  String &resolvedVoice,
  String &resolvedBmg,
  String &resolvedFile,
  int depth
){
  if(depth > 6) return false; // recursion guard
  resolvedCardId = "";
  resolvedType = "";
  resolvedVoice = "";
  resolvedBmg = "";
  resolvedFile = "";

  if(!soundConfig.containsKey("card") || !soundConfig["card"].containsKey(requestedCardId)){
    return false;
  }

  const JsonObject reqCard = soundConfig["card"][requestedCardId].as<JsonObject>();
  const char* reqTypeC = reqCard["type"];
  if(!reqTypeC) return false;
  String reqType = String(reqTypeC);
  if(reqType.length() == 0) return false;

  if(reqType == "random_all" || typeHasRandomPrefix(reqType)){
    String selectedId = "";
    int matchCount = 0;

    if(reqType == "random_all"){
      // First pass: only among non-random cards to avoid recursion loops.
      for(JsonPair kv : soundConfig["card"].as<JsonObject>()){
        const char* cid = kv.key().c_str();
        JsonObject card = kv.value().as<JsonObject>();

        const char* ct = card["type"];
        const char* cf = card["file"];
        if(!ct || !cf) continue;

        String cardType = String(ct);
        if(cardType.length() == 0) continue;
        if(cardTypeIsRandom(cardType)) continue;

        if(String(cf).length() == 0) continue;

        matchCount++;
        if(random(matchCount) == 0){
          selectedId = String(cid);
        }
      }

      // Fallback: if user has no non-random cards, allow any card with file.
      if(selectedId.length() == 0){
        matchCount = 0;
        for(JsonPair kv : soundConfig["card"].as<JsonObject>()){
          const char* cid = kv.key().c_str();
          JsonObject card = kv.value().as<JsonObject>();

          const char* cf = card["file"];
          if(!cf) continue;

          if(String(cf).length() == 0) continue;

          matchCount++;
          if(random(matchCount) == 0){
            selectedId = String(cid);
          }
        }
      }
    } else {
      // random_<base>
      // Example: random_kamen_rider -> underlying base type is "kamen_ride" (and we also try direct match).
      String derived = reqType.substring(7); // after "random_"
      String mappedBase = derived;

      // If it ends with "_rider", map to "_ride" to match existing basic keys.
      const String fromSuffix = "_rider";
      const String toSuffix   = "_ride";
      if(mappedBase.length() >= fromSuffix.length()){
        const String tail = mappedBase.substring(mappedBase.length() - fromSuffix.length());
        if(tail == fromSuffix){
          mappedBase = mappedBase.substring(0, mappedBase.length() - fromSuffix.length()) + toSuffix;
        }
      }

      for(JsonPair kv : soundConfig["card"].as<JsonObject>()){
        const char* cid = kv.key().c_str();
        JsonObject card = kv.value().as<JsonObject>();

        const char* ct = card["type"];
        const char* cf = card["file"];
        if(!ct || !cf) continue;

        String cardType = String(ct);
        if(cardType.length() == 0) continue;
        if(cardTypeIsRandom(cardType)) continue; // avoid recursion
        if(String(cf).length() == 0) continue;

        if(cardType != mappedBase && cardType != derived) continue;

        matchCount++;
        if(random(matchCount) == 0){
          selectedId = String(cid);
        }
      }
    }

    if(selectedId.length() == 0) return false;
    return resolveCardForPlayback(selectedId, resolvedCardId, resolvedType, resolvedVoice, resolvedBmg, resolvedFile, depth + 1);
  }

  // Normal card
  const char* fileC = reqCard["file"];
  const char* voiceC = reqCard["voice"];
  const char* bmgC = reqCard["bmg"];

  if(!fileC || String(fileC).length() == 0) return false;

  resolvedCardId = requestedCardId;
  resolvedType = reqType;
  resolvedVoice = voiceC ? String(voiceC) : "";
  resolvedBmg = bmgC ? String(bmgC) : "";
  resolvedFile = String(fileC);
  return true;
}

//////////////////////////////////////////////////////
// HANDLE CARD
//////////////////////////////////////////////////////
void handleCard(String id){

  Serial.println("CARD: "+id);

  if(!soundConfig.containsKey("card") || !soundConfig["card"].containsKey(id)){
    Serial.println("Card not found, playing error...");
    playBase("error");
    playState=IDLE;  // Reset state sau khi phát error
    inCardFinished=false;
    return;
  }

  // Resolve playback data:
  // - Normal card: use its own type/voice/bmg/file
  // - Random card (type starts with "random_"/is "random_all"): pick another card and use its data
  String resolvedCardId, resolvedType, resolvedVoice, resolvedBmg, resolvedFile;
  if(!resolveCardForPlayback(id, resolvedCardId, resolvedType, resolvedVoice, resolvedBmg, resolvedFile, 0)){
    Serial.println("Random resolve failed, playing error...");
    playBase("error");
    playState=IDLE;
    inCardFinished=false;
    return;
  }

  // Set current voice type for touch functionality
  currentVoiceType = resolvedVoice;
  currentVoiceIndex = 0; // Reset voice index when new card is loaded
  currentCardId = resolvedCardId; // Store resolved card ID for BMG playback

  Serial.printf("CARD_VOICE: Requested card %s resolved to %s (voice='%s')\n",
                id.c_str(), resolvedCardId.c_str(), currentVoiceType.c_str());

  // Phát type từ base section (type of resolved card)
  playBase(resolvedType.c_str());
  typePlayingPulse=true;  // Bật pulse LED
  typePulseStart=millis();

  // Lưu file number cho button 17
  savedFileNumber = fileToNum(resolvedFile.c_str());
}

//////////////////////////////////////////////////////
// WIFI / WEB helper functions
//////////////////////////////////////////////////////
void startAP(){
  WiFi.mode(WIFI_AP);
  bool ok = WiFi.softAP(AP_SSID, AP_PASS);
  if(ok){
    Serial.printf("AP started: %s (pass %s)\n", AP_SSID, AP_PASS);
    apStartMillis = millis();
    apActive = true;
  } else {
    Serial.println("Failed to start AP");
  }
}

void stopAP(){
  WiFi.softAPdisconnect(true);
  WiFi.mode(WIFI_OFF);
  apActive = false;
  Serial.println("AP stopped due to timeout (no clients)");
}

// Serve the web UI (single-page application)
const char index_html[] PROGMEM = R"rawliteral(
<!DOCTYPE html>
<html>
<head>
<meta charset="utf-8" />
<title>Driver Decade</title>
<meta name="viewport" content="width=device-width,initial-scale=1" />
<style>
/* Pink-first theme */
:root{
  --bg:#0a0b10;
  --panel:#10121a;
  --panel2:#141829;
  --text:#ffffff;
  --muted:#b6b8c6;
  --border:#25283a;
  --primary:#ff4fa1;     /* pink */
  --primary2:#ff2f88;
  --green:#31ff7a;       /* accent green */
  --danger:#ff3b5c;
  --danger2:#e1324f;
}
body{font-family:system-ui;background:radial-gradient(1200px 600px at 18% 0%, rgba(255,79,161,.22), transparent 60%),var(--bg);color:var(--text);margin:0;padding:18px}
.card{background:linear-gradient(180deg, rgba(255,79,161,.08), transparent 140px),var(--panel);border:1px solid var(--border);border-radius:14px;padding:16px}
.tabs{display:flex;gap:8px;margin-bottom:12px;flex-wrap:wrap;overflow-x:auto}
.tabbtn{flex:1;padding:10px;border-radius:10px;border:1px solid var(--border);background:rgba(255,255,255,.04);color:var(--text);cursor:pointer}
.tabbtn.active{background:linear-gradient(90deg,var(--primary),var(--primary2));border-color:rgba(255,79,161,.55);color:#0b0c12}
.color-grid{display:grid;grid-template-columns:repeat(4,1fr);gap:10px;margin-top:10px}
.color-btn{height:48px;border-radius:8px;font-weight:600;border:none;cursor:pointer}
.rgb{background:linear-gradient(90deg,#ff0000,#00ff00,#0000ff);color:#000}
.msg{margin-top:8px;font-size:13px;color:var(--muted)}
.manage-tabs{display:flex;gap:6px;margin-bottom:15px;border-bottom:1px solid var(--border);padding-bottom:8px}
.manage-tab-btn{flex:1;padding:8px;border-radius:10px;border:1px solid var(--border);background:rgba(255,255,255,.04);color:var(--text);cursor:pointer;font-size:12px}
.manage-tab-btn.active{background:linear-gradient(90deg,var(--primary),var(--primary2));border-color:rgba(255,79,161,.55);color:#0b0c12}
.manage-subsection{background:var(--panel2);border:1px solid var(--border);padding:15px;border-radius:12px;margin-bottom:15px}
.form-group{margin-bottom:15px}
.form-group h6{margin:10px 0 8px 0;color:var(--muted);font-size:14px}
.form-control{width:100%;padding:10px;border-radius:10px;border:1px solid var(--border);background:rgba(255,255,255,.04);color:var(--text);margin-bottom:8px}
.form-control:focus{outline:none;border-color:rgba(255,79,161,.75);box-shadow:0 0 0 3px rgba(255,79,161,.18)}
button{background:linear-gradient(90deg,var(--primary),var(--primary2));color:#0b0c12;border:none;padding:8px 16px;border-radius:10px;cursor:pointer;margin-right:8px;font-weight:700}
button:hover{filter:saturate(1.1) brightness(1.05)}
.form-control:disabled{opacity:.55;cursor:not-allowed}
.data-list{max-height:200px;overflow-y:auto;border:1px solid #333;border-radius:4px;background:#0f1115;padding:8px;margin-top:10px}
.data-item{display:flex;justify-content:space-between;align-items:center;padding:6px;border-bottom:1px solid #333}
.data-item:last-child{border-bottom:none}
.table-wrap{max-height:340px;overflow:auto;border:1px solid var(--border);border-radius:12px;background:rgba(0,0,0,.25);margin-top:10px}
.data-table{width:100%;border-collapse:separate;border-spacing:0;table-layout:fixed}
.data-table th,.data-table td{padding:12px;border-bottom:1px solid rgba(255,255,255,.06);text-align:left;font-size:13px;vertical-align:top;word-break:break-word;white-space:normal}
.data-table th{position:sticky;top:0;background:linear-gradient(180deg, rgba(255,79,161,.12), rgba(20,24,41,.95));color:var(--muted);z-index:1}
.data-table tr:hover td{background:#0c0f18}
.row-actions{display:flex;gap:6px;flex-wrap:wrap}
.btn-sm{padding:6px 10px;font-size:12px;border-radius:999px;margin-right:0}
.btn-ghost{background:rgba(255,255,255,.06);color:var(--text);border:1px solid var(--border)}
.btn-ghost:hover{background:rgba(255,255,255,.10)}
.btn-danger{background:linear-gradient(90deg,var(--danger),var(--danger2));color:#0b0c12}
.btn-danger:hover{filter:saturate(1.1) brightness(1.05)}
.btn-muted{background:rgba(49,255,122,.12);border:1px solid rgba(49,255,122,.30);color:var(--green)}
.btn-muted:hover{background:rgba(49,255,122,.18)}
.mono{font-family:ui-monospace, SFMono-Regular, Menlo, Monaco, Consolas, "Liberation Mono", "Courier New", monospace}
.pill{display:inline-flex;align-items:center;gap:6px;padding:6px 8px;border-radius:999px;background:rgba(255,255,255,.04);border:1px solid var(--border);margin:4px 6px 0 0}
.pill button{padding:4px 8px;border-radius:999px}
.filters{display:grid;grid-template-columns:repeat(4,minmax(0,1fr));gap:8px;margin:10px 0}
.filters .form-control{margin-bottom:0}

/* Modal */
.modal{position:fixed;inset:0;background:rgba(0,0,0,.6);display:none;align-items:center;justify-content:center;padding:18px;z-index:50}
.modal.show{display:flex}
.modal-card{width:min(860px,100%);background:linear-gradient(180deg, rgba(255,79,161,.12), transparent 120px),var(--panel);border:1px solid rgba(255,79,161,.25);border-radius:16px;box-shadow:0 20px 60px rgba(0,0,0,.65)}
.modal-head{display:flex;align-items:center;justify-content:space-between;padding:14px 16px;border-bottom:1px solid rgba(255,255,255,.08)}
.modal-title{font-weight:700}
.modal-body{padding:16px}
.modal-foot{display:flex;justify-content:flex-end;gap:8px;padding:14px 16px;border-top:1px solid rgba(255,255,255,.08)}
.grid2{display:grid;grid-template-columns:1fr 1fr;gap:10px}

/* Card table: allow horizontal scroll, avoid wrapping */
#cardTable{table-layout:auto;min-width:980px}
#cardTable th,#cardTable td{white-space:nowrap;word-break:normal;vertical-align:middle}
@media (max-width:480px){
  body{padding:12px}
  .tabs{gap:4px}
  .tabbtn{padding:8px 6px;font-size:12px}
  .color-grid{grid-template-columns:repeat(2,1fr)}
  .filters{grid-template-columns:1fr}
  .grid2{grid-template-columns:1fr}
}
</style>
</head>
<body>
<h3>Driver Decade</h3>
<div class="card">
  <div class="tabs">
    <button class="tabbtn" id="tabLed">LED</button>
    <button class="tabbtn" id="tabVolume">Volume</button>
    <button class="tabbtn active" id="tabManage">Manage</button>
    <button class="tabbtn" id="tabLog">Log</button>
  </div>

  <div id="ledSection" style="display:none">
    <p>Nhấn nút màu để áp dụng ngay (hoặc nhấn RGB Mode)</p>
    <div class="color-grid">
      <button class="color-btn" style="background:#ff0000;color:#fff" onclick="preset('red')">Red</button>
      <button class="color-btn" style="background:#00ff00;color:#000" onclick="preset('green')">Green</button>
      <button class="color-btn" style="background:#0000ff;color:#fff" onclick="preset('blue')">Blue</button>
      <button class="color-btn" style="background:#ffff00;color:#000" onclick="preset('yellow')">Yellow</button>
      <button class="color-btn" style="background:#ff00ff;color:#000" onclick="preset('magenta')">Magenta</button>
      <button class="color-btn" style="background:#00ffff;color:#000" onclick="preset('cyan')">Cyan</button>
      <button class="color-btn" style="background:#ffffff;color:#000" onclick="preset('white')">White</button>
      <button class="color-btn rgb" onclick="preset('rgb')">RGB Mode</button>
    </div>
    <div id="ledMsg" class="msg"></div>
  </div>

  <div id="volumeSection" style="display:none">
    <h4>Speaker Volume</h4>
    <p>Adjust speaker volume (0-30). Current setting will be saved automatically.</p>
    <div style="margin:20px 0">
      <label for="volumeSlider" style="display:block;margin-bottom:10px">Volume: <span id="volumeValue">25</span></label>
      <input type="range" id="volumeSlider" min="0" max="30" value="25" style="width:100%;max-width:300px">
    </div>
    <button onclick="setVolume()">Set Volume</button>
    <div id="volumeMsg" class="msg"></div>
  </div>

  <div id="manageSection">
    <h4>Data Manager</h4>
    <div style="background:#1a1d24;padding:10px;border-radius:6px;margin-bottom:15px;font-size:12px;color:#9aa3b2">
      <strong>Folder Structure:</strong><br>
      📁 /01/ - Basic sounds (types, in_card, out_card, etc.)<br>
      📁 /02/ - Voice sounds (decade, kiva, blade)<br>
      📁 /03/ - BMG sounds (background music)<br>
      📁 /MP3/ - Card sounds (main files)
    </div>
    <div class="manage-tabs">
      <button class="manage-tab-btn active" id="manageTabType">Type</button>
      <button class="manage-tab-btn" id="manageTabVoice">Voice</button>
      <button class="manage-tab-btn" id="manageTabBmg">BMG</button>
      <button class="manage-tab-btn" id="manageTabCard">Card</button>
    </div>

    <div id="manageContent">
      <!-- Type Management -->
      <div id="typeSection" class="manage-subsection">
        <div style="display:flex;align-items:center;justify-content:space-between;gap:10px;flex-wrap:wrap">
          <h5 style="margin:0">Type Management <small style="color:#9aa3b2">(plays from folder 01)</small></h5>
          <button class="btn-sm btn-muted" onclick="openModalType()">+ Add Type</button>
        </div>
        <div class="table-wrap">
          <table class="data-table" id="typeTable">
            <thead>
              <tr>
                <th style="width:28%">Type</th>
                <th style="width:32%">File (folder 01)</th>
                <th style="width:40%">Actions</th>
              </tr>
            </thead>
            <tbody id="typeTableBody"></tbody>
          </table>
        </div>

        <div id="typeMsg" class="msg"></div>
      </div>

      <!-- Voice Management -->
      <div id="voiceSection" class="manage-subsection" style="display:none">
        <div style="display:flex;align-items:center;justify-content:space-between;gap:10px;flex-wrap:wrap">
          <h5 style="margin:0">Voice Management <small style="color:#9aa3b2">(plays from folder 02)</small></h5>
          <button class="btn-sm btn-muted" onclick="openModalVoice()">+ Add Voice</button>
        </div>
        <div class="table-wrap">
          <table class="data-table" id="voiceTable">
            <thead>
              <tr>
                <th style="width:22%">Voice</th>
                <th style="width:38%">Files (folder 02)</th>
                <th style="width:40%">Actions</th>
              </tr>
            </thead>
            <tbody id="voiceTableBody"></tbody>
          </table>
        </div>

        <div id="voiceMsg" class="msg"></div>
      </div>

      <!-- BMG Management -->
      <div id="bmgSection" class="manage-subsection" style="display:none">
        <div style="display:flex;align-items:center;justify-content:space-between;gap:10px;flex-wrap:wrap">
          <h5 style="margin:0">BMG Management <small style="color:#9aa3b2">(plays from folder 03)</small></h5>
          <button class="btn-sm btn-muted" onclick="openModalBmg()">+ Add BMG</button>
        </div>
        <div class="table-wrap">
          <table class="data-table" id="bmgTable">
            <thead>
              <tr>
                <th style="width:28%">BMG</th>
                <th style="width:32%">File (folder 03)</th>
                <th style="width:40%">Actions</th>
              </tr>
            </thead>
            <tbody id="bmgTableBody"></tbody>
          </table>
        </div>

        <div id="bmgMsg" class="msg"></div>
      </div>

      <!-- Card Management -->
      <div id="cardSection" class="manage-subsection" style="display:none">
        <div style="display:flex;align-items:center;justify-content:space-between;gap:10px;flex-wrap:wrap">
          <h5 style="margin:0">Card Management <small style="color:#9aa3b2">(main files from /MP3/ folder)</small></h5>
          <button class="btn-sm btn-muted" onclick="openModalCard()">+ Add Card</button>
        </div>

        <div class="filters">
          <input id="cardFilterQuery" class="form-control" placeholder="Search by name or 11-bit id..." oninput="applyCardFilters()">
          <select id="cardFilterType" class="form-control" onchange="applyCardFilters()"></select>
        </div>
        <div class="table-wrap">
          <table class="data-table" id="cardTable">
            <thead>
              <tr>
                <th style="width:22%">Card ID</th>
                <th style="width:22%">Name</th>
                <th style="width:16%">Type</th>
                <th style="width:20%">File</th>
                <th style="width:20%">Actions</th>
              </tr>
            </thead>
            <tbody id="cardTableBody"></tbody>
          </table>
        </div>

        <div id="cardMsg" class="msg"></div>
      </div>
    </div>
  </div>

  <div id="logSection" style="display:none">
    <h4>Card Log</h4>
    <p>Danh sách các lần đọc thẻ gần nhất (tự động refresh mỗi 1 giây).</p>
    <textarea id="logArea" readonly></textarea>
    <div style="margin-top:8px">
      <button onclick="clearLog()">Clear</button>
      <span class="msg" id="logMsg"></span>
    </div>
  </div>
</div>

<!-- Modal -->
<div class="modal" id="modal">
  <div class="modal-card">
    <div class="modal-head">
      <div class="modal-title" id="modalTitle">Edit</div>
      <button class="btn-sm btn-ghost" onclick="closeModal()">Close</button>
    </div>
    <div class="modal-body" id="modalBody"></div> 
  </div>
</div>

<script>
const tLed     = document.getElementById('tabLed');
const tVolume  = document.getElementById('tabVolume');
const tManage  = document.getElementById('tabManage');
const tLog     = document.getElementById('tabLog');

const sLed     = document.getElementById('ledSection');
const sVolume  = document.getElementById('volumeSection');
const sManage  = document.getElementById('manageSection');
const sLog     = document.getElementById('logSection');

// Initial view: show Manage immediately
sManage.style.display = 'block';
sLed.style.display = 'none';
sVolume.style.display = 'none';
sLog.style.display = 'none';
tManage.classList.add('active');
tLed.classList.remove('active');
tVolume.classList.remove('active');
tLog.classList.remove('active');
loadManageData();

function escapeHtml(s){
  return String(s)
    .replaceAll('&','&amp;')
    .replaceAll('<','&lt;')
    .replaceAll('>','&gt;')
    .replaceAll('"','&quot;')
    .replaceAll("'","&#039;");
}

async function loadNotePopup(){
  try{
    const r = await fetch('/note', { cache: 'no-store' });
    if(!r.ok) return;
    const t = (await r.text()).trim();
    if(!t || t.toLowerCase() === 'no') return;
    openModal(
      'note',
      'Note',
      `
        <div style="white-space:pre-wrap;line-height:1.35">${escapeHtml(t)}</div>
        <div style="margin-top:12px;display:flex;gap:8px;justify-content:flex-end">
          <button class="btn-sm btn-ghost" onclick="disableNote()">Don't show again</button>
        </div>
      `
    );
  }catch(e){
    // ignore
  }
}

async function disableNote(){
  try{
    await fetch('/note?note=no', { cache:'no-store' });
  }catch(e){}
  closeModal();
}

tLed.onclick = ()=>{
  tLed.classList.add('active');
  tVolume.classList.remove('active');
  tManage.classList.remove('active');
  tLog.classList.remove('active');
  sLed.style.display='block';
  sVolume.style.display='none';
  sManage.style.display='none';
  sLog.style.display='none';
};

tVolume.onclick = ()=>{
  tVolume.classList.add('active');
  tLed.classList.remove('active');
  tManage.classList.remove('active');
  tLog.classList.remove('active');
  sVolume.style.display='block';
  sLed.style.display='none';
  sManage.style.display='none';
  sLog.style.display='none';
  // Khi mở tab Volume thì load volume hiện tại từ ESP
  loadVolume();
};

// Manage tab
tManage.onclick = ()=>{
  tManage.classList.add('active');
  tLed.classList.remove('active');
  tVolume.classList.remove('active');
  tLog.classList.remove('active');
  sManage.style.display='block';
  sLed.style.display='none';
  sVolume.style.display='none';
  sLog.style.display='none';
  loadManageData();
};

// Log tab
tLog.onclick = ()=>{
  tLog.classList.add('active');
  tLed.classList.remove('active');
  tVolume.classList.remove('active');
  tManage.classList.remove('active');
  sLog.style.display='block';
  sLed.style.display='none';
  sVolume.style.display='none';
  sManage.style.display='none';
  refreshLog();
};

// Manage sub-tabs
const manageTabs = ['type', 'voice', 'bmg', 'card'];
manageTabs.forEach(tab => {
  document.getElementById(`manageTab${tab.charAt(0).toUpperCase() + tab.slice(1)}`).onclick = () => {
    // Hide all subsections
    document.querySelectorAll('.manage-subsection').forEach(section => section.style.display = 'none');
    // Remove active class from all tab buttons
    document.querySelectorAll('.manage-tab-btn').forEach(btn => btn.classList.remove('active'));
    // Show selected subsection and activate tab
    document.getElementById(`${tab}Section`).style.display = 'block';
    document.getElementById(`manageTab${tab.charAt(0).toUpperCase() + tab.slice(1)}`).classList.add('active');
  };
});

// Helper function to sort object keys by file names
function sortByFileName(obj) {
  const sorted = {};
  Object.keys(obj).sort((a, b) => {
    // Extract numbers from file names for sorting
    const aMatch = obj[a].match(/(\d+)\.mp3/);
    const bMatch = obj[b].match(/(\d+)\.mp3/);

    if (aMatch && bMatch) {
      return parseInt(aMatch[1]) - parseInt(bMatch[1]);
    }

    // If no numbers, sort alphabetically
    return obj[a].localeCompare(obj[b]);
  }).forEach(key => {
    sorted[key] = obj[key];
  });
  return sorted;
}

// Helper function to sort entire JSON data
function sortJsonData(data) {
  // Sort basic section
  if (data.basic) {
    const orderedBasic = {};

    // Keep boot_sound first
    if(data.basic.boot_sound) orderedBasic.boot_sound = data.basic.boot_sound;

    // Get editable entries (types and bmg)
    const editableEntries = {};
    Object.keys(data.basic).forEach(key => {
      if(key !== 'boot_sound' && !['in_card', 'out_card', 'open', 'close', 'error', 'touch'].includes(key)){
        editableEntries[key] = data.basic[key];
      }
    });

    // Sort editable entries by file name
    const sortedEditable = sortByFileName(editableEntries);
    Object.assign(orderedBasic, sortedEditable);

    // Add system types back (touch, etc.)
    ['touch', 'in_card', 'out_card', 'open', 'close', 'error'].forEach(key => {
      if(data.basic[key]) orderedBasic[key] = data.basic[key];
    });

    data.basic = orderedBasic;
  }

  // Sort voice section alphabetically
  if (data.voice) {
    const sortedVoice = {};
    Object.keys(data.voice).sort().forEach(key => {
      sortedVoice[key] = data.voice[key];
    });
    data.voice = sortedVoice;
  }

  // Sort card section by file name
  if (data.card) {
    const sortedCard = {};
    Object.keys(data.card).sort((a, b) => {
      // Random card types might not have `file` saved (only {name,type}), so guard against undefined.
      const aFile = data.card[a].file || '';
      const bFile = data.card[b].file || '';

      const aMatch = aFile.match(/(\d+)\.mp3/);
      const bMatch = bFile.match(/(\d+)\.mp3/);

      if (aMatch && bMatch) {
        return parseInt(aMatch[1]) - parseInt(bMatch[1]);
      }

      return aFile.localeCompare(bFile);
    }).forEach(key => {
      sortedCard[key] = data.card[key];
    });
    data.card = sortedCard;
  }

  return data;
}

// Random type helpers (UI-only)
function toRandomTypeName(baseType){
  // Example: kamen_ride -> random_kamen_ride
  // We generate "random_<type>" from current basic types so user can just pick it on Card.
  if(!baseType) return '';
  return `random_${String(baseType)}`;
}

function isRandomTypeName(t){
  return t === 'random_all' || (typeof t === 'string' && t.startsWith('random_'));
}

function titleCaseFromType(typeStr){
  const s = String(typeStr || '').replace(/^random_/, 'random_');
  const parts = s.split('_').filter(Boolean);
  return parts.map(w => w.charAt(0).toUpperCase() + w.slice(1)).join(' ');
}

function updateCardModalByType(){
  const typeEl = document.getElementById('m_cardType');
  const nameEl = document.getElementById('m_cardName');
  const fileEl = document.getElementById('m_cardFile');
  const voiceEl = document.getElementById('m_cardVoice');
  const bmgEl = document.getElementById('m_cardBmg');
  const fileWrap = document.getElementById('m_cardFileWrap');
  const voiceWrap = document.getElementById('m_cardVoiceWrap');
  const bmgWrap = document.getElementById('m_cardBmgWrap');
  if(!typeEl || !nameEl || !fileEl || !voiceEl || !bmgEl) return;

  const t = typeEl.value;
  const isRand = isRandomTypeName(t);

  // Auto-fill name for random types if user hasn't customized it.
  if(isRand){
    const autoName = titleCaseFromType(t);
    const cur = nameEl.value.trim();
    const lastAuto = nameEl.getAttribute('data-last-auto') || '';
    if(cur.length === 0 || cur === lastAuto){
      nameEl.value = autoName;
      nameEl.setAttribute('data-last-auto', autoName);
    }
    // Ensure random card name stays consistent
    nameEl.disabled = true;
  } else {
    nameEl.disabled = false;
  }

  // Hide dependent fields for random cards
  if(fileWrap) fileWrap.style.display = isRand ? 'none' : 'block';
  if(voiceWrap) voiceWrap.style.display = isRand ? 'none' : 'block';
  if(bmgWrap) bmgWrap.style.display = isRand ? 'none' : 'block';

  if(isRand){
    // Clear/force defaults so we don't persist misleading values
    fileEl.value = '';
    voiceEl.value = 'decade';
    bmgEl.value = 'decade';
  }
}

// Modal state
let modalState = { kind:'', key:'' };

function closeModal(){
  document.getElementById('modal').classList.remove('show');
  document.getElementById('modalBody').innerHTML = '';
  modalState = { kind:'', key:'' };
}

function openModal(kind, title, bodyHtml, key){
  modalState = { kind, key: key || '' };
  document.getElementById('modalTitle').textContent = title;
  document.getElementById('modalBody').innerHTML = bodyHtml;
  document.getElementById('modal').classList.add('show');
}

function openModalType(key){
  const isEdit = !!key;
  const name = isEdit ? key : '';
  const file = (isEdit && manageData.basic && manageData.basic[key]) ? manageData.basic[key] : '';
  openModal(
    'type',
    isEdit ? `Edit Type: ${key}` : 'Add Type',
    `
      <div class="grid2">
        <div>
          <div class="msg">Type name</div>
          <input id="m_typeName" class="form-control mono" value="${name}" ${isEdit ? 'readonly' : ''} placeholder="kamen_ride">
        </div>
        <div>
          <div class="msg">File in /01/</div>
          <input id="m_typeFile" class="form-control mono" value="${file}" placeholder="006.mp3">
        </div>
      </div>
      <div class="msg" id="m_msg"></div>
    `,
    key
  );
}

function openModalVoice(key){
  const isEdit = !!key;
  const name = isEdit ? key : '';
  const files = (isEdit && manageData.voice && Array.isArray(manageData.voice[key])) ? manageData.voice[key].join(',') : '';
  openModal(
    'voice',
    isEdit ? `Edit Voice: ${key}` : 'Add Voice',
    `
      <div>
        <div class="msg">Voice name ${isEdit && key==='decade' ? '(default - cannot delete)' : ''}</div>
        <input id="m_voiceName" class="form-control mono" value="${name}" ${isEdit ? 'readonly' : ''} placeholder="decade">
      </div>
      <div>
        <div class="msg">Files in /02/ (comma separated)</div>
        <textarea id="m_voiceFiles" class="form-control mono" rows="4" placeholder="001.mp3,002.mp3">${files}</textarea>
      </div>
      <div class="msg" id="m_msg"></div>
    `,
    key
  );
}

function openModalBmg(key){
  const isEdit = !!key;
  const name = isEdit ? key : '';
  const file = (isEdit && manageData.bmg && manageData.bmg[key]) ? manageData.bmg[key] : '';
  openModal(
    'bmg',
    isEdit ? `Edit BMG: ${key}` : 'Add BMG',
    `
      <div class="grid2">
        <div>
          <div class="msg">BMG name ${isEdit && key==='decade' ? '(default - cannot delete)' : ''}</div>
          <input id="m_bmgName" class="form-control mono" value="${name}" ${isEdit ? 'readonly' : ''} placeholder="decade">
        </div>
        <div>
          <div class="msg">File in /03/</div>
          <input id="m_bmgFile" class="form-control mono" value="${file}" placeholder="012.mp3">
        </div>
      </div>
      <div class="msg" id="m_msg"></div>
    `,
    key
  );
}

function openModalCard(key){
  const isEdit = !!key;
  const card = (isEdit && manageData.card && manageData.card[key]) ? manageData.card[key] : {};
  const id = isEdit ? key : '';
  const name = card.name || '';
  const type = card.type || 'kamen_ride';
  const voice = card.voice || 'decade';
  const bmg = card.bmg || 'decade';
  const file = card.file || '';

  // Build selects from current data
  let typeOpts = '';
  const normalKeys = [];
  if(manageData.basic){
    Object.keys(manageData.basic).forEach(k => {
      if(k !== 'boot_sound' && !['in_card','out_card','open','close','error','touch'].includes(k)) normalKeys.push(k);
    });
  }
  if(normalKeys.length === 0) normalKeys.push('kamen_ride');
  if(!normalKeys.includes('kamen_ride')) normalKeys.unshift('kamen_ride');
  typeOpts += `<optgroup label="Normal Type">`;
  normalKeys.forEach(k => typeOpts += `<option value="${k}" ${k===type?'selected':''}>${k}</option>`);
  typeOpts += `</optgroup><optgroup label="Random Type">`;
  normalKeys.forEach(k => {
    const r = toRandomTypeName(k);
    typeOpts += `<option value="${r}" ${r===type?'selected':''}>${r}</option>`;
  });
  typeOpts += `<option value="random_all" ${type==='random_all'?'selected':''}>random_all</option></optgroup>`;

  let voiceOpts = '';
  if(manageData.voice){
    Object.keys(manageData.voice).sort().forEach(v => voiceOpts += `<option value="${v}" ${v===voice?'selected':''}>${v}</option>`);
  }
  if(!voiceOpts.includes('decade')) voiceOpts = `<option value="decade" selected>decade</option>` + voiceOpts;

  let bmgOpts = '';
  if(manageData.bmg){
    Object.keys(manageData.bmg).sort().forEach(v => bmgOpts += `<option value="${v}" ${v===bmg?'selected':''}>${v}</option>`);
  }
  if(!bmgOpts.includes('decade')) bmgOpts = `<option value="decade" selected>decade</option>` + bmgOpts;

  openModal(
    'card',
    isEdit ? `Edit Card: ${key}` : 'Add Card',
    `
      <div class="grid2">
        <div>
          <div class="msg">Card ID (11 bits 0/1)</div>
          <input id="m_cardId" class="form-control mono" value="${id}" placeholder="01010010101" maxlength="11">
        </div>
        <div>
          <div class="msg">Name</div>
          <input id="m_cardName" class="form-control" value="${name}" placeholder="Kuuga">
        </div>
      </div>
      <div class="grid2">
        <div>
          <div class="msg">Type</div>
          <select id="m_cardType" class="form-control mono" onchange="updateCardModalByType()">${typeOpts}</select>
        </div>
        <div id="m_cardFileWrap">
          <div class="msg">Main file in /MP3/ (optional for random_*)</div>
          <input id="m_cardFile" class="form-control mono" value="${file}" placeholder="0001.mp3">
        </div>
      </div>
      <div class="grid2">
        <div id="m_cardVoiceWrap">
          <div class="msg">Voice</div>
          <select id="m_cardVoice" class="form-control mono">${voiceOpts}</select>
        </div>
        <div id="m_cardBmgWrap">
          <div class="msg">BMG</div>
          <select id="m_cardBmg" class="form-control mono">${bmgOpts}</select>
        </div>
      </div>
      <div class="msg" id="m_msg"></div>
    `,
    key
  );

  // Apply initial lock/auto-name based on current type
  updateCardModalByType();
}

function modalSave(){
  const msgEl = document.getElementById('m_msg');
  const fail = (t)=>{ if(msgEl) msgEl.textContent = t; };
  try{
    if(modalState.kind === 'type'){
      const name = document.getElementById('m_typeName').value.trim();
      const file = document.getElementById('m_typeFile').value.trim();
      if(!name || !file) return fail('Name and file are required');
      if(!manageData.basic) manageData.basic = {};
      manageData.basic[name] = file;
      saveManageData();
      closeModal();
      return;
    }
    if(modalState.kind === 'voice'){
      const name = document.getElementById('m_voiceName').value.trim();
      const filesStr = document.getElementById('m_voiceFiles').value.trim();
      if(!name || !filesStr) return fail('Name and files are required');
      const files = filesStr.split(',').map(f => f.trim()).filter(Boolean);
      if(!manageData.voice) manageData.voice = {};
      manageData.voice[name] = files;
      saveManageData();
      closeModal();
      return;
    }
    if(modalState.kind === 'bmg'){
      const name = document.getElementById('m_bmgName').value.trim();
      const file = document.getElementById('m_bmgFile').value.trim();
      if(!name || !file) return fail('Name and file are required');
      if(!manageData.bmg) manageData.bmg = {};
      manageData.bmg[name] = file;
      saveManageData();
      closeModal();
      return;
    }
    if(modalState.kind === 'card'){
      const id = document.getElementById('m_cardId').value.trim();
      const name = document.getElementById('m_cardName').value.trim();
      const type = document.getElementById('m_cardType').value;
      let voice = document.getElementById('m_cardVoice').value;
      let bmg = document.getElementById('m_cardBmg').value;
      let file = document.getElementById('m_cardFile').value.trim();
      const randomType = isRandomTypeName(type);
      const isEdit = !!modalState.key;
      const oldId = isEdit ? modalState.key : '';

      if(!manageData.card) manageData.card = {};
      if(!isEdit){
        if(manageData.card[id]){
          return fail(`Card ID "${id}" already exists. Please Edit it instead.`);
        }
      } else {
        if(oldId !== id && manageData.card[id]){
          if(!confirm(`Card ID "${id}" already exists. Overwrite it?`)){
            document.getElementById('m_msg').textContent = 'Cancelled (duplicate ID)';
            return;
          }
        }
      }

      if(randomType){
        // For random cards, keep JSON minimal: only name/type are needed.
        file = '';
        voice = 'decade';
        bmg = 'decade';
      }
      if(!id || (!file && !randomType)) return fail('ID and file are required (file optional for random_*)');
      if(id.length !== 11 || !/^[01]{11}$/.test(id)) return fail('Card ID must be exactly 11 characters (0/1 only)');

      if(randomType){
        const nextCard = { name, type };
        if(isEdit && oldId && oldId !== id) delete manageData.card[oldId];
        manageData.card[id] = nextCard;
      } else {
        const nextCard = { name, type, voice, bmg, file };
        if(isEdit && oldId && oldId !== id) delete manageData.card[oldId];
        manageData.card[id] = nextCard;
      }
      saveManageData();
      closeModal();
      return;
    }
  }catch(e){
    fail('Error: ' + e.message);
  }
}

// Manage data functions
let manageData = {};

async function loadManageData(){
  try{
    const r = await fetch('/sound.json');
    manageData = await r.json();
    populateTypeData();
    populateVoiceData();
    populateBmgData();
    populateCardData();
    populateSelects();
  } catch(e){
    console.error('Failed to load manage data:', e);
  }
}

function populateTypeData(){
  const body = document.getElementById('typeTableBody');
  body.innerHTML = '';

  const editableKeys = [];
  if(manageData.basic){
    Object.keys(manageData.basic).forEach(key => {
      if(!['boot_sound', 'in_card', 'out_card', 'open', 'close', 'error', 'touch'].includes(key)){
        editableKeys.push(key);
      }
    });
  }

  editableKeys.sort().forEach(key => {
    const file = manageData.basic[key];
    body.innerHTML += `
      <tr>
        <td class="mono">${key}</td>
        <td class="mono">${file || ''}</td>
        <td>
          <div class="row-actions">
            <button class="btn-sm" onclick="playType('${key}')">Play</button>
            <button class="btn-sm btn-ghost" onclick="openModalType('${key}')">Edit</button>
            <button class="btn-sm btn-danger" onclick="deleteType('${key}')">Delete</button>
          </div>
        </td>
      </tr>
    `;
  });
}

function populateVoiceData(){
  const body = document.getElementById('voiceTableBody');
  body.innerHTML = '';
  if(!manageData.voice) return;

  Object.keys(manageData.voice).sort().forEach(key => {
    const filesArr = Array.isArray(manageData.voice[key]) ? manageData.voice[key] : [];
    let filesHtml = '';
    filesArr.forEach((f, idx) => {
      filesHtml += `<span class="pill mono"><span>${f}</span><button class="btn-sm" onclick="playVoiceType('${key}',${idx})">Play</button></span>`;
    });
    body.innerHTML += `
      <tr>
        <td class="mono">${key}</td>
        <td>${filesHtml || '<span class="msg">empty</span>'}</td>
        <td>
          <div class="row-actions">
            <button class="btn-sm" onclick="playVoiceType('${key}',0)">Play (0)</button>
            <button class="btn-sm btn-ghost" onclick="openModalVoice('${key}')">Edit</button>
            <button class="btn-sm btn-danger" onclick="deleteVoice('${key}')">Delete</button>
          </div>
        </td>
      </tr>
    `;
  });
}

function populateBmgData(){
  const body = document.getElementById('bmgTableBody');
  body.innerHTML = '';
  if(!manageData.bmg) return;

  Object.keys(manageData.bmg).sort().forEach(key => {
    const file = manageData.bmg[key];
    body.innerHTML += `
      <tr>
        <td class="mono">${key}</td>
        <td class="mono">${file || ''}</td>
        <td>
          <div class="row-actions">
            <button class="btn-sm" onclick="playBmgKey('${key}')">Play</button>
            <button class="btn-sm btn-ghost" onclick="openModalBmg('${key}')">Edit</button>
            <button class="btn-sm btn-danger" onclick="deleteBmg('${key}')">Delete</button>
          </div>
        </td>
      </tr>
    `;
  });
}

function populateCardData(){
  const body = document.getElementById('cardTableBody');
  body.innerHTML = '';
  if(!manageData.card) return;

  const q = (document.getElementById('cardFilterQuery')?.value || '').trim();
  let nameFilter = '';
  let idFilter = '';
  if(q){
    // If user includes an 11-bit string, treat it as Card ID.
    const bitMatch = q.match(/([01]{11})/);
    if(bitMatch){
      idFilter = bitMatch[1];
      nameFilter = q.replace(bitMatch[1], '').trim().toLowerCase();
    } else {
      nameFilter = q.toLowerCase();
    }
  }
  const typeFilter = document.getElementById('cardFilterType')?.value || '';

  Object.keys(manageData.card).sort().forEach(key => {
    const card = manageData.card[key] || {};
    const name = card.name || '';
    const type = card.type || '';
    const file = card.file || '';

    if(idFilter && String(key) !== idFilter) return;
    if(nameFilter && !String(name).toLowerCase().includes(nameFilter)) return;
    if(typeFilter && type !== typeFilter) return;

    body.innerHTML += `
      <tr>
        <td class="mono">${key}</td>
        <td>${name}</td>
        <td class="mono">${type}</td>
        <td class="mono">${file}</td>
        <td>
          <div class="row-actions">
            <button class="btn-sm" onclick="playCardKey('${key}')">Play</button>
            <button class="btn-sm btn-ghost" onclick="openModalCard('${key}')">Edit</button>
            <button class="btn-sm btn-danger" onclick="deleteCard('${key}')">Delete</button>
          </div>
        </td>
      </tr>
    `;
  });
}

// Preview data lists
function populatePreviewData(){
  // Preview tab removed — guard to avoid breaking the page
  const basicList = document.getElementById('previewBasicList');
  if(!basicList) return;
  // Basic
  basicList.innerHTML = '<option value="">Select basic sound...</option>';
  if(manageData.basic){
    Object.keys(manageData.basic).forEach(key => {
      basicList.innerHTML += `<option value="${key}">${key} -> ${manageData.basic[key]}</option>`;
    });
  }

  // Voice
  const voiceList = document.getElementById('previewVoiceList');
  voiceList.innerHTML = '<option value="">Select voice type...</option>';
  if(manageData.voice){
    Object.keys(manageData.voice).forEach(key => {
      const files = manageData.voice[key].join(', ');
      voiceList.innerHTML += `<option value="${key}">${key} -> [${files}]</option>`;
    });
  }

  // BMG
  const bmgList = document.getElementById('previewBmgList');
  bmgList.innerHTML = '<option value="">Select BMG...</option>';
  if(manageData.bmg){
    Object.keys(manageData.bmg).forEach(key => {
      bmgList.innerHTML += `<option value="${key}">${key} -> ${manageData.bmg[key]}</option>`;
    });
  }

  // Card
  const cardList = document.getElementById('previewCardList');
  cardList.innerHTML = '<option value="">Select card...</option>';
  if(manageData.card){
    Object.keys(manageData.card).forEach(key => {
      const card = manageData.card[key];
      const name = card.name || '';
      const label = name ? `${key} [${name}]` : key;
      cardList.innerHTML += `<option value="${key}">${label} -> ${card.type}/${card.voice}/${card.file}</option>`;
    });
  }
}

function populateSelects(){
  // Card add/edit uses modal-built selects now, so the only remaining "select"
  // we need to keep in sync on the main page is the Card filter by type.
  populateCardFilterSelects();
}

function populateCardFilterSelects(){
  const typeSel = document.getElementById('cardFilterType');
  if(!typeSel) return;

  const normalKeys = [];
  if(manageData.basic){
    Object.keys(manageData.basic).forEach(k => {
      if(k !== 'boot_sound' && !['in_card','out_card','open','close','error','touch'].includes(k)) normalKeys.push(k);
    });
  }
  normalKeys.sort();

  let html = `<option value="">All types</option>`;
  html += `<optgroup label="Normal Type">` + normalKeys.map(k => `<option value="${k}">${k}</option>`).join('') + `</optgroup>`;
  const randomKeys = normalKeys.map(k => toRandomTypeName(k));
  html += `<optgroup label="Random Type">` +
            `<option value="random_all">random_all</option>` +
            randomKeys.map(k => `<option value="${k}">${k}</option>`).join('') +
          `</optgroup>`;
  typeSel.innerHTML = html;
}

function applyCardFilters(){
  populateCardData();
}

// Type management functions
function playType(key){
  if(!key) return;
  previewBasicByKey(key);
}

function editType(key){
  if(!key) return;
  document.getElementById('typeName').value = key;
  document.getElementById('typeFile').value = (manageData.basic && manageData.basic[key]) ? manageData.basic[key] : '';
}

function deleteType(key){
  if(!key) return;
  const randomName = toRandomTypeName(key);
  const cards = manageData.card || {};
  const affectedBase = Object.keys(cards).filter(cid => (cards[cid]?.type || '') === key);
  const affectedRandom = Object.keys(cards).filter(cid => (cards[cid]?.type || '') === randomName);
  const warn =
    `Delete type "${key}"?\n\n` +
    `- Cards with type "${key}" will be deleted: ${affectedBase.length}\n` +
    `- Cards with related random type "${randomName}" will be deleted: ${affectedRandom.length}\n\n` +
    `This cannot be undone.`;
  if(confirm(warn)){
    affectedBase.forEach(cid => { delete cards[cid]; });
    affectedRandom.forEach(cid => { delete cards[cid]; });
    delete manageData.basic[key];
    saveManageData();
  }
}

async function previewBasicByKey(key){
  const msg = document.getElementById('previewMsg') || document.getElementById('typeMsg');
  if(msg) msg.textContent = 'Playing basic sound...';
  try{
    const res = await fetch('/preview', {
      method:'POST',
      headers:{'Content-Type':'application/json'},
      body: JSON.stringify({ section: 'basic', key })
    });
    const j = await res.json();
    if(msg) msg.textContent = j.success ? `Playing basic: ${key}` : ('Error: '+(j.error||'unknown'));
  } catch(e){
    if(msg) msg.textContent = 'Error: ' + e.message;
  }
}

function saveType(){
  const name = document.getElementById('typeName').value.trim();
  const file = document.getElementById('typeFile').value.trim();

  if(!name || !file){
    document.getElementById('typeMsg').textContent = 'Name and file are required';
    return;
  }

  if(!manageData.basic) manageData.basic = {};
  manageData.basic[name] = file;

  saveManageData();
  document.getElementById('typeName').value = '';
  document.getElementById('typeFile').value = '';
}

// Voice management functions
function deleteVoice(key){
  if(!key) return;
  if(key === 'decade'){
    alert('Cannot delete voice "decade" (default pointer).');
    return;
  }
  const affected = Object.keys(manageData.card || {}).filter(cid => (manageData.card[cid]?.voice || '') === key);
  const warn = `Delete voice "${key}"?\n\n- Cards using this voice will be reset to "decade": ${affected.length}`;
  if(confirm(warn)){
    // reset cards
    affected.forEach(cid => { manageData.card[cid].voice = 'decade'; });
    delete manageData.voice[key];
    saveManageData();
  }
}

async function playVoiceType(key, index){
  if(!key) return;
  const msg = document.getElementById('previewMsg') || document.getElementById('voiceMsg');
  if(msg) msg.textContent = 'Playing voice...';
  try{
    const res = await fetch('/preview', {
      method:'POST',
      headers:{'Content-Type':'application/json'},
      body: JSON.stringify({ section: 'voice', key, index: index || 0 })
    });
    const j = await res.json();
    if(msg) msg.textContent = j.success ? `Playing voice: ${key}` : ('Error: '+(j.error||'unknown'));
  } catch(e){
    if(msg) msg.textContent = 'Error: ' + e.message;
  }
}

function saveVoice(){
  const name = document.getElementById('voiceName').value.trim();
  const filesStr = document.getElementById('voiceFiles').value.trim();

  if(!name || !filesStr){
    document.getElementById('voiceMsg').textContent = 'Name and files are required';
    return;
  }

  const files = filesStr.split(',').map(f => f.trim()).filter(f => f);

  if(!manageData.voice) manageData.voice = {};
  manageData.voice[name] = files;

  saveManageData();
  document.getElementById('voiceName').value = '';
  document.getElementById('voiceFiles').value = '';
}

// BMG management functions
function deleteBmg(key){
  if(!key) return;
  if(key === 'decade'){
    alert('Cannot delete BMG "decade" (default pointer).');
    return;
  }
  const affected = Object.keys(manageData.card || {}).filter(cid => (manageData.card[cid]?.bmg || '') === key);
  const warn = `Delete BMG "${key}"?\n\n- Cards using this BMG will be reset to "decade": ${affected.length}`;
  if(confirm(warn)){
    affected.forEach(cid => { manageData.card[cid].bmg = 'decade'; });
    delete manageData.bmg[key];
    saveManageData();
  }
}

async function playBmgKey(key){
  if(!key) return;
  const msg = document.getElementById('previewMsg') || document.getElementById('bmgMsg');
  if(msg) msg.textContent = 'Playing BMG...';
  try{
    const res = await fetch('/preview', {
      method:'POST',
      headers:{'Content-Type':'application/json'},
      body: JSON.stringify({ section: 'bmg', key })
    });
    const j = await res.json();
    if(msg) msg.textContent = j.success ? `Playing BMG: ${key}` : ('Error: '+(j.error||'unknown'));
  } catch(e){
    if(msg) msg.textContent = 'Error: ' + e.message;
  }
}

function saveBmg(){
  const name = document.getElementById('bmgName').value.trim();
  const file = document.getElementById('bmgFile').value.trim();

  if(!name || !file){
    document.getElementById('bmgMsg').textContent = 'Name and file are required';
    return;
  }

  if(!manageData.bmg) manageData.bmg = {};
  manageData.bmg[name] = file;

  saveManageData();
  document.getElementById('bmgName').value = '';
  document.getElementById('bmgFile').value = '';
}

// Card management functions
function editCard(key){
  if(!key) return;
  const card = (manageData.card && manageData.card[key]) ? manageData.card[key] : {};
  document.getElementById('cardId').value = key;
  document.getElementById('cardName').value = card.name || '';
  document.getElementById('cardType').value = card.type || '';
  document.getElementById('cardVoice').value = card.voice || '';
  document.getElementById('cardBmg').value = card.bmg || '';
  document.getElementById('cardFile').value = card.file || '';
}

function deleteCard(key){
  if(!key) return;
  if(confirm(`Delete card "${key}"?`)){
    delete manageData.card[key];
    saveManageData();
  }
}

async function playCardKey(key){
  if(!key) return;
  const msg = document.getElementById('previewMsg') || document.getElementById('cardMsg');
  if(msg) msg.textContent = 'Playing card...';
  try{
    const res = await fetch('/preview', {
      method:'POST',
      headers:{'Content-Type':'application/json'},
      body: JSON.stringify({ section: 'card', key })
    });
    const j = await res.json();
    if(msg) msg.textContent = j.success ? `Playing card: ${key}` : ('Error: '+(j.error||'unknown'));
  } catch(e){
    if(msg) msg.textContent = 'Error: ' + e.message;
  }
}

function saveCard(){
  const id = document.getElementById('cardId').value.trim();
  const name = document.getElementById('cardName').value.trim();
  const type = document.getElementById('cardType').value;
  const voice = document.getElementById('cardVoice').value;
  const bmg = document.getElementById('cardBmg').value;
  const file = document.getElementById('cardFile').value.trim();

  const randomType = isRandomTypeName(type);
  // Random cards don't require their own MP3/voice/BMG; they get resolved to another card at runtime.
  if(!id || (!file && !randomType)){
    document.getElementById('cardMsg').textContent = 'ID and file are required';
    return;
  }

  // Validate card id: exactly 11 chars, only 0/1
  if(id.length !== 11 || !/^[01]{11}$/.test(id)){
    document.getElementById('cardMsg').textContent = 'Card ID must be exactly 11 characters (0 and 1 only)';
    return;
  }

  if(!manageData.card) manageData.card = {};

  // Duplicate check
  if(manageData.card[id]){
    if(!confirm(`Card ID "${id}" already exists. Overwrite it?`)){
      document.getElementById('cardMsg').textContent = 'Cancelled (duplicate ID)';
      return;
    }
  }
  manageData.card[id] = { name, type, voice, bmg, file };

  saveManageData();
  document.getElementById('cardId').value = '';
  document.getElementById('cardName').value = '';
  document.getElementById('cardFile').value = '';
}

// Save management data
async function saveManageData(){
  try{
    const sortedData = sortJsonData(JSON.parse(JSON.stringify(manageData)));
    const res = await fetch('/save', {
      method:'POST',
      headers:{'Content-Type':'application/json'},
      body: JSON.stringify(sortedData, null, 2)
    });
    const j = await res.json();
    if(j.success){
      manageData = sortedData;
      loadManageData(); // Refresh the display
      document.getElementById('typeMsg').textContent = 'Saved successfully';
      document.getElementById('voiceMsg').textContent = 'Saved successfully';
      document.getElementById('bmgMsg').textContent = 'Saved successfully';
      document.getElementById('cardMsg').textContent = 'Saved successfully';
    } else {
      throw new Error(j.error || 'Unknown error');
    }
  } catch(e){
    document.getElementById('typeMsg').textContent = 'Error: ' + e.message;
    document.getElementById('voiceMsg').textContent = 'Error: ' + e.message;
    document.getElementById('bmgMsg').textContent = 'Error: ' + e.message;
    document.getElementById('cardMsg').textContent = 'Error: ' + e.message;
  }
}

// Preview play functions
async function previewBasic(){
  const key = document.getElementById('previewBasicList').value;
  if(!key) return;
  const msg = document.getElementById('previewMsg');
  msg.textContent = 'Playing basic sound...';
  try{
    const res = await fetch('/preview', {
      method:'POST',
      headers:{'Content-Type':'application/json'},
      body: JSON.stringify({ section: 'basic', key })
    });
    const j = await res.json();
    msg.textContent = j.success ? `Playing basic: ${key}` : ('Error: '+(j.error||'unknown'));
  } catch(e){
    msg.textContent = 'Error: ' + e.message;
  }
}

async function previewVoice(){
  const key = document.getElementById('previewVoiceList').value;
  if(!key) return;
  const msg = document.getElementById('previewMsg');
  msg.textContent = 'Playing voice...';
  try{
    const res = await fetch('/preview', {
      method:'POST',
      headers:{'Content-Type':'application/json'},
      body: JSON.stringify({ section: 'voice', key })
    });
    const j = await res.json();
    msg.textContent = j.success ? `Playing voice: ${key}` : ('Error: '+(j.error||'unknown'));
  } catch(e){
    msg.textContent = 'Error: ' + e.message;
  }
}

async function previewBmg(){
  const key = document.getElementById('previewBmgList').value;
  if(!key) return;
  const msg = document.getElementById('previewMsg');
  msg.textContent = 'Playing BMG...';
  try{
    const res = await fetch('/preview', {
      method:'POST',
      headers:{'Content-Type':'application/json'},
      body: JSON.stringify({ section: 'bmg', key })
    });
    const j = await res.json();
    msg.textContent = j.success ? `Playing BMG: ${key}` : ('Error: '+(j.error||'unknown'));
  } catch(e){
    msg.textContent = 'Error: ' + e.message;
  }
}

async function previewCard(){
  const key = document.getElementById('previewCardList').value;
  if(!key) return;
  const msg = document.getElementById('previewMsg');
  msg.textContent = 'Playing card main sound...';
  try{
    const res = await fetch('/preview', {
      method:'POST',
      headers:{'Content-Type':'application/json'},
      body: JSON.stringify({ section: 'card', key })
    });
    const j = await res.json();
    msg.textContent = j.success ? `Playing card: ${key}` : ('Error: '+(j.error||'unknown'));
  } catch(e){
    msg.textContent = 'Error: ' + e.message;
  }
}

async function loadJson(){
  document.getElementById('jsonMsg').textContent = 'Loading...';
  try{
    const r = await fetch('/sound.json');
    if(!r.ok) throw new Error('HTTP '+r.status);
    const t = await r.text();
    document.getElementById('jsonArea').value = t;
    document.getElementById('jsonMsg').textContent = 'Loaded';
  } catch(e){
    document.getElementById('jsonMsg').textContent = 'Error: '+e.message;
  }
}

async function saveJson(){
  document.getElementById('jsonMsg').textContent = 'Saving...';
  try{
    const res = await fetch('/save', { method:'POST', headers:{'Content-Type':'application/json'}, body: document.getElementById('jsonArea').value });
    const j = await res.json();
    document.getElementById('jsonMsg').textContent = j.success ? 'Saved' : ('Error: '+(j.error||'unknown'));
  } catch(e){
    document.getElementById('jsonMsg').textContent = 'Error: '+e.message;
  }
}

async function preset(name){
  document.getElementById('ledMsg').textContent = 'Applying...';
  try{
    const res = await fetch('/preset?color='+encodeURIComponent(name));
    const j = await res.json();
    document.getElementById('ledMsg').textContent = j.success ? ('Applied: '+name) : ('Error: '+(j.error||'unknown'));
  } catch(e){
    document.getElementById('ledMsg').textContent = 'Error: '+e.message;
  }
}

// Volume functions
document.getElementById('volumeSlider').addEventListener('input', function() {
  document.getElementById('volumeValue').textContent = this.value;
});

async function loadVolume(){
  try{
    const r = await fetch('/get_volume');
    if(!r.ok) throw new Error('HTTP '+r.status);
    const j = await r.json();
    if(j.success){
      const v = j.volume;
      const slider = document.getElementById('volumeSlider');
      const label  = document.getElementById('volumeValue');
      slider.value = v;
      label.textContent = v;
    }
  } catch(e){
    console.error('Failed to load volume:', e);
  }
}

async function setVolume(){
  const volume = document.getElementById('volumeSlider').value;
  document.getElementById('volumeMsg').textContent = 'Setting volume...';
  try{
    const res = await fetch('/set_volume', {
      method:'POST',
      headers:{'Content-Type':'application/json'},
      body: JSON.stringify({volume: parseInt(volume)})
    });
    const j = await res.json();
    document.getElementById('volumeMsg').textContent = j.success ? ('Volume set to: '+volume) : ('Error: '+(j.error||'unknown'));
  } catch(e){
    document.getElementById('volumeMsg').textContent = 'Error: '+e.message;
  }
}

// Card log functions
async function refreshLog(){
  const msg = document.getElementById('logMsg');
  const ta  = document.getElementById('logArea');
  try{
    const r = await fetch('/card_log');
    if(!r.ok) throw new Error('HTTP '+r.status);
    const data = await r.json(); // [{t,bits,id,name?},...]
    ta.value = data.map(e => {
      const label = e.name ? `${e.id} [${e.name}]` : e.id;
      return `${e.t}\t${label}\t${e.bits}`;
    }).join('\n');
    ta.scrollTop = ta.scrollHeight;
    msg.textContent = '';
  } catch(e){
    msg.textContent = 'Error loading log: '+e.message;
  }
}

async function clearLog(){
  const msg = document.getElementById('logMsg');
  try{
    const res = await fetch('/card_log_clear', { method:'POST' });
    const j = await res.json();
    if(j.success){
      document.getElementById('logArea').value = '';
      msg.textContent = 'Log cleared';
    } else {
      msg.textContent = 'Error: cannot clear log';
    }
  } catch(e){
    msg.textContent = 'Error clearing log: '+e.message;
  }
}

// Auto refresh mỗi 1s
setInterval(refreshLog, 1000);

// Show note popup once on initial load (if enabled)
loadNotePopup();
</script>
</body>
</html>
)rawliteral";

// Dedicated JSON editor page at /json
const char json_html[] PROGMEM = R"rawliteral(
<!DOCTYPE html>
<html>
<head>
<meta charset="utf-8" />
<title>sound-json editor</title>
<meta name="viewport" content="width=device-width,initial-scale=1" />
<style>
/* Pink-first theme */
:root{
  --bg:#0a0b10;
  --panel:#10121a;
  --text:#ffffff;
  --muted:#b6b8c6;
  --border:#25283a;
  --primary:#ff4fa1;
  --primary2:#ff2f88;
  --green:#31ff7a;
}
body{font-family:system-ui;background:radial-gradient(1200px 600px at 18% 0%, rgba(255,79,161,.22), transparent 60%),var(--bg);color:var(--text);margin:0;padding:18px}
.wrap{background:linear-gradient(180deg, rgba(255,79,161,.08), transparent 140px),var(--panel);border:1px solid var(--border);border-radius:14px;padding:16px}
.top{display:flex;gap:8px;align-items:center;flex-wrap:wrap;margin-bottom:10px}
button{background:linear-gradient(90deg,var(--primary),var(--primary2));color:#0b0c12;border:none;padding:8px 16px;border-radius:10px;cursor:pointer;font-weight:800}
button:hover{filter:saturate(1.1) brightness(1.05)}
.msg{font-size:13px;color:var(--muted)}
textarea{width:100%;height:78vh;background:rgba(0,0,0,.35);color:var(--green);border-radius:12px;border:1px solid var(--border);padding:12px;font-family:ui-monospace, SFMono-Regular, Menlo, Monaco, Consolas, "Liberation Mono", "Courier New", monospace}
</style>
</head>
<body> 
<div class="wrap">
  <div class="top">
    <button onclick="loadJson()">Load</button>
    <button onclick="formatJson()">Format</button>
    <button onclick="saveJson()">Save</button>
    <span class="msg" id="jsonMsg"></span>
  </div>
  <textarea id="jsonArea" spellcheck="false"></textarea>
</div>
<script>
async function loadJson(){
  const msg = document.getElementById('jsonMsg');
  msg.textContent = 'Loading...';
  try{
    const r = await fetch('/sound.json');
    const t = await r.text();
    document.getElementById('jsonArea').value = t;
    msg.textContent = 'Loaded';
  }catch(e){ msg.textContent = 'Error: ' + e.message; }
}
async function saveJson(){
  const msg = document.getElementById('jsonMsg');
  msg.textContent = 'Saving...';
  try{
    const text = document.getElementById('jsonArea').value;
    JSON.parse(text);
    const r = await fetch('/save', {
      method:'POST',
      headers:{'Content-Type':'application/json'},
      body: text
    });
    const j = await r.json();
    msg.textContent = j.success ? 'Saved' : ('Error: '+(j.error||'unknown'));
  }catch(e){ msg.textContent = 'Error: ' + e.message; }
}
function formatJson(){
  const msg = document.getElementById('jsonMsg');
  try{
    const text = document.getElementById('jsonArea').value;
    const obj = JSON.parse(text);
    document.getElementById('jsonArea').value = JSON.stringify(obj, null, 2);
    msg.textContent = 'Formatted';
  }catch(e){
    msg.textContent = 'Error: ' + e.message;
  }
}
loadJson();
</script>
</body>
</html>
)rawliteral";

void handleRoot(){
  server.send_P(200, "text/html", index_html);
}

void handleJsonPage(){
  server.send_P(200, "text/html", json_html);
}

void handleGetSound(){
  // Đọc sound.json từ filesystem để tránh giới hạn NVS/Preferences
  if(!ensureSoundFsReady()){
    server.send_P(200, "application/json", defaultSoundJson);
    return;
  }

  File f;
  if(fsBackend == FS_LITTLEFS){
    f = LittleFS.open("/sound.json", "r");
  } else if(fsBackend == FS_SPIFFS){
    f = SPIFFS.open("/sound.json", "r");
  }

  if(!f){
    server.send_P(200, "application/json", defaultSoundJson);
    return;
  }
  server.streamFile(f, "application/json");
  f.close();
}

void handleSaveSound(){
  String body = server.arg("plain");
  if(body.length()==0){
    server.send(400, "application/json", "{\"success\":false,\"error\":\"empty body\"}");
    return;
  }

  if(!ensureSoundFsReady()){
    server.send(500, "application/json", "{\"success\":false,\"error\":\"FS not ready\"}");
    return;
  }

  // Validate & load into RAM first
  DeserializationError err = deserializeJson(soundConfig, body);
  if(err){
    String msg = "JSON error: ";
    msg += err.c_str();
    String res = "{\"success\":false,\"error\":\"" + msg + "\"}";
    server.send(400, "application/json", res);
    return;
  }

  // Store JSON into LittleFS
  File f;
  if(fsBackend == FS_LITTLEFS){
    f = LittleFS.open("/sound.json", "w");
  } else if(fsBackend == FS_SPIFFS){
    f = SPIFFS.open("/sound.json", "w");
  }
  if(!f){
    server.send(500, "application/json", "{\"success\":false,\"error\":\"failed opening /sound.json for write\"}");
    return;
  }
  f.print(body);
  f.close();

  server.send(200, "application/json", "{\"success\":true}");
}

// Preview bất kỳ file trong sound.json (basic / voice / bmg / card)
void handlePreviewSound(){
  String body = server.arg("plain");
  if(body.length()==0){
    server.send(400, "application/json", "{\"success\":false,\"error\":\"empty body\"}");
    return;
  }

  DynamicJsonDocument tmp(1024);
  DeserializationError err = deserializeJson(tmp, body);
  if(err){
    String res = String("{\"success\":false,\"error\":\"") + err.c_str() + "\"}";
    server.send(400, "application/json", res);
    return;
  }

  const char* section = tmp["section"];
  const char* key     = tmp["key"];
  int index           = tmp["index"] | 0;

  if(!section || !key){
    server.send(400, "application/json", "{\"success\":false,\"error\":\"missing section/key\"}");
    return;
  }

  String secStr(section);

  if(secStr == "basic"){
    if(!soundConfig.containsKey("basic") || !soundConfig["basic"].containsKey(key)){
      server.send(404, "application/json", "{\"success\":false,\"error\":\"basic key not found\"}");
      return;
    }
    const char* file = soundConfig["basic"][key];
    if(!file){
      server.send(500, "application/json", "{\"success\":false,\"error\":\"no file for basic key\"}");
      return;
    }
    int n = fileToNum(file);
    player.playFolder(1, n);
  }
  else if(secStr == "voice"){
    if(!soundConfig.containsKey("voice") || !soundConfig["voice"].containsKey(key)){
      server.send(404, "application/json", "{\"success\":false,\"error\":\"voice key not found\"}");
      return;
    }
    JsonArray arr = soundConfig["voice"][key];
    int size = arr.size();
    if(size == 0){
      server.send(500, "application/json", "{\"success\":false,\"error\":\"voice list empty\"}");
      return;
    }
    if(index < 0 || index >= size) index = 0;
    const char* file = arr[index];
    if(!file){
      server.send(500, "application/json", "{\"success\":false,\"error\":\"voice file invalid\"}");
      return;
    }
    int n = fileToNum(file);
    player.playFolder(2, n);
  }
  else if(secStr == "bmg"){
    if(!soundConfig.containsKey("bmg") || !soundConfig["bmg"].containsKey(key)){
      server.send(404, "application/json", "{\"success\":false,\"error\":\"bmg key not found\"}");
      return;
    }
    const char* file = soundConfig["bmg"][key];
    if(!file){
      server.send(500, "application/json", "{\"success\":false,\"error\":\"no file for bmg key\"}");
      return;
    }
    int n = fileToNum(file);
    player.playFolder(3, n);
  }
  else if(secStr == "card"){
    if(!soundConfig.containsKey("card") || !soundConfig["card"].containsKey(key)){
      server.send(404, "application/json", "{\"success\":false,\"error\":\"card key not found\"}");
      return;
    }
    // If it's a random_* / random_all card, resolve to another card to get its real file.
    String resolvedCardId, resolvedType, resolvedVoice, resolvedBmg, resolvedFile;
    if(!resolveCardForPlayback(String(key), resolvedCardId, resolvedType, resolvedVoice, resolvedBmg, resolvedFile, 0)){
      server.send(500, "application/json", "{\"success\":false,\"error\":\"no resolvable main file for card\"}");
      return;
    }

    int n = fileToNum(resolvedFile.c_str());
    // Dùng luôn logic playCardRoot để hưởng LED + state MP3
    playCardRoot(n);
  }
  else{
    server.send(400, "application/json", "{\"success\":false,\"error\":\"unknown section\"}");
    return;
  }

  server.send(200, "application/json", "{\"success\":true}");
}

void handleSetColor(){
  String body = server.arg("plain");
  if(body.length()==0){ server.send(400, "application/json", "{\"success\":false,\"error\":\"empty\"}"); return; }
  DynamicJsonDocument tmp(1024);
  DeserializationError err = deserializeJson(tmp, body);
  if(err){ String res = String("{\"success\":false,\"error\":\"") + err.c_str() + "\"}"; server.send(400, "application/json", res); return; }
  int r = tmp["r"] | 0;
  int g = tmp["g"] | 0;
  int b = tmp["b"] | 0;
  applyColorAndSave((uint8_t)r,(uint8_t)g,(uint8_t)b);
  server.send(200, "application/json", "{\"success\":true}");
}

void handlePresetColor(){
  String color = server.arg("color");
  color.toLowerCase();
  if(color=="red") applyColorAndSave(255,0,0);
  else if(color=="green") applyColorAndSave(0,255,0);
  else if(color=="blue") applyColorAndSave(0,0,255);
  else if(color=="yellow") applyColorAndSave(255,255,0);
  else if(color=="magenta") applyColorAndSave(255,0,255);
  else if(color=="cyan") applyColorAndSave(0,255,255);
  else if(color=="white") applyColorAndSave(255,255,255);
  else if(color=="rgb") setModeRGBAndSave();
  else { server.send(400, "application/json", "{\"success\":false,\"error\":\"unknown preset\"}"); return; }
  server.send(200, "application/json", "{\"success\":true}");
}

// Trả về volume hiện tại cho web (để hiển thị đúng slider)
void handleGetVolume(){
  // Đọc từ Preferences, default 25 nếu chưa có
  uint16_t volume = pref.getUShort("volume", 25);
  String res = "{\"success\":true,\"volume\":";
  res += String(volume);
  res += "}";
  server.send(200, "application/json", res);
}

void handleSetVolume(){
  String body = server.arg("plain");
  if(body.length()==0){ server.send(400, "application/json", "{\"success\":false,\"error\":\"empty body\"}"); return; }

  DynamicJsonDocument tmp(1024);
  DeserializationError err = deserializeJson(tmp, body);
  if(err){ String res = String("{\"success\":false,\"error\":\"") + err.c_str() + "\"}"; server.send(400, "application/json", res); return; }

  int volume = tmp["volume"] | 25;
  volume = constrain(volume, 0, 30); // DFPlayer volume range 0-30

  player.volume(volume);
  pref.putUShort("volume", (uint16_t)volume);

  server.send(200, "application/json", "{\"success\":true}");
}

// Trả log thẻ cho web
void handleCardLog(){
  DynamicJsonDocument doc(4096);
  JsonArray arr = doc.to<JsonArray>();

  // Trả về theo thứ tự thời gian (cũ -> mới)
  for(int i = 0; i < cardLogCount; i++){
    int idx = (cardLogIndex - cardLogCount + i + CARD_LOG_SIZE) % CARD_LOG_SIZE;
    JsonObject o = arr.createNestedObject();
    o["t"]    = cardLogTime[idx];
    o["bits"] = cardLogBits[idx];
    o["id"]   = cardLogId[idx];

    // Nếu có tên trong sound.json thì trả kèm để web hiển thị label
    if(soundConfig.containsKey("card") && soundConfig["card"].containsKey(cardLogId[idx])){
      const char* nm = soundConfig["card"][cardLogId[idx]]["name"];
      if(nm){
        o["name"] = nm;
      }
    }
  }

  String out;
  serializeJson(arr, out);
  server.send(200, "application/json", out);
}

// Clear log từ web
void handleClearCardLog(){
  clearCardLog();
  server.send(200, "application/json", "{\"success\":true}");
}

//////////////////////////////////////////////////////
// SETUP
//////////////////////////////////////////////////////
void setup(){

  // Disable brownout detector to prevent resets from LiPo voltage drops
  WRITE_PERI_REG(RTC_CNTL_BROWN_OUT_REG, 0);

  Serial.begin(115200);

  pinMode(S1_PIN,INPUT);
  pinMode(S2_PIN,INPUT);
  pinMode(L1,INPUT_PULLUP);
  pinMode(L2,INPUT_PULLUP);
  pinMode(TOUCH_PIN,INPUT);  // Touch pin as digital input without pullup

  setupLED();

  // Initialize DFPlayer (không còn phụ thuộc SD của ESP32)
  Serial.println("Initializing DFPlayer...");
  // ESP32-C3 Super Mini: dùng UART1, RX=GPIO21, TX=GPIO20 để nối DFPlayer
  mySerial.begin(9600, SERIAL_8N1, 21, 20);
  delay(2000); // Wait before DFPlayer init
  player.begin(mySerial);
  delay(500); // Wait for DFPlayer to stabilize

  // Preferences (store led state, volume)
  pref.begin("decade", false);
  gNote = pref.getString("note", "Nhận mod đồ chơi liên hệ 0123456789 không nghe máy là do đang ỉa. By Bảo Đang ỈaỈa");

  // Auto-refresh Preferences after flashing new firmware:
  // If firmware build/schema changes, clear the whole namespace so config is rebuilt from defaults.
  uint32_t savedSchema = pref.getUInt("pref_schema", 0);
  String savedBuild = pref.getString("pref_build", "");
  bool needClear = (savedSchema != PREF_SCHEMA_VERSION) || (savedBuild != String(PREF_BUILD_ID));
  if(needClear){
    Serial.printf("PREF: refresh (schema %lu->%lu, build '%s'->'%s'). Clearing Preferences...\n",
                  (unsigned long)savedSchema,
                  (unsigned long)PREF_SCHEMA_VERSION,
                  savedBuild.c_str(),
                  PREF_BUILD_ID);
    pref.clear();
    pref.putUInt("pref_schema", PREF_SCHEMA_VERSION);
    pref.putString("pref_build", PREF_BUILD_ID);
  }

  // Load sound.json from filesystem into RAM
  Serial.println("Mounting sound filesystem (LittleFS/SPIFFS)...");
  if(!ensureSoundFsReady()){
    Serial.println("Error: sound FS mount failed. Fallback to defaultSoundJson in RAM.");
    deserializeJson(soundConfig, defaultSoundJson);
  } else {
    bool fileExists = false;
    if(fsBackend == FS_LITTLEFS) fileExists = LittleFS.exists("/sound.json");
    else if(fsBackend == FS_SPIFFS) fileExists = SPIFFS.exists("/sound.json");

    // If firmware build/schema changed, also refresh sound.json so defaults match new code.
    if(needClear){
      Serial.println("FS: refresh /sound.json (overwrite defaultSoundJson) due to firmware build/schema change.");
      if(fsBackend == FS_LITTLEFS){
        File f0 = LittleFS.open("/sound.json", "w");
        if(f0){ f0.print(defaultSoundJson); f0.close(); }
      } else if(fsBackend == FS_SPIFFS){
        File f0 = SPIFFS.open("/sound.json", "w");
        if(f0){ f0.print(defaultSoundJson); f0.close(); }
      }
      fileExists = true;
    }

    if(!fileExists){
      // Migration from legacy Preferences if available
      String legacySoundJson = pref.getString("sound_json", "");
      const char* fallbackJson = defaultSoundJson;
      bool legacyOk = false;
      if(legacySoundJson.length() > 0){
        DeserializationError legacyErr = deserializeJson(soundConfig, legacySoundJson);
        legacyOk = !legacyErr;
        if(!legacyOk){
          Serial.print("Legacy sound_json invalid/too large, Err=");
          Serial.println(legacyErr.c_str());
        }
      }

      // Write initial file
      String toWrite = legacyOk ? legacySoundJson : String(fallbackJson);
      if(fsBackend == FS_LITTLEFS){
        File f0 = LittleFS.open("/sound.json", "w");
        if(f0){ f0.print(toWrite); f0.close(); }
      } else if(fsBackend == FS_SPIFFS){
        File f0 = SPIFFS.open("/sound.json", "w");
        if(f0){ f0.print(toWrite); f0.close(); }
      }

      // Ensure RAM has correct data
      if(!legacyOk){
        deserializeJson(soundConfig, fallbackJson);
      }
    }

    // Now load from file
    Serial.println("Loading sound.json from sound FS...");
    File f;
    if(fsBackend == FS_LITTLEFS) f = LittleFS.open("/sound.json", "r");
    else if(fsBackend == FS_SPIFFS) f = SPIFFS.open("/sound.json", "r");

    if(!f){
      Serial.println("Error: cannot open /sound.json, fallback to defaultSoundJson in RAM...");
      deserializeJson(soundConfig, defaultSoundJson);
    } else {
      DeserializationError cfgErr = deserializeJson(soundConfig, f);
      f.close();
      if(cfgErr){
        Serial.print("Warning: failed parsing /sound.json, fallback to default. Err=");
        Serial.println(cfgErr.c_str());
        deserializeJson(soundConfig, defaultSoundJson);
      } else {
        Serial.println("sound.json loaded from sound FS");
      }
    }
  }

  // Get saved volume (default 25 lần chạy đầu tiên)
  uint16_t savedVolume = pref.getUShort("volume", 25);
  player.volume(savedVolume);
  uint16_t savedMode = pref.getUShort("led_mode", (uint16_t)MODE_RGB);
  ledMode = (savedMode == MODE_COLOR) ? MODE_COLOR : MODE_RGB;
  uint16_t r = pref.getUShort("led_r", 255);
  uint16_t g = pref.getUShort("led_g", 0);
  uint16_t b = pref.getUShort("led_b", 0);
  curR = (uint8_t)r; curG = (uint8_t)g; curB = (uint8_t)b;

  // Apply initial color/mode
  if(ledMode == MODE_COLOR) {
    ledOn = true;
    setColor(curR, curG, curB);
  } else {
    ledOn = true;
    colorIdx = 0;
    lastColor = millis();
  }

  // Start AP and web server
  startAP();

  server.on("/", HTTP_GET, handleRoot);
  server.on("/json", HTTP_GET, handleJsonPage);
  server.on("/sound.json", HTTP_GET, handleGetSound);
  server.on("/save", HTTP_POST, handleSaveSound);
  server.on("/note", HTTP_GET, handleNote);
  server.on("/preview", HTTP_POST, handlePreviewSound);
  server.on("/set_color", HTTP_POST, handleSetColor);
  server.on("/preset", HTTP_GET, handlePresetColor);
  server.on("/get_volume", HTTP_GET, handleGetVolume);
  server.on("/set_volume", HTTP_POST, handleSetVolume);
  server.on("/card_log", HTTP_GET, handleCardLog);
  server.on("/card_log_clear", HTTP_POST, handleClearCardLog);
  server.onNotFound(handleNotFound);
  server.begin();

  // Auto-play boot sound after power-on
  Serial.println("Playing boot sound..."); 
  playBase("boot_sound"); // Play boot sound from sound.json "boot_sound": "011.mp3"

  Serial.println("READY");
}

//////////////////////////////////////////////////////
// LOOP
//////////////////////////////////////////////////////
void loop(){

  //////////////////////////////////////////////////
  // SCAN ƯU TIÊN CAO NHẤT: khi đang capturing thì
  // KHÔNG làm bất cứ việc gì khác (LED, DFPlayer, web...)
  // để việc đọc cạnh IR hoàn toàn sạch, không bị trễ.
  //////////////////////////////////////////////////
  if(capturing){
    int s1=digitalRead(S1_PIN);
    int s2=digitalRead(S2_PIN);

    // Lần đầu tiên: lưu s1 hiện tại
    if(prev_s1<0){
      prev_s1=s1;
      return;
    }

    // Khi s1 đổi: lấy giá trị s2 tại thời điểm đó và thêm vào buffer
    if(s1!=prev_s1){
      // Log từng cạnh giống test/ir.txt
      Serial.printf("EDGE %02d: s1=%d s2=%d\n", countBit+1, s1, s2);

      bits += (s2 ? '1' : '0');
      countBit++;

      // Khi đã lấy đủ 13 bits
      if(countBit>=13){
        String card = bits.substring(2);  // Bỏ 2 bit đầu, lấy 11 bit

        // Lưu vào log buffer cho web
        addCardLog(bits, card);

        // Log tổng kết
        Serial.printf("SEQ DONE: bits=%s len=%d card=%s\n",
                      bits.c_str(), countBit, card.c_str());
        Serial.printf("SEQ: %s (chờ in_card phát xong)\n", card.c_str());

        pendingCardId = card;  // Lưu card ID
        
        // Nếu "in_card" đã phát xong → xử lý ngay
        if(inCardFinished){
          Serial.println("in_card đã xong, xử lý card ngay");
          handleCard(pendingCardId);
          pendingCardId="";
          playState=IDLE;
          inCardFinished=false;
        }
        
        capturing=false;
      }
      prev_s1=s1;
    }

    // Khi đang capturing thì thoát loop sớm, không xử lý gì thêm
    return;
  }

  updateLED();
  updateTypePulse();  // Update LED pulse khi phát type
  // Chỉ cho fade out chạy khi KHÔNG đọc thẻ để tránh bất kỳ ảnh hưởng nào đến timing đọc IR
  if(!capturing){
    updateLedFadeOut();  // Update LED fade out
  }
  updateMp3Led();   // Update LED khi phát MP3 file

  //////////////////////////////////////////////////
  // L1
  //////////////////////////////////////////////////
  static bool last16=HIGH;
  bool cur16=digitalRead(L1);

  if(cur16!=last16){
    delay(5);
    last16=cur16;

    if(cur16==LOW){
      playBase("in_card");
      // Khi bắt đầu "in_card":
      // - Nếu đang ở MODE_COLOR: sáng đúng màu đang set (curR, curG, curB)
      // - Nếu đang ở MODE_RGB: chọn ngẫu nhiên 1 màu trong bảng colors để sáng cố định
      ledOn = true;
      if(ledMode == MODE_COLOR){
        setColor(curR, curG, curB);
      } else {
        colorIdx = random(0, 6); // 6 màu trong bảng colors
        setColor(colors[colorIdx][0], colors[colorIdx][1], colors[colorIdx][2]);
      }

      Serial.println("=== START IR CAPTURE ===");

      capturing=true;
      bits="";
      countBit=0;
      prev_s1=-1;
      playState=WAITING_FOR_IN_CARD;  // Chờ "in_card" phát xong
      inCardFinished=false;  // Reset flag
    }
    else{
      playBase("out_card");
      
      capturing=false;
      savedFileNumber=-1;
      playState=IDLE;
      pendingCardId="";
      inCardFinished=false;  // Reset flag
      typePlayingPulse=false;  // Tắt pulse LED

      // Reset voice state when card is removed
      currentVoiceType = "";
      currentVoiceIndex = 0;
      voicePlaying = false;
      currentCardId = ""; // Reset card ID
      touchHoldStart = 0; // Reset touch timer
      bmgPlaying = false; // Reset BMG flag
      longPressTriggered = false; // Reset long press flag
      
      // Bắt đầu fade out LED thay vì tắt hẳn
      ledFadingOut=true;
      ledFadeStart=millis();
    }
  }

  //////////////////////////////////////////////////
  // L2
  //////////////////////////////////////////////////
  static bool last17=HIGH;
  bool cur17=digitalRead(L2);

  if(cur17!=last17){
    delay(5);
    last17=cur17;

    if(cur17==LOW){
      btn17Held=true;
      closePlaying=true;
      openPlaying=false;
      playBase("close");
      ledStart();  // Bật LED khi phát "close"
    }
    else{
      btn17Held=false;
      openPlaying=true;
      playBase("open");
      ledStart();  // Bật LED khi phát "open"
    }
  }

  //////////////////////////////////////////////////
  // DIGITAL TOUCH PIN 32
  //////////////////////////////////////////////////
  bool touchState = digitalRead(TOUCH_PIN); // LOW = touched, HIGH = not touched
  unsigned long nowTouch = millis();

  // Lần đầu tiên sau khi boot: chỉ khởi tạo trạng thái, không làm hành động (tránh phát touch tự động)
  if(!touchInitialized){
    touchInitialized = true;
    touchLastState = touchState;
    lastTouchChange = nowTouch;
  }
  // Xử lý khi có thay đổi trạng thái touch (debounce 100ms)
  if(touchState != touchLastState && (nowTouch - lastTouchChange > 100)){
    lastTouchChange = nowTouch;

    if(touchState == LOW){ // Touch pressed (HIGH -> LOW)
      // Trường hợp KHÔNG giữ L2: dùng để đổi màu LED tuần tự + phát touch
  if(!btn17Held){
        ledPresetIndex = (ledPresetIndex + 1) % LED_PRESET_COUNT;
        applyLedPresetByIndex(ledPresetIndex);
        // Phát âm touch nếu có cấu hình
        playBase("touch");
      }
      // Trường hợp đang giữ L2 và đã có card: 1 click = voice, double click = BMG
      else if(currentCardId.length() > 0 && !capturing){
        static unsigned long lastTouchPressForBmg = 0;
        unsigned long dt = nowTouch - lastTouchPressForBmg;

        Serial.printf("TOUCH PRESSED (L2 held): dt=%lu, voiceType='%s'\n",
                      dt, currentVoiceType.c_str());

        if(dt > 50 && dt < 400){
          // Double click: kích hoạt BMG
          Serial.println("TOUCH DOUBLE CLICK: Trigger BMG");
          playBmgFile();
          lastTouchPressForBmg = 0; // reset
        } else {
          // Single click: phát voice bình thường
          if(millis() - lastVoicePlay > 300){ // debounce giữa các lần voice
            Serial.printf("TOUCH SINGLE CLICK: Playing voice '%s'\n", currentVoiceType.c_str());
            playVoiceFile();
            lastVoicePlay = millis();
          } else {
            Serial.println("VOICE DEBOUNCED: Too soon since last voice play");
          }
          lastTouchPressForBmg = nowTouch;
        }
      }
    }
  }

  touchLastState = touchState;

  //////////////////////////////////////////////////
  // DFPLAYER STATE POLLING - Detect MP3 finish (playMp3Folder không có event riêng)
  //////////////////////////////////////////////////
  static unsigned long lastStateCheck = 0;
  static uint8_t lastPlayerState = 0;

  if(mp3PlayingFlag && (millis() - lastStateCheck > 100)){ // Check every 100ms
    uint8_t currentState = player.readState();

    // Detect khi player chuyển từ trạng thái bất kỳ sang 0 = stopped
    if(lastPlayerState != 0 && currentState == 0){
      Serial.println("MP3 finished (state polling), turning LED off");
      mp3Playing = false;
      mp3PlayingFlag = false;
      mp3StartTime = 0;
      ledOn = false;
      setColor(0, 0, 0);
    }

    lastPlayerState = currentState;
    lastStateCheck = millis();
  }

  //////////////////////////////////////////////////
  // DFPLAYER EVENT (for other file types)
  //////////////////////////////////////////////////
  if(player.available()){

    if(player.readType()==DFPlayerPlayFinished){

      // "in_card" phát xong → xử lý card nếu quét xong, nếu chưa thì mark flag
      if(playState==WAITING_FOR_IN_CARD){
        if(pendingCardId.length()){
          handleCard(pendingCardId);
          pendingCardId="";
          playState=IDLE;
          inCardFinished=false;
        } else {
          inCardFinished=true;
        }
      }
      // "close" phát xong → phát file main nếu button vẫn giữ và có card
      else if(closePlaying){
        closePlaying=false;
        typePlayingPulse=false;  // Tắt pulse

        if(btn17Held){
          if(savedFileNumber>0){
            playCardRoot(savedFileNumber);
          }
          // Nếu không có card → không phát gì
        }

        // Chỉ fade out LED khi close phát xong nếu không có MP3 đang phát
        if(!mp3Playing){
          ledFadingOut=true;
          ledFadeStart=millis();
        }
      }
      // "open" phát xong → tắt LED
      else if(openPlaying){
        openPlaying=false;

        // Fade out LED khi open phát xong
        ledFadingOut=true;
        ledFadeStart=millis();
      }
      // Voice file phát xong → reset flag
      else if(voicePlaying){
        voicePlaying=false;
        Serial.println("Voice file finished playing");
      }
      // BMG file phát xong → reset flag
      else if(bmgPlaying){
        bmgPlaying=false;
        Serial.println("BMG file finished playing");
      }
    }
  }

  //////////////////////////////////////////////////
  // WEB SERVER + AP TIMEOUT (chỉ chạy khi KHÔNG capturing)
  //////////////////////////////////////////////////
  // handle web server
  server.handleClient();

  // check AP timeout: nếu chưa có client nào kết nối trong 30s thì tắt AP
  if(apActive){
    if((millis() - apStartMillis) > AP_TIMEOUT){
      if(WiFi.softAPgetStationNum() == 0){
        stopAP();
      } else {
        // someone connected; keep AP on
        apActive = false; // don't auto-stop anymore
        Serial.println("Client detected on AP; keeping AP alive");
      }
    }
  }
}
