
#define REL_HSTR 2  // ширина гистерезиса, градусов
#define ENC_A 4
#define ENC_B 3
#define ENC_KEY 2
#define BUZZ_PIN 5
#define DISP_CLK 6
#define DISP_DIO 7
#define MOS_PIN 8
#define NTC_PIN 0

// библиотека энкодера
#include <EncButton.h>
EncButton<EB_TICK, ENC_A, ENC_B, ENC_KEY> enc;
// библиотека дисплея
#include <GyverTM1637.h>
GyverTM1637 disp(DISP_CLK, DISP_DIO);
// библиотека термистора
#include <GyverNTC.h>
GyverNTC ntc(NTC_PIN, 10000, 3950);

bool state;     // статус (0 ожидание, 1 таймер запущен)
byte mode;      // вывод: 0 часы, 1 градусы, 2 датчик
uint32_t tmr;   // общий таймер работы
int mins = 480; // минуты (по умолч 8 часов)
int temp = 40;  // целевая температура (по умолч 40 градусов)
int sens = 0;   // темепратура с датчика

void setup() {
  // активные пины как выходы
  pinMode(BUZZ_PIN, OUTPUT);
  pinMode(MOS_PIN, OUTPUT);
  // на всякий случай очистим дисплей и установим яркость
  disp.clear();
  disp.brightness(7);
  updDisp();  // обновить дисплей
}

void loop() {
  controlTick();  // опрос энкодера
  sensorTick();   // опрос датчика и работа реле
  if (state) timerTick();   // опрос таймеров (во время работы)
}

void controlTick() {
  enc.tick();         // опрос энкодера
  if (enc.turn()) {           // если был поворот (любой)
    // при повороте меняем значения согласно режиму вывода
    if (enc.right()) {        // вправо
      if (mode == 1) temp++;
      else if (mode == 0) mins += 10;
    }
    if (enc.left()) {         // влево
      if (mode == 1) temp--;
      else if (mode == 0) mins -= 10;
      if (mins < 0) mins = 0;
    }
    updDisp();      // обновляем дисплей в любом случае
  }
  if (enc.held()) {   // кнопка энкодера удержана
    if (!state) {     // если таймер не запущен
      state = 1;      // запускаем
      tmr = millis(); // и запоминаем время запуска
    }
  }
  if (enc.click()) {          // клик по кнопке
    mode++;                   // следующий режим вывода
    if (mode >= 3) mode = 0;  // закольцуем 0,1,2
    updDisp();                // обновить дисплей
  }
}

void timerTick() {
  // переменные таймеров
  static uint32_t tmrSec;
  // таймер 2 раза в секунду
  if (millis() - tmrSec >= 500) {
    tmrSec = millis();
    // переключаем переменную 0,1,0,1... для мигания точками
    static bool dotsFlag = 0;
    dotsFlag = !dotsFlag;
    disp.point(dotsFlag, false);     // обновляем точки
    updDisp();  // обновляем дисп
    // ждём после вывода данных, особенность дисплея
  }
  // рабочий таймер,с момента запуска до "минут"
  if (millis() - tmr >= mins * 60000ul) {
    // время вышло! Суши вёсла
    digitalWrite(MOS_PIN, 0);
    // пропищать 5 раз
    for (int i = 0; i < 5; i++) {
      tone(BUZZ_PIN, 1000);   // на частоте 1000 Гц
      delay(400);
      noTone(BUZZ_PIN);
      delay(400);
    }
    state = 0;    // уйти в ждущий режим
  }
}
// опрос датчика, регулирование
void sensorTick() {
  static uint32_t tmrSens;
  // каждую секунду
  if (millis() - tmrSens >= 1000) {
    tmrSens = millis();
    // получаем значение с датчика и округляем
    sens = round(ntc.getTempAverage());
    // если главный таймер запущен - релейное регулирование
    // с гистерезисом (см. урок)
    if (state) {
      static bool relayState = 0;
      if (sens < temp - REL_HSTR) relayState = 1;
      else if (sens >= temp) relayState = 0;
      digitalWrite(MOS_PIN, relayState);
    }
  }
}
// обновить дисплей в зависимости от текущего режима отображения
void updDisp() {
  switch (mode) {
    case 0:
      if (!state) {
        disp.point(1);
        disp.displayClock(mins / 60, mins % 60);
      } else {
        int curMins = mins - (millis() - tmr) / 60000ul;
        disp.displayClock(curMins / 60, curMins % 60);
      }
      break;
    case 1:
      if (!state) disp.point(0);
      disp.displayInt(temp);
      disp.displayByte(0, _t);
      break;
    case 2:
      if (!state) disp.point(0);
      disp.displayInt(sens);
      disp.displayByte(0, _S);
      break;
  }
}