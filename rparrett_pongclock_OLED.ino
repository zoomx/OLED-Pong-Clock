/*
    rparrett_pongclock_OLED
    a porting to OLED of the pongclock by Rob Parrett
    http://robparrett.com/pongclock
    https://github.com/rparrett/pongclock

*/


#include <Bounce2.h>		//debounce library https://github.com/thomasfredericks/Bounce2
#include <SPI.h>
#include <Wire.h>
#include <EEPROM.h>
#include "RTClib.h" // https://github.com/adafruit/RTClib
#include <Adafruit_GFX.h>    // https://github.com/adafruit/Adafruit-ST7735-Library
//#include <Adafruit_ST7735.h> //
#include <Adafruit_SSD1306.h>   // Use OLED instead of TFT pin

#define DS1307_I2C_ADDRESS 0x68

#define PONGCLOCK "pck"

struct {
  char pongclock[4];
  byte ledpwm;
} settings = {
  PONGCLOCK,
  0
};

int8_t ledpwm_steps[4] = { 255, 128, 64, 16 };

/*
  #define cs   10
  #define dc   9
  #define rst  8
*/

//Adafruit_ST7735 display = Adafruit_ST7735(cs, dc, rst);
#define OLED_RESET 4
Adafruit_SSD1306 display(OLED_RESET);

const int sqw = 2;	//get the square wave from DS1307
//const int ledbtn = 5; //4; OLED doesn't have led
const int minutebtn = 6;
const int hourbtn = 7;

const int led = 13; //5;

Bounce minutebouncer = Bounce(minutebtn, 20);
Bounce hourbouncer = Bounce(hourbtn, 20);
//Bounce ledbouncer = Bounce(ledbtn, 20);

RTC_DS1307 RTC;

const int16_t h = 64; //128;
const int16_t w = 128; //160;

int pause = 0;

const int16_t paddle_h = 20;
const int16_t paddle_w = 2;

const int16_t lpaddle_x = 0;
const int16_t rpaddle_x = w - paddle_w;

int16_t lpaddle_y = 0;
int16_t rpaddle_y = h - paddle_h;

int16_t lpaddle_d = 1;
int16_t rpaddle_d = -1;

const int16_t lpaddle_ball_t = w - w / 4;
const int16_t rpaddle_ball_t = w / 4;

int16_t target_y = 0;

int16_t ball_x = 2;
int16_t ball_y = 2;
int16_t ball_dx = 1;
int16_t ball_dy = 1;
const int16_t ball_w = 3;
const int16_t ball_h = 3;

const int16_t dashline_h = 4;
const int16_t dashline_w = 2;
const int16_t dashline_n = h / dashline_h;
const int16_t dashline_x = w / 2 - 1;
const int16_t dashline_y = dashline_h / 2;

int16_t lscore = 12;
int16_t rscore = 4;

byte newminute = 0;
byte newhour = 0;

int16_t hour = 12;
int16_t minute = 4;
volatile int16_t second = 0;

const int16_t font_s  = 2;
const int16_t font_w  = 5 * font_s;
const int16_t font_h  = 7 * font_s;
const int16_t font_sp = 2 * font_s;

const uint16_t fgcolor = WHITE; //ST7735_GREEN;
const uint16_t bgcolor = BLACK; //ST7735_BLACK;


//*****************************************
void sqwint(void) {
  second = second + 1;
  //Serial.println(second);
}
//*****************************************

void setup(void) {
  randomSeed(analogRead(0));

  //Wire.begin();
  Serial.begin(115200);
  Serial.println(F("Pong Clock rparret for OLED"));
  initsettings();
  initdisplay();
  initclock();
  initgame();

  //Serial.println(F("End inits"));

  pinMode(minutebtn, INPUT_PULLUP);
  //digitalWrite(minutebtn, HIGH);
  pinMode(hourbtn, INPUT_PULLUP);
  //digitalWrite(hourbtn, HIGH);
  //pinMode(ledbtn, INPUT_PULLUP);
  //digitalWrite(ledbtn, HIGH);
  Serial.println(F("End setup"));
}
//*****************************************
void savesettings() {
  for (unsigned int t = 0; t < sizeof(settings); t++) {
    EEPROM.write(t, *((char*)&settings + t));
  }
}
//*****************************************
void initsettings() {
  if (EEPROM.read(0) == PONGCLOCK[0] &&
      EEPROM.read(1) == PONGCLOCK[1] &&
      EEPROM.read(2) == PONGCLOCK[2] &&
      EEPROM.read(3) == PONGCLOCK[3])
  {
    for (unsigned int t = 0; t < sizeof(settings); t++) {
      *((char*)&settings + t) = EEPROM.read(t);
    }
  } else {
    savesettings();
  }
}
//*****************************************
void initdisplay() {
  /*
    display.initR(INITR_REDTAB);

    display.fillScreen(ST7735_BLACK);
    display.setRotation(3);
    display.setTextSize(font_s);
  */

  display.begin(SSD1306_SWITCHCAPVCC, 0x3C);
  display.clearDisplay();
  display.fillRect(0, 0, 128, 64, BLACK);      //Blanks out last frame to black
  display.setTextColor(WHITE);
  display.setTextSize(2);
  display.display();
  //display.print("Pong Clock");
  //display.display();
  analogWrite(led, ledpwm_steps[settings.ledpwm]);
  Serial.println(F("End initdisplay"));
}
//*****************************************
void initclock() {
  /*
    hour   = 10;
    minute = 30;
    second = 0;
    return;
  */

  //Enable Square Wave Output
  Wire.beginTransmission(DS1307_I2C_ADDRESS);
  Wire.write(0x07); // move pointer to SQW address
  Wire.write(0x10); // sends 0x10 (hex) 00010000 (binary)
  Wire.endTransmission();


  if (! RTC.isrunning()) {
    RTC.adjust(DateTime(__DATE__, __TIME__));
  }

  RTC.begin();

  DateTime now = RTC.now();

  hour   = now.hour();
  minute = now.minute();
  second = now.second();

  Serial.print(hour);
  Serial.print(F(" "));
  Serial.println(minute);

  if (hour > 12) hour = hour - 12;

  attachInterrupt(0, sqwint, FALLING);
  pinMode(sqw, INPUT_PULLUP);
  //digitalWrite(sqw, HIGH);
}
//*****************************************
void initgame() {
  lpaddle_y = random(0, h - paddle_h);
  rpaddle_y = random(0, h - paddle_h);

  // ball is placed on the center of the left paddle
  ball_y = lpaddle_y + (paddle_h / 2);

  lscore = hour;
  rscore = minute;

  calc_target_y();
}
//*****************************************
void loop() {
  keeptime();
  lpaddle();
  rpaddle();
  ball();
  midline();
  score(fgcolor);
  display.display();
  buttons();
}
//*****************************************
void buttons() {
  minutebouncer.update();
  if (minutebouncer.fallingEdge()) {
    minute = minute + 1;
    if (minute > 59) minute = 0;
    second = 0;
    newminute = 1;
    changescore(false);
    setclock();
  }

  hourbouncer.update();
  if (hourbouncer.fallingEdge()) {
    hour = hour + 1;
    if (hour > 12) hour = 1;
    newhour = 1;
    changescore(false);
    setclock();
  }

  /*
    ledbouncer.update();
    if (ledbouncer.fallingEdge()) {
      settings.ledpwm++;
      if (settings.ledpwm > sizeof(ledpwm_steps) - 1) {
        settings.ledpwm = 0;
      }

      analogWrite(led, ledpwm_steps[settings.ledpwm]);

      savesettings();
    }
  */
}
//*****************************************
void setclock() {
  DateTime now = RTC.now();
  DateTime updated = DateTime(now.year(), now.month(), now.day(), hour, minute, now.second());
  RTC.adjust(updated);
  RTC.begin();
}
//*****************************************
void keeptime() {
  if (second > 59) {
    newminute = 1;
    second = 0;
    minute = minute + 1;
  }

  if (minute > 59) {
    newminute = 0;
    newhour = 1;
    minute = 0;
    hour = hour + 1;
  }

  if (hour > 12) {
    hour = 1;
  }
}
//*****************************************
void midline() {
  for (int16_t i = 0; i < dashline_n; i = i + 2) {
    display.fillRect(dashline_x, dashline_y + i * dashline_h, dashline_w, dashline_h, fgcolor);
  }

  for (int16_t i = 1; i < dashline_n; i = i + 2) {
    display.fillRect(dashline_x, dashline_y + i * dashline_h, dashline_w, dashline_h, bgcolor);
  }
}
//*****************************************
void score(int16_t color) {
  int lscore_x, rscore_x;
  if (lscore > 9) lscore_x = w / 2 - font_w - font_w - font_sp - 10;
  else lscore_x = w / 2 - font_w - 10;
  rscore_x = w / 2 + 10;
  display.setTextColor(color);
  display.setCursor(lscore_x, dashline_y);
  display.println(lscore);
  display.setCursor(rscore_x, dashline_y);
  display.println(rscore);
}
//*****************************************
void lpaddle() {
  if (pause > 0) return;

  if (lpaddle_d == 1) {
    display.fillRect(lpaddle_x, lpaddle_y, paddle_w, 1, bgcolor);
  }
  else if (lpaddle_d == -1) {
    display.fillRect(lpaddle_x, lpaddle_y + paddle_h - 1, paddle_w, 1, bgcolor);
  }

  lpaddle_y = lpaddle_y + lpaddle_d;

  if (ball_dx == 1) lpaddle_d = 0;
  else {
    if (lpaddle_y + paddle_h / 2 == target_y) lpaddle_d = 0;
    else if (lpaddle_y + paddle_h / 2 > target_y) lpaddle_d = -1;
    else lpaddle_d = 1;
  }

  if (lpaddle_y + paddle_h >= h && lpaddle_d == 1) lpaddle_d = 0;
  else if (lpaddle_y <= 0 && lpaddle_d == -1) lpaddle_d = 0;

  display.fillRect(lpaddle_x, lpaddle_y, paddle_w, paddle_h, fgcolor);
}
//*****************************************
void rpaddle() {
  if (pause > 0) return;
  if (rpaddle_d == 1) {
    display.fillRect(rpaddle_x, rpaddle_y, paddle_w, 1, bgcolor);
  }
  else if (rpaddle_d == -1) {
    display.fillRect(rpaddle_x, rpaddle_y + paddle_h - 1, paddle_w, 1, bgcolor);
  }
  rpaddle_y = rpaddle_y + rpaddle_d;
  if (ball_dx == -1) rpaddle_d = 0;
  else {
    if (rpaddle_y + paddle_h / 2 == target_y) rpaddle_d = 0;
    else if (rpaddle_y + paddle_h / 2 > target_y) rpaddle_d = -1;
    else rpaddle_d = 1;
  }

  if (rpaddle_y + paddle_h >= h && rpaddle_d == 1) rpaddle_d = 0;
  else if (rpaddle_y <= 0 && rpaddle_d == -1) rpaddle_d = 0;

  display.fillRect(rpaddle_x, rpaddle_y, paddle_w, paddle_h, fgcolor);
}
//*****************************************
void calc_target_y() {
  int16_t target_x;
  int16_t reflections;
  int16_t y;

  if (ball_dx == 1) {
    target_x = w - ball_w;
  }
  else {
    target_x = -1 * (w - ball_w);
  }

  y = abs(target_x * (ball_dy / ball_dx) + ball_y);

  reflections = floor(y / h);

  if (reflections % 2 == 0) {
    target_y = y % h;
  }
  else {
    target_y = h - (y % h);
  }

  // when the time changes, we want to dodge the ball
  // instead of seeking it.

  if (newminute && ball_dx == -1) {
    if (target_y > h - paddle_h) {
      target_y = target_y - paddle_h;
    } else {
      target_y = target_y + paddle_h;
    }
  } else if (newhour && ball_dx == 1) {
    if (target_y > h - paddle_h) {
      target_y = target_y - paddle_h;
    } else {
      target_y = target_y + paddle_h;
    }
  }
}
//*****************************************
void changescore(int newgame) {
  score(bgcolor);

  if (newhour) {
    lscore = hour;
    rscore = minute;
    newhour = 0;
    newminute = 0;
  } else if (newminute) {
    rscore = minute;
    newminute = 0;
  }

  if (newgame) {
    pause = 200;

    ball_x = paddle_w;
    ball_y = lpaddle_y + (paddle_h / 2);
    ball_dx = 1;

    calc_target_y();
  }
}
//*****************************************
void ball() {
  if (pause > 0) {
    pause = pause - 1;
    return;
  }

  display.fillRect(ball_x, ball_y, ball_w, ball_h, bgcolor);

  ball_x = ball_x + ball_dx;
  ball_y = ball_y + ball_dy;

  if (ball_dx == -1 && ball_x == paddle_w && ball_y + ball_h >= lpaddle_y && ball_y <= lpaddle_y + paddle_h) {
    ball_dx = ball_dx * -1;

    calc_target_y();
  } else if (ball_dx == 1 && ball_x + ball_w == w - paddle_w && ball_y + ball_h >= rpaddle_y && ball_y <= rpaddle_y + paddle_h) {
    ball_dx = ball_dx * -1;

    calc_target_y();
  } else if ((ball_dx == 1 && ball_x >= w) || (ball_dx == -1 && ball_x + ball_w < 0)) {
    changescore(1);
  }

  if (ball_y > h - ball_w || ball_y < 0) {
    ball_dy = ball_dy * -1;
  }

  display.fillRect(ball_x, ball_y, ball_w, ball_h, fgcolor);
}

