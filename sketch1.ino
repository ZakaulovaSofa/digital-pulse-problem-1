#include <ESP8266WiFi.h>
#include <WiFiClientSecure.h>
#include <UniversalTelegramBot.h>
#include <ArduinoJson.h>

// Пины для RGB, микрофона и кнопки
const int micPin = A0;
const int buttonPin = 14;  // D5 на ESP8266
const int redPin = 5;      // D1
const int greenPin = 4;    // D2
const int bluePin = 0;     // D3

// Порог звука (настройте под ваш микрофон)
const int soundThreshold = 39; // Изменил порог на 50 по вашему требованию
const unsigned long monitoringDuration = 20000;  // 20 секунд
const unsigned long longSoundThreshold = 2000;   // 2 секунды (по вашему требованию)

bool isMonitoring = false;
bool isBroken = false; // Флаг поломки
unsigned long monitoringStartTime = 0;
unsigned long soundStartTime = 0;
bool soundDetected = false;

void setup() {
  Serial.begin(115200);
  
  // Настройка пинов
  pinMode(redPin, OUTPUT);
  pinMode(greenPin, OUTPUT);
  pinMode(bluePin, OUTPUT);
  pinMode(buttonPin, INPUT_PULLUP);  // Кнопка с подтяжкой к VCC
  
  setColor(0, 0, 255);  // Исходный цвет - синий
}

void loop() {
  // Проверка нажатия кнопки (с антидребезгом)
  if (digitalRead(buttonPin) == LOW && !isMonitoring && !isBroken) {
    delay(50);  // Антидребезг
    if (digitalRead(buttonPin) == LOW) {
      startMonitoring();
    }
  }

  // Если обнаружена поломка - мигаем красным
  if (isBroken) {
    blinkRed();
    return; // Прекращаем дальнейшие проверки
  }

  // Если идёт мониторинг
  if (isMonitoring) {
    unsigned long currentTime = millis();
    
    // Проверка, не истекло ли время мониторинга
    if (currentTime - monitoringStartTime >= monitoringDuration) {
      stopMonitoring();
      return;
    }

    // Чтение звука
    int soundValue = analogRead(micPin);
    Serial.print("Время: ");
    Serial.print((currentTime - monitoringStartTime) / 1000);
    Serial.print("с | Уровень звука: ");
    Serial.println(soundValue);

    // Проверка на превышение порога
    if (soundValue > soundThreshold) {
      if (!soundDetected) {
        soundDetected = true;
        soundStartTime = currentTime;
      } else {
        // Если звук длится дольше 2 секунд
        if (currentTime - soundStartTime >= longSoundThreshold) {
          Serial.println("⚠️ Поломка! Долгий звук (>2 сек)");
          triggerBrokenState();
          return; // Прекращаем мониторинг
        }
      }
    } else {
      soundDetected = false;
    }
    
    // Во время мониторинга горим зелёным
    setColor(0, 255, 0);
    delay(1000);  // Уменьшил задержку для более точного детектирования
  }
}

void startMonitoring() {
  Serial.println("🚀 Начало мониторинга звука (20 сек)");
  isMonitoring = true;
  isBroken = false;
  monitoringStartTime = millis();
  setColor(0, 255, 0);  // Зелёный (мониторинг)
}

void stopMonitoring() {
  Serial.println("🛑 Мониторинг завершён");
  isMonitoring = false;
  soundDetected = false;
  setColor(0, 0, 255);  // Синий (ожидание)
}

void triggerBrokenState() {
  isBroken = true;
  isMonitoring = false;
  Serial.println("‼️ Обнаружена поломка! Мониторинг остановлен");
}

void blinkRed() {
  static unsigned long lastBlinkTime = 0;
  static bool ledState = false;
  
  if (millis() - lastBlinkTime >= 500) { // Мигаем каждые 500мс
    ledState = !ledState;
    if (ledState) {
      setColor(255, 0, 0); // Красный
    } else {
      setColor(0, 0, 0);   // Выключено
    }
    lastBlinkTime = millis();
  }
}

// Функция установки цвета RGB
void setColor(int red, int green, int blue) {
  analogWrite(redPin, red);
  analogWrite(greenPin, green);
  analogWrite(bluePin, blue);
}