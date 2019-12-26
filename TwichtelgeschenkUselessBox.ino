#include <Adafruit_NeoPixel.h>
#include <ESP8266WiFi.h>
#include <Wire.h>
#include <Arduino.h>
#include <Servo.h>
#include <Ticker.h>
#include <ESP8266HTTPClient.h>

unsigned long ulReqcount;
unsigned long ulReconncount;

// WiFi settings
const char* ssid = "vspace.one";
const char* password = "12345678";

extern "C" {
#include <user_interface.h>
}

// Notes
#define NOTE_B0  31
#define NOTE_C1  33
#define NOTE_CS1 35
#define NOTE_D1  37
#define NOTE_DS1 39
#define NOTE_E1  41
#define NOTE_F1  44
#define NOTE_FS1 46
#define NOTE_G1  49
#define NOTE_GS1 52
#define NOTE_A1  55
#define NOTE_AS1 58
#define NOTE_B1  62
#define NOTE_C2  65
#define NOTE_CS2 69
#define NOTE_D2  73
#define NOTE_DS2 78
#define NOTE_E2  82
#define NOTE_F2  87
#define NOTE_FS2 93
#define NOTE_G2  98
#define NOTE_GS2 104
#define NOTE_A2  110
#define NOTE_AS2 117
#define NOTE_B2  123
#define NOTE_C3  131
#define NOTE_CS3 139
#define NOTE_D3  147
#define NOTE_DS3 156
#define NOTE_E3  165
#define NOTE_F3  175
#define NOTE_FS3 185
#define NOTE_G3  196
#define NOTE_GS3 208
#define NOTE_A3  220
#define NOTE_AS3 233
#define NOTE_B3  247
#define NOTE_C4  262
#define NOTE_CS4 277
#define NOTE_D4  294
#define NOTE_DS4 311
#define NOTE_E4  330
#define NOTE_F4  349
#define NOTE_FS4 370
#define NOTE_G4  392
#define NOTE_GS4 415
#define NOTE_A4  440
#define NOTE_AS4 466
#define NOTE_B4  494
#define NOTE_C5  523
#define NOTE_CS5 554
#define NOTE_D5  587
#define NOTE_DS5 622
#define NOTE_E5  659
#define NOTE_F5  698
#define NOTE_FS5 740
#define NOTE_G5  784
#define NOTE_GS5 831
#define NOTE_A5  880
#define NOTE_AS5 932
#define NOTE_B5  988
#define NOTE_C6  1047
#define NOTE_CS6 1109
#define NOTE_D6  1175
#define NOTE_DS6 1245
#define NOTE_E6  1319
#define NOTE_F6  1397
#define NOTE_FS6 1480
#define NOTE_G6  1568
#define NOTE_GS6 1661
#define NOTE_A6  1760
#define NOTE_AS6 1865
#define NOTE_B6  1976
#define NOTE_C7  2093
#define NOTE_CS7 2217
#define NOTE_D7  2349
#define NOTE_DS7 2489
#define NOTE_E7  2637
#define NOTE_F7  2794
#define NOTE_FS7 2960
#define NOTE_G7  3136
#define NOTE_GS7 3322
#define NOTE_A7  3520
#define NOTE_AS7 3729
#define NOTE_B7  3951
#define NOTE_C8  4186
#define NOTE_CS8 4435
#define NOTE_D8  4699
#define NOTE_DS8 4978

#define PLAYSPEED        133  //= 15000/113 BPM
#define TONEDURATION     0.9  //Add little gap between the notes
#define LED_COUNT        1    //Only one Neopixel is used

//Hardware pins
#define NEOPIXELPIN       0
#define SERVO_POWER       2
#define SERVO_CONTROL     4
#define SPEAKER           5
#define MULTICOLOUR_R    12
#define MULTICOLOUR_G    13
#define MULTICOLOUR_B    14
#define MULTICOLOUR_TONE 15
#define CURRENTLY_UNUSED 16

//Machinestates
#define STATE_STARTUP     0
#define STATE_DEEPSLEEP   1
#define STATE_PLAY        2
#define STATE_TURN_OFF1   3
#define STATE_GO_SLEEP1   4
#define STATE_SLEEP1      5
#define STATE_TURN_OFF2   6
#define STATE_GO_SLEEP2   7
#define STATE_SLEEP2      8
#define STATE_TURN_OFF    9
#define STATE_KEEP_OFF   10
#define STATE_GO_SLEEP   11
#define STATE_SLEEP      12

//Variables
uint8_t state = STATE_STARTUP;
uint8_t state_old = -1;
uint8_t timeout = 23;
bool switch_state = false;
int8_t connection = -1;

//Config servo and neopixel
Servo myservo;
Adafruit_NeoPixel strip(LED_COUNT, NEOPIXELPIN, NEO_GRBW + NEO_KHZ800);

//Config timer interrupts
Ticker Tick;
Ticker Music;
Ticker RgbCycle;

