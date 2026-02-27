#include <Arduino.h>
#include "HardwareSerial.h"
#include "DFRobotDFPlayerMini.h"
#include <ArduinoJson.h>
#include <WiFi.h>
#include <WebServer.h>
#include <Preferences.h>

// Disable brownout detector to prevent resets
#include "soc/soc.h"
#include "soc/rtc_cntl_reg.h"

//////////////////////////////////////////////////////
// DFPLAYER (ESP32-C3: dùng UART1, ví dụ TX=GPIO21, RX=GPIO20)
//////////////////////////////////////////////////////
HardwareSerial mySerial(1);
DFRobotDFPlayerMini player;

// DynamicJsonDocument size: tăng nếu sound.json lớn
static DynamicJsonDocument soundConfig(32768);

// sound.json mặc định (dùng khi lần đầu nạp code, chưa có dữ liệu trong Preferences)
const char defaultSoundJson[] PROGMEM = R"rawliteral(
{
  "basic": {
    "in_card": "001.mp3",
    "out_card": "002.mp3",
    "open": "003.mp3",
    "close": "004.mp3",
    "error": "005.mp3",
    "kamen_ride": "006.mp3",
    "attack_ride": "007.mp3",
    "final_attack_ride": "008.mp3",
    "final_form_ride": "009.mp3",
    "no_voice": "010.mp3",
    "boot_sound": "011.mp3"
  },

  "bmg": {
    "kiva": "001.mp3",
    "decade": "012.mp3",
    "blade": "013.mp3"
  },

  "voice": {
    "decade": ["001.mp3", "002.mp3", "003.mp3", "004.mp3"],
    "kiva": ["005.mp3", "006.mp3", "007.mp3", "008.mp3"],
    "blade": ["009.mp3", "010.mp3", "011.mp3", "012.mp3"]
  },

  "card": {
    "010100101011": {
      "name": "Decade Kuuga",
      "type": "kamen_ride",
      "voice": "decade",
      "bmg": "decade",
      "file": "0001.mp3"
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
//  - LED_B   -> GPIO10
//  - DFPlayer TX -> GPIO20 (RX của ESP32-C3)
//  - DFPlayer RX -> GPIO21 (TX của ESP32-C3)
//////////////////////////////////////////////////////
#define S1_PIN 2
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
bool touchInitialized = false;           // tránh trigger no_voice lần đầu khi mới boot

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
    case 1: // green  (đã hoán đổi kênh cho đúng dây thực tế)
      applyColorAndSave(0,0,255);
      break;
    case 2: // blue   (đã hoán đổi kênh cho đúng dây thực tế)
      applyColorAndSave(0,255,0);
      break;
    case 3: // yellow
      applyColorAndSave(255,0,255);
      break;
    case 4: // magenta
      applyColorAndSave(255,255,0);
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

  const char* type=soundConfig["card"][id]["type"];
  const char* file=soundConfig["card"][id]["file"];
  const char* voice=soundConfig["card"][id]["voice"];

  if(!type || !file){
    Serial.println("Invalid card data, playing error...");
    playBase("error");
    playState=IDLE;  // Reset state sau khi phát error
    inCardFinished=false;
    return;
  }

  // Set current voice type for touch functionality
  currentVoiceType = voice ? String(voice) : "";
  currentVoiceIndex = 0; // Reset voice index when new card is loaded
  currentCardId = id;    // Store current card ID for BMG playback

  Serial.printf("CARD_VOICE: Card %s has voice type '%s'\n", id.c_str(), currentVoiceType.c_str());

  // Phát type từ base section
  playBase(type);
  typePlayingPulse=true;  // Bật pulse LED
  typePulseStart=millis();

  // Lưu file number cho button 17
  savedFileNumber=fileToNum(file);
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
body{font-family:system-ui;background:#0f1115;color:#eee;margin:0;padding:18px}
.card{background:#1a1d24;border-radius:12px;padding:16px}
.tabs{display:flex;gap:8px;margin-bottom:12px;flex-wrap:wrap;overflow-x:auto}
.tabbtn{flex:1;padding:10px;border-radius:8px;border:none;background:#2a2f3b;color:#ddd;cursor:pointer}
.tabbtn.active{background:#4b7cff;color:#fff}
textarea{width:100%;height:320px;background:#0b0d11;color:#00e676;border-radius:8px;border:1px solid #333;padding:10px;font-family:monospace}
.color-grid{display:grid;grid-template-columns:repeat(4,1fr);gap:10px;margin-top:10px}
.color-btn{height:48px;border-radius:8px;font-weight:600;border:none;cursor:pointer}
.rgb{background:linear-gradient(90deg,#ff0000,#00ff00,#0000ff);color:#000}
.msg{margin-top:8px;font-size:13px;color:#9aa3b2}
.manage-tabs{display:flex;gap:4px;margin-bottom:15px;border-bottom:1px solid #333;padding-bottom:8px}
.manage-tab-btn{flex:1;padding:8px;border-radius:6px;border:none;background:#2a2f3b;color:#ddd;cursor:pointer;font-size:12px}
.manage-tab-btn.active{background:#4b7cff;color:#fff}
.manage-subsection{background:#1a1d24;padding:15px;border-radius:8px;margin-bottom:15px}
.form-group{margin-bottom:15px}
.form-group h6{margin:10px 0 8px 0;color:#9aa3b2;font-size:14px}
.form-control{width:100%;padding:8px;border-radius:4px;border:1px solid #555;background:#2a2f3b;color:#eee;margin-bottom:8px}
.form-control:focus{outline:none;border-color:#4b7cff}
button{background:#4b7cff;color:#fff;border:none;padding:8px 16px;border-radius:4px;cursor:pointer;margin-right:8px}
button:hover{background:#3a6cff}
.data-list{max-height:200px;overflow-y:auto;border:1px solid #333;border-radius:4px;background:#0f1115;padding:8px;margin-top:10px}
.data-item{display:flex;justify-content:space-between;align-items:center;padding:6px;border-bottom:1px solid #333}
.data-item:last-child{border-bottom:none}
@media (max-width:480px){
  body{padding:12px}
  .tabs{gap:4px}
  .tabbtn{padding:8px 6px;font-size:12px}
  .color-grid{grid-template-columns:repeat(2,1fr)}
}
</style>
</head>
<body>
<h3>Driver Decade</h3>
<div class="card">
  <div class="tabs">
    <button class="tabbtn active" id="tabJson">sound.json</button>
    <button class="tabbtn" id="tabLed">LED</button>
    <button class="tabbtn" id="tabVolume">Volume</button>
    <button class="tabbtn" id="tabManage">Manage</button>
    <button class="tabbtn" id="tabPreview">Preview</button>
    <button class="tabbtn" id="tabLog">Log</button>
  </div>

  <div id="jsonSection">
    <textarea id="jsonArea"></textarea>
    <div style="margin-top:8px">
      <button onclick="loadJson()">Load</button>
      <button onclick="saveJson()">Save</button>
      <span id="jsonMsg" class="msg"></span>
    </div>
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

  <div id="manageSection" style="display:none">
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
        <h5>Type Management <small style="color:#9aa3b2">(plays from folder 01)</small></h5>
        <div class="form-group">
          <select id="typeList" class="form-control">
            <option value="">Select type to edit...</option>
          </select>
          <button onclick="editType()">Edit</button>
          <button onclick="deleteType()">Delete</button>
        </div>

        <div class="form-group">
          <h6>Add/Edit Type</h6>
          <input type="text" id="typeName" placeholder="Type name (e.g. kamen_ride)" class="form-control">
          <input type="text" id="typeFile" placeholder="File in /01/ folder (e.g. 006.mp3)" class="form-control">
          <button onclick="saveType()">Save Type</button>
          <div id="typeMsg" class="msg"></div>
        </div>

        <div class="data-list" id="typeDataList"></div>
      </div>

      <!-- Voice Management -->
      <div id="voiceSection" class="manage-subsection" style="display:none">
        <h5>Voice Management <small style="color:#9aa3b2">(plays from folder 02)</small></h5>
        <div class="form-group">
          <select id="voiceList" class="form-control">
            <option value="">Select voice to edit...</option>
          </select>
          <button onclick="editVoice()">Edit</button>
          <button onclick="deleteVoice()">Delete</button>
        </div>

        <div class="form-group">
          <h6>Add/Edit Voice</h6>
          <input type="text" id="voiceName" placeholder="Voice name (e.g. decade)" class="form-control">
          <textarea id="voiceFiles" placeholder="Files in /02/ folder (comma separated, e.g. 001.mp3,002.mp3)" class="form-control" rows="3"></textarea>
          <button onclick="saveVoice()">Save Voice</button>
          <div id="voiceMsg" class="msg"></div>
        </div>

        <div class="data-list" id="voiceDataList"></div>
      </div>

      <!-- BMG Management -->
      <div id="bmgSection" class="manage-subsection" style="display:none">
        <h5>BMG Management <small style="color:#9aa3b2">(plays from folder 03)</small></h5>
        <div class="form-group">
          <select id="bmgList" class="form-control">
            <option value="">Select BMG to edit...</option>
          </select>
          <button onclick="editBmg()">Edit</button>
          <button onclick="deleteBmg()">Delete</button>
        </div>

        <div class="form-group">
          <h6>Add/Edit BMG</h6>
          <input type="text" id="bmgName" placeholder="BMG name (e.g. special_move)" class="form-control">
          <input type="text" id="bmgFile" placeholder="File in /03/ folder (e.g. 001.mp3)" class="form-control">
          <button onclick="saveBmg()">Save BMG</button>
          <div id="bmgMsg" class="msg"></div>
        </div>

        <div class="data-list" id="bmgDataList"></div>
      </div>

      <!-- Card Management -->
      <div id="cardSection" class="manage-subsection" style="display:none">
        <h5>Card Management <small style="color:#9aa3b2">(main files from /MP3/ folder)</small></h5>
        <div class="form-group">
          <select id="cardList" class="form-control">
            <option value="">Select card to edit...</option>
          </select>
          <button onclick="editCard()">Edit</button>
          <button onclick="deleteCard()">Delete</button>
        </div>

        <div class="form-group">
          <h6>Add/Edit Card</h6>
          <input type="text" id="cardId" placeholder="Card ID (exactly 11 characters: 0 and 1 only)" class="form-control" maxlength="11">
          <input type="text" id="cardName" placeholder="Card name / label (e.g. Kuuga)" class="form-control">
          <select id="cardType" class="form-control">
            <option value="kamen_ride">kamen_ride</option>
          </select>
          <select id="cardVoice" class="form-control">
            <option value="decade">decade</option>
          </select>
          <select id="cardBmg" class="form-control">
            <option value="001.mp3">001.mp3</option>
          </select>
          <input type="text" id="cardFile" placeholder="Main file in /MP3/ folder (e.g. 0001.mp3)" class="form-control">
          <button onclick="saveCard()">Save Card</button>
          <div id="cardMsg" class="msg"></div>
        </div>

        <div class="data-list" id="cardDataList"></div>
      </div>
    </div>
  </div>

  <div id="previewSection" style="display:none">
    <h4>Preview Sounds</h4>
    <p>Chọn item trong sound.json và nhấn Play để nghe thử trực tiếp.</p>

    <div class="manage-subsection">
      <h5>Basic (folder 01)</h5>
      <div class="form-group">
        <select id="previewBasicList" class="form-control">
          <option value="">Select basic sound...</option>
        </select>
        <button onclick="previewBasic()">Play</button>
      </div>
    </div>

    <div class="manage-subsection">
      <h5>Voice (folder 02)</h5>
      <div class="form-group">
        <select id="previewVoiceList" class="form-control">
          <option value="">Select voice type...</option>
        </select>
        <button onclick="previewVoice()">Play</button>
      </div>
    </div>

    <div class="manage-subsection">
      <h5>BMG (folder 03)</h5>
      <div class="form-group">
        <select id="previewBmgList" class="form-control">
          <option value="">Select BMG...</option>
        </select>
        <button onclick="previewBmg()">Play</button>
      </div>
    </div>

    <div class="manage-subsection">
      <h5>Card Main (folder /MP3)</h5>
      <div class="form-group">
        <select id="previewCardList" class="form-control">
          <option value="">Select card...</option>
        </select>
        <button onclick="previewCard()">Play</button>
      </div>
    </div>

    <div id="previewMsg" class="msg"></div>
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

<script>
const tJson    = document.getElementById('tabJson');
const tLed     = document.getElementById('tabLed');
const tVolume  = document.getElementById('tabVolume');
const tManage  = document.getElementById('tabManage');
const tPreview = document.getElementById('tabPreview');
const tLog     = document.getElementById('tabLog');

const sJson    = document.getElementById('jsonSection');
const sLed     = document.getElementById('ledSection');
const sVolume  = document.getElementById('volumeSection');
const sManage  = document.getElementById('manageSection');
const sPreview = document.getElementById('previewSection');
const sLog     = document.getElementById('logSection');

tJson.onclick = ()=>{
  tJson.classList.add('active');
  tLed.classList.remove('active');
  tVolume.classList.remove('active');
  tManage.classList.remove('active');
  tPreview.classList.remove('active');
  tLog.classList.remove('active');
  sJson.style.display='block';
  sLed.style.display='none';
  sVolume.style.display='none';
  sManage.style.display='none';
  sPreview.style.display='none';
  sLog.style.display='none';
};

tLed.onclick = ()=>{
  tLed.classList.add('active');
  tJson.classList.remove('active');
  tVolume.classList.remove('active');
  tManage.classList.remove('active');
  tPreview.classList.remove('active');
  tLog.classList.remove('active');
  sLed.style.display='block';
  sJson.style.display='none';
  sVolume.style.display='none';
  sManage.style.display='none';
  sPreview.style.display='none';
  sLog.style.display='none';
};

tVolume.onclick = ()=>{
  tVolume.classList.add('active');
  tJson.classList.remove('active');
  tLed.classList.remove('active');
  tManage.classList.remove('active');
  tPreview.classList.remove('active');
  tLog.classList.remove('active');
  sVolume.style.display='block';
  sJson.style.display='none';
  sLed.style.display='none';
  sManage.style.display='none';
  sPreview.style.display='none';
  sLog.style.display='none';
  // Khi mở tab Volume thì load volume hiện tại từ ESP
  loadVolume();
};

// Manage tab
tManage.onclick = ()=>{
  tManage.classList.add('active');
  tJson.classList.remove('active');
  tLed.classList.remove('active');
  tVolume.classList.remove('active');
  tPreview.classList.remove('active');
  tLog.classList.remove('active');
  sManage.style.display='block';
  sJson.style.display='none';
  sLed.style.display='none';
  sVolume.style.display='none';
  sPreview.style.display='none';
  sLog.style.display='none';
  loadManageData();
};

// Preview tab
tPreview.onclick = ()=>{
  tPreview.classList.add('active');
  tJson.classList.remove('active');
  tLed.classList.remove('active');
  tVolume.classList.remove('active');
  tManage.classList.remove('active');
  tLog.classList.remove('active');
  sPreview.style.display='block';
  sJson.style.display='none';
  sLed.style.display='none';
  sVolume.style.display='none';
  sManage.style.display='none';
  sLog.style.display='none';
  loadManageData();
};

// Log tab
tLog.onclick = ()=>{
  tLog.classList.add('active');
  tJson.classList.remove('active');
  tLed.classList.remove('active');
  tVolume.classList.remove('active');
  tManage.classList.remove('active');
  tPreview.classList.remove('active');
  sLog.style.display='block';
  sJson.style.display='none';
  sLed.style.display='none';
  sVolume.style.display='none';
  sPreview.style.display='none';
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
      if(key !== 'boot_sound' && !['in_card', 'out_card', 'open', 'close', 'error', 'no_voice'].includes(key)){
        editableEntries[key] = data.basic[key];
      }
    });

    // Sort editable entries by file name
    const sortedEditable = sortByFileName(editableEntries);
    Object.assign(orderedBasic, sortedEditable);

    // Add system types back (no_voice, etc.)
    ['no_voice', 'in_card', 'out_card', 'open', 'close', 'error'].forEach(key => {
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
      const aFile = data.card[a].file;
      const bFile = data.card[b].file;

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
    populatePreviewData();
  } catch(e){
    console.error('Failed to load manage data:', e);
  }
}

function populateTypeData(){
  const list = document.getElementById('typeList');
  const dataList = document.getElementById('typeDataList');

  list.innerHTML = '<option value="">Select type to edit...</option>';
  dataList.innerHTML = '';

  if(manageData.basic){
    Object.keys(manageData.basic).forEach(key => {
      if(!['boot_sound', 'in_card', 'out_card', 'open', 'close', 'error', 'no_voice'].includes(key)){
        list.innerHTML += `<option value="${key}">${key} -> ${manageData.basic[key]} (folder 01)</option>`;
        dataList.innerHTML += `<div class="data-item"><span>${key}: ${manageData.basic[key]} (folder 01)</span></div>`;
      }
    });
  }
}

function populateVoiceData(){
  const list = document.getElementById('voiceList');
  const dataList = document.getElementById('voiceDataList');

  list.innerHTML = '<option value="">Select voice to edit...</option>';
  dataList.innerHTML = '';

  if(manageData.voice){
    Object.keys(manageData.voice).forEach(key => {
      const files = manageData.voice[key].join(', ');
      list.innerHTML += `<option value="${key}">${key} -> [${files}] (folder 02)</option>`;
      dataList.innerHTML += `<div class="data-item"><span>${key}: [${files}] (folder 02)</span></div>`;
    });
  }
}

function populateBmgData(){
  const list = document.getElementById('bmgList');
  const dataList = document.getElementById('bmgDataList');

  list.innerHTML = '<option value="">Select BMG to edit...</option>';
  dataList.innerHTML = '';

  if(manageData.bmg){
    Object.keys(manageData.bmg).forEach(key => {
      list.innerHTML += `<option value="${key}">${key} -> ${manageData.bmg[key]} (folder 03)</option>`;
      dataList.innerHTML += `<div class="data-item"><span>${key}: ${manageData.bmg[key]} (folder 03)</span></div>`;
    });
  }
}

function populateCardData(){
  const list = document.getElementById('cardList');
  const dataList = document.getElementById('cardDataList');

  list.innerHTML = '<option value="">Select card to edit...</option>';
  dataList.innerHTML = '';

  if(manageData.card){
    Object.keys(manageData.card).forEach(key => {
      const card = manageData.card[key];
      const name = card.name || '';
      const label = name ? `${key} [${name}]` : key;
      list.innerHTML += `<option value="${key}">${label} -> ${card.type}/${card.voice}/${card.file}</option>`;
      dataList.innerHTML += `<div class="data-item"><span>${label}: ${card.type} | ${card.voice} | ${card.bmg} | ${card.file}</span></div>`;
    });
  }
}

// Preview data lists
function populatePreviewData(){
  // Basic
  const basicList = document.getElementById('previewBasicList');
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
  // Populate type select (editable types only, default to kamen_ride)
  const typeSelect = document.getElementById('cardType');
  typeSelect.innerHTML = '';
  let hasKamenRide = false;
  if(manageData.basic){
    Object.keys(manageData.basic).forEach(key => {
      if(key !== 'boot_sound' && !['in_card', 'out_card', 'open', 'close', 'error'].includes(key)){
        const selected = key === 'kamen_ride' ? 'selected' : '';
        if(key === 'kamen_ride') hasKamenRide = true;
        typeSelect.innerHTML += `<option value="${key}" ${selected}>${key}</option>`;
      }
    });
  }
  if(!hasKamenRide){
    typeSelect.innerHTML = '<option value="kamen_ride" selected>kamen_ride</option>' + typeSelect.innerHTML;
  }

  // Populate voice select (default to decade)
  const voiceSelect = document.getElementById('cardVoice');
  voiceSelect.innerHTML = '';
  let hasDecade = false;
  if(manageData.voice){
    Object.keys(manageData.voice).forEach(key => {
      const selected = key === 'decade' ? 'selected' : '';
      if(key === 'decade') hasDecade = true;
      voiceSelect.innerHTML += `<option value="${key}" ${selected}>${key}</option>`;
    });
  }
  if(!hasDecade){
    voiceSelect.innerHTML = '<option value="decade" selected>decade</option>' + voiceSelect.innerHTML;
  }

  // Populate BMG select (default to first available)
  const bmgSelect = document.getElementById('cardBmg');
  bmgSelect.innerHTML = '';
  if(manageData.bmg && Object.keys(manageData.bmg).length > 0){
    Object.keys(manageData.bmg).forEach(key => {
      bmgSelect.innerHTML += `<option value="${key}">${key}</option>`;
    });
  } else {
    bmgSelect.innerHTML = '<option value="001.mp3" selected>001.mp3</option>';
  }
}

// Type management functions
function editType(){
  const selected = document.getElementById('typeList').value;
  if(!selected) return;

  document.getElementById('typeName').value = selected;
  document.getElementById('typeFile').value = manageData.basic[selected];
}

function deleteType(){
  const selected = document.getElementById('typeList').value;
  if(!selected) return;

  if(confirm(`Delete type "${selected}"?`)){
    delete manageData.basic[selected];
    saveManageData();
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
function editVoice(){
  const selected = document.getElementById('voiceList').value;
  if(!selected) return;

  document.getElementById('voiceName').value = selected;
  document.getElementById('voiceFiles').value = manageData.voice[selected].join(',');
}

function deleteVoice(){
  const selected = document.getElementById('voiceList').value;
  if(!selected) return;

  if(confirm(`Delete voice "${selected}"?`)){
    delete manageData.voice[selected];
    saveManageData();
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
function editBmg(){
  const selected = document.getElementById('bmgList').value;
  if(!selected) return;

  document.getElementById('bmgName').value = selected;
  document.getElementById('bmgFile').value = manageData.bmg[selected];
}

function deleteBmg(){
  const selected = document.getElementById('bmgList').value;
  if(!selected) return;

  if(confirm(`Delete BMG "${selected}"?`)){
    delete manageData.bmg[selected];
    saveManageData();
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
function editCard(){
  const selected = document.getElementById('cardList').value;
  if(!selected) return;

  const card = manageData.card[selected];
  document.getElementById('cardId').value = selected;
  document.getElementById('cardName').value = card.name || '';
  document.getElementById('cardType').value = card.type;
  document.getElementById('cardVoice').value = card.voice;
  document.getElementById('cardBmg').value = card.bmg;
  document.getElementById('cardFile').value = card.file;
}

function deleteCard(){
  const selected = document.getElementById('cardList').value;
  if(!selected) return;

  if(confirm(`Delete card "${selected}"?`)){
    delete manageData.card[selected];
    saveManageData();
  }
}

function saveCard(){
  const id = document.getElementById('cardId').value.trim();
  const name = document.getElementById('cardName').value.trim();
  const type = document.getElementById('cardType').value;
  const voice = document.getElementById('cardVoice').value;
  const bmg = document.getElementById('cardBmg').value;
  const file = document.getElementById('cardFile').value.trim();

  if(!id || !file){
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
</script>
</body>
</html>
)rawliteral";

void handleRoot(){
  server.send_P(200, "text/html", index_html);
}

void handleGetSound(){
  // Đọc sound.json từ Preferences thay vì SD
  String content = pref.getString("sound_json", "");
  if(content.length() == 0){
    // Nếu chưa có dữ liệu, trả về JSON rỗng hợp lệ để web có thể thao tác
    content = "{}";
  }
  server.send(200, "application/json", content);
}

void handleSaveSound(){
  String body = server.arg("plain");
  if(body.length()==0){
    server.send(400, "application/json", "{\"success\":false,\"error\":\"empty body\"}");
    return;
  }

  // Validate JSON
  DynamicJsonDocument tmp(32768);
  DeserializationError err = deserializeJson(tmp, body);
  if(err){
    String msg = "JSON error: ";
    msg += err.c_str();
    String res = "{\"success\":false,\"error\":\"" + msg + "\"}";
    server.send(400, "application/json", res);
    return;
  }

  // Lưu JSON thô vào Preferences và cập nhật cấu hình trong RAM
  pref.putString("sound_json", body);
  deserializeJson(soundConfig, body);

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
    const char* file = soundConfig["card"][key]["file"];
    if(!file){
      server.send(500, "application/json", "{\"success\":false,\"error\":\"no main file for card\"}");
      return;
    }
    int n = fileToNum(file);
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
  else if(color=="green") applyColorAndSave(0,0,255);
  else if(color=="blue") applyColorAndSave(0,255,0);
  else if(color=="yellow") applyColorAndSave(255,0,255);
  else if(color=="magenta") applyColorAndSave(255,255,0);
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
  // ESP32-C3 Super Mini: dùng UART1, RX=GPIO20, TX=GPIO21 để nối DFPlayer
  mySerial.begin(9600, SERIAL_8N1, 20, 21);
  delay(300); // Wait before DFPlayer init
  player.begin(mySerial);
  delay(500); // Wait for DFPlayer to stabilize

  // Preferences (store led state, volume và sound.json)
  pref.begin("decade", false);

  // Load sound.json từ Preferences vào RAM
  Serial.println("Loading sound.json from Preferences...");
  String soundJson = pref.getString("sound_json", "");
  if(soundJson.length() > 0){
    DeserializationError cfgErr = deserializeJson(soundConfig, soundJson);
    if(cfgErr){
      Serial.print("Warning: failed parsing sound_json from Preferences: ");
      Serial.println(cfgErr.c_str());
    } else {
      Serial.println("sound.json loaded from Preferences");
    }
  } else {
    Serial.println("No sound_json found in Preferences, using built-in defaultSoundJson");
    soundJson = String(defaultSoundJson);
    DeserializationError cfgErr = deserializeJson(soundConfig, soundJson);
    if(cfgErr){
      Serial.print("Error: failed parsing built-in defaultSoundJson: ");
      Serial.println(cfgErr.c_str());
    } else {
      // Lưu lại vào Preferences để những lần sau đọc ra
      pref.putString("sound_json", soundJson);
      Serial.println("Default sound.json saved to Preferences");
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
  server.on("/sound.json", HTTP_GET, handleGetSound);
  server.on("/save", HTTP_POST, handleSaveSound);
  server.on("/preview", HTTP_POST, handlePreviewSound);
  server.on("/set_color", HTTP_POST, handleSetColor);
  server.on("/preset", HTTP_GET, handlePresetColor);
  server.on("/get_volume", HTTP_GET, handleGetVolume);
  server.on("/set_volume", HTTP_POST, handleSetVolume);
  server.on("/card_log", HTTP_GET, handleCardLog);
  server.on("/card_log_clear", HTTP_POST, handleClearCardLog);
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

  // Lần đầu tiên sau khi boot: chỉ khởi tạo trạng thái, không làm hành động (tránh phát no_voice tự động)
  if(!touchInitialized){
    touchInitialized = true;
    touchLastState = touchState;
    lastTouchChange = nowTouch;
  }
  // Xử lý khi có thay đổi trạng thái touch (debounce 100ms)
  if(touchState != touchLastState && (nowTouch - lastTouchChange > 100)){
    lastTouchChange = nowTouch;

    if(touchState == LOW){ // Touch pressed (HIGH -> LOW)
      // Trường hợp KHÔNG giữ L2: dùng để đổi màu LED tuần tự + phát no_voice
      if(!btn17Held){
        ledPresetIndex = (ledPresetIndex + 1) % LED_PRESET_COUNT;
        applyLedPresetByIndex(ledPresetIndex);
        // Phát âm báo no_voice nếu có cấu hình
        playBase("no_voice");
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