//Send message to telegram via telegramiotbot.com
bool telegram(String Message = "", String Token = "59c3dc1a8b797") {
  int c;
  String Send = "";
  for (int i = 0; i < Message.length(); i++) {
    c = Message.charAt(i);
    if ( ('a' <= c && c <= 'z')
         || ('A' <= c && c <= 'Z')
         || ('0' <= c && c <= '9') ) {
      Send += char(c);
    } else {
      if (c <= 0x0F) {
        Send += "%0" + String(c, HEX);
      } else {
        Send += "%" + String(c, HEX);
      }
    }
  }

  HTTPClient http;
  http.begin("http://telegramiotbot.com/api/notify?token=" + Token + "&message=" + Send);
  int httpCode = http.GET();
  return httpCode != HTTP_CODE_OK;
}

//Every 50 ms; 20x per second
void tick() {
  if (timeout != 0) {
    timeout--;
  }
}

//Rickroll
int8_t music = 0;
void musicplayer() { // Plays music in 1/16 Notes
  if (music > 0 ) {
    switch (music) {
      case  1: tone(SPEAKER, NOTE_C4,  6 * PLAYSPEED * TONEDURATION); break;
      case  7: tone(SPEAKER, NOTE_D4,  6 * PLAYSPEED * TONEDURATION); break;
      case 13: tone(SPEAKER, NOTE_G3,  4 * PLAYSPEED * TONEDURATION); break;
      case 17: tone(SPEAKER, NOTE_D4,  6 * PLAYSPEED * TONEDURATION); break;
      case 23: tone(SPEAKER, NOTE_E4,  6 * PLAYSPEED * TONEDURATION); break;
      case 29: tone(SPEAKER, NOTE_G4,  1 * PLAYSPEED * TONEDURATION); break;
      case 30: tone(SPEAKER, NOTE_F4,  1 * PLAYSPEED * TONEDURATION); break;
      case 31: tone(SPEAKER, NOTE_E4,  1 * PLAYSPEED * TONEDURATION); break;
      case 32: tone(SPEAKER, NOTE_C4,  7 * PLAYSPEED * TONEDURATION); break;
      case 39: tone(SPEAKER, NOTE_D4,  6 * PLAYSPEED * TONEDURATION); break;
      case 45: tone(SPEAKER, NOTE_G3, 10 * PLAYSPEED * TONEDURATION); break;
      //         55                PAUSE     4
      case 59: tone(SPEAKER, NOTE_G3,  1 * PLAYSPEED * TONEDURATION); break;
      case 60: tone(SPEAKER, NOTE_G3,  1 * PLAYSPEED * TONEDURATION); break;
      case 61: tone(SPEAKER, NOTE_A3,  1 * PLAYSPEED * TONEDURATION); break;
      case 62: tone(SPEAKER, NOTE_C4,  1 * PLAYSPEED * TONEDURATION); break;
      case 63: tone(SPEAKER, NOTE_A3,  1 * PLAYSPEED * TONEDURATION); break;
      case 64: tone(SPEAKER, NOTE_C4,  1 * PLAYSPEED * TONEDURATION); break;
      case 70: music = -2; break;
    }
    music++;
  }
}

//Fading Neopixel
uint16_t fading = 0;
void rgbcycle() {
  if (fading != 0) {
    uint16_t pos = fading / 256;
    uint16_t fade = fading % 256;
    uint8_t r, g, b;
    switch (pos) {
      case 0:
        r = fade;
        g = 0;
        b = 0;
        break;
      case 1:
        r = 255;
        g = fade;
        b = 0;
        break;
      case 2:
        r = 255 - fade;
        g = 255;
        b = 0;
        break;
      case 3:
        r = 0;
        g = 255;
        b = fade;
        break;
      case 4:
        r = 0;
        g = 255 - fade;
        b = 255;
        break;
      case 5:
        r = fade;
        g = 0;
        b = 255;
        break;
      case 6:
        r = 255;
        g = 0;
        b = 255 - fade;
        break;
      case 7:
        fading = 255;
        break;
    }
    fading ++;
    strip.setPixelColor(0, strip.Color(r, g, b));
    strip.show();
  }
}

//Setup WLAN
void WiFiStart() {
  ulReconncount++;

  // Connect to WiFi network
  Serial.print("Connecting to ");
  Serial.println(ssid);

  WiFi.hostname("UselessBox");
  WiFi.begin(ssid, password);
  while (WiFi.status() != WL_CONNECTED) {
    delay(100);
      Serial.print(".");
  }
//  WiFi.config(IPAddress(192, 168, 2, 81), IPAddress(192, 168, 2, 1), IPAddress(255, 255, 255, 0), IPAddress(192, 168, 2, 1));

  Serial.println("WiFi connected");

  telegram("UselessBox:\nWiFi connected!\nTry: " + String(ulReconncount));

  // Print the IP address
  Serial.println(WiFi.localIP());
}

//Startup
void setup() {
  // put your setup code here, to run once:
  Serial.begin(115200);
  Serial.print("Started");
  
  pinMode(MULTICOLOUR_TONE, OUTPUT);
  pinMode(MULTICOLOUR_R, OUTPUT);
  pinMode(MULTICOLOUR_G, OUTPUT);
  pinMode(MULTICOLOUR_B, OUTPUT);
  pinMode(SPEAKER, OUTPUT);
  pinMode(SERVO_POWER, OUTPUT);
  digitalWrite(SERVO_POWER, HIGH);

  strip.setBrightness(255); // Set BRIGHTNESS to about 1/5 (max = 255)
  strip.begin();
  strip.show();

  WiFi.mode(WIFI_STA);


  Music.attach_ms(PLAYSPEED, musicplayer);
  Tick.attach_ms(50, tick);
  RgbCycle.attach_ms(2, rgbcycle);
  
  digitalWrite(MULTICOLOUR_R, 1);
  digitalWrite(MULTICOLOUR_G, 0);
  digitalWrite(MULTICOLOUR_B, 0);
  strip.setPixelColor(0, strip.Color(0, 0, 0));
  strip.show();
  myservo.attach(SERVO_CONTROL);
  digitalWrite(SERVO_POWER, LOW);
  myservo.write(50);
}

//Main loop
void loop() {
  // put your main code here, to run repeatedly:
  if (WiFi.status() != WL_CONNECTED){//Restart WLAN connection if needed
//    WiFiStart();
  }
  if (analogRead(A0) < 512) {//read  switch state
    if (switch_state == false){//detect change
      switch_state = true;
      telegram("Switched on!");
    }
    if (state == STATE_STARTUP or state == STATE_DEEPSLEEP) {
      state = STATE_PLAY;
      fading = 0;
      music = 1;
      digitalWrite(MULTICOLOUR_R, 0);
      digitalWrite(MULTICOLOUR_G, 1);
      digitalWrite(MULTICOLOUR_B, 0);
      strip.setPixelColor(0, strip.Color(0, 0, 0, 255));
      strip.show();
    } else if (music <= 0 and state != STATE_TURN_OFF1 and state != STATE_TURN_OFF2 and state != STATE_TURN_OFF) {
      if (state == STATE_GO_SLEEP or state == STATE_SLEEP or state == STATE_GO_SLEEP2 or state == STATE_SLEEP2 or state == STATE_KEEP_OFF){
        state = STATE_TURN_OFF;
      } else if (state == STATE_GO_SLEEP1 or state == STATE_SLEEP1){
        state = STATE_TURN_OFF2;
      } else {
        state = STATE_TURN_OFF1;
      }
      fading = 0;
      myservo.attach(SERVO_CONTROL);
      digitalWrite(SERVO_POWER, LOW);
      myservo.write(180);
      digitalWrite(MULTICOLOUR_R, 0);
      digitalWrite(MULTICOLOUR_G, 0);
      digitalWrite(MULTICOLOUR_B, 1);
      strip.setPixelColor(0, strip.Color(0, 0, 0, 255));
      strip.show();      
    }
    timeout = 73;
  } else {
    if (switch_state == true){//detect change
      switch_state = false;
      telegram("Switched off!");
    }
    
    if (fading == 0) {
      fading = 1;
    }
    
    if (state == STATE_PLAY or state == STATE_TURN_OFF1 or state == STATE_TURN_OFF2 or (state == STATE_KEEP_OFF and timeout == 0)){
      if (state == STATE_PLAY){
        state = STATE_DEEPSLEEP;
        music = 0;
        noTone(SPEAKER);
      } else if (state == STATE_TURN_OFF1) {
        state = STATE_GO_SLEEP1;
      } else if (state == STATE_TURN_OFF2) {
        state = STATE_GO_SLEEP2;
      } else {
        state = STATE_GO_SLEEP;
      }
      digitalWrite(MULTICOLOUR_R, 1);
      digitalWrite(MULTICOLOUR_G, 0);
      digitalWrite(MULTICOLOUR_B, 0);
      strip.setPixelColor(0, strip.Color(0, 0, 0));
      myservo.write(50);
      strip.show();
      timeout = 23;
    }

    if (state == STATE_TURN_OFF){
      state = STATE_KEEP_OFF;
      digitalWrite(MULTICOLOUR_R, 1);
      digitalWrite(MULTICOLOUR_G, 1);
      digitalWrite(MULTICOLOUR_B, 0);
      myservo.attach(SERVO_CONTROL);
      digitalWrite(SERVO_POWER, LOW);
      myservo.write(120);
      timeout = 42;
    }

    if (timeout == 0){
      if (state == STATE_STARTUP){
        state = STATE_DEEPSLEEP;
        myservo.detach();
        digitalWrite(SERVO_POWER, HIGH);
      } else if (state == STATE_GO_SLEEP1 or state == STATE_GO_SLEEP2 or state == STATE_GO_SLEEP) {
        if (state == STATE_GO_SLEEP1){
          state = STATE_SLEEP1;
        } else if (state == STATE_GO_SLEEP2){
          state = STATE_SLEEP2;
        } else {
          state = STATE_SLEEP;
        }
        myservo.detach();
        digitalWrite(SERVO_POWER, HIGH);
        timeout = 73;
      } else if(state == STATE_SLEEP1 or state == STATE_SLEEP2 or state == STATE_SLEEP) {
        state = STATE_DEEPSLEEP;
      }
    }
  }
}
