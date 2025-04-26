#include <ESP8266WiFi.h>
#include <WiFiClientSecure.h>
#include <UniversalTelegramBot.h>
#include <ArduinoJson.h>
#include <DHT.h>

// Пины для RGB, микрофона и кнопки
const int micPin = A0;
const int buttonPin = 14;  // D5 на ESP8266
const int redPin = 5;      // D1
const int greenPin = 4;    // D2
const int bluePin = 0;     // D3
const int dhtPin = 12;     // D6 для DHT11

// Параметры датчика DHT
#define DHTTYPE DHT11
DHT dht(dhtPin, DHTTYPE);

// Пороги
const int soundThreshold = 39; // Порог звука
const int moistureThreshold = 30; // Порог влажности
const unsigned long soundDurationThreshold = 100; // 0.1 секунды звука
const unsigned long moistureCheckDuration = 10000; // 10 секунд проверки влажности
const unsigned long monitoringDuration = 20000;  // 20 секунд общего мониторинга
const unsigned long measurementInterval = 1000;  // Интервал замеров 1 секунда

// Состояния системы
enum SystemState { IDLE, MONITORING, SOUND_DETECTED, CHECKING_MOISTURE, BROKEN };
SystemState systemState = IDLE;

// Таймеры
unsigned long monitoringStartTime = 0;
unsigned long soundStartTime = 0;
unsigned long moistureCheckStartTime = 0;

void setup() {
  Serial.begin(115200);
  
  // Настройка пинов
  pinMode(redPin, OUTPUT);
  pinMode(greenPin, OUTPUT);
  pinMode(bluePin, OUTPUT);
  pinMode(buttonPin, INPUT_PULLUP);
  
  // Инициализация DHT11
  dht.begin();
  
  setColor(0, 0, 255);  // Исходный цвет - синий (ожидание)
}

void loop() {
  unsigned long currentTime = millis();

  // Обработка состояний
  switch (systemState) {
    case IDLE:
      handleIdleState();
      break;
      
    case MONITORING:
      handleMonitoringState(currentTime);
      break;
      
    case SOUND_DETECTED:
      handleSoundDetectedState(currentTime);
      break;
      
    case CHECKING_MOISTURE:
      handleMoistureCheckState(currentTime);
      break;
      
    case BROKEN:
      handleBrokenState();
      break;
  }
  
  delay(100); // Небольшая задержка для стабильности
}

void handleIdleState() {
  // Проверка нажатия кнопки (с антидребезгом)
  if (digitalRead(buttonPin) == LOW) {
    delay(50);
    if (digitalRead(buttonPin) == LOW) {
      startMonitoring();
    }
  }
}

void handleMonitoringState(unsigned long currentTime) {
  // Проверка времени мониторинга
  if (currentTime - monitoringStartTime >= monitoringDuration) {
    stopMonitoring();
    return;
  }

  // Проверка звука каждую секунду
  static unsigned long lastSoundCheck = 0;
  if (currentTime - lastSoundCheck >= measurementInterval) {
    lastSoundCheck = currentTime;
    
    int soundValue = analogRead(micPin);
    Serial.print("Время: ");
    Serial.print((currentTime - monitoringStartTime) / 1000);
    Serial.print("с | Уровень звука: ");
    Serial.println(soundValue);

    if (soundValue > soundThreshold) {
      if (systemState == MONITORING) {
        systemState = SOUND_DETECTED;
        soundStartTime = currentTime;
        Serial.println("🔊 Обнаружен звук выше порога");
      }
    }
  }

  setColor(0, 255, 0);  // Зелёный (мониторинг)
}

void handleSoundDetectedState(unsigned long currentTime) {
  // Проверяем, продолжается ли звук
  int soundValue = analogRead(micPin);
  
  if (soundValue > soundThreshold) {
    Serial.println(currentTime - soundStartTime);
    if (currentTime - soundStartTime >= soundDurationThreshold) {
      Serial.println("🔔 Звуковой сигнал подтверждён, начинаю проверку влажности");
      systemState = CHECKING_MOISTURE;
      moistureCheckStartTime = currentTime;
      setColor(255, 165, 0); // Оранжевый (проверка влажности)
    }
  } else {
    // Звук прекратился до достижения порога
    systemState = MONITORING;
    Serial.println("🔇 Звук прекратился");
  }
}

void handleMoistureCheckState(unsigned long currentTime) {
  // Проверяем влажность каждую секунду
  static unsigned long lastMoistureCheck = 0;
  if (currentTime - lastMoistureCheck >= measurementInterval) {
    lastMoistureCheck = currentTime;
    
    float humidity = dht.readHumidity();
    float temperature = dht.readTemperature();
    
    if (isnan(humidity) || isnan(temperature)) {
      Serial.println("Ошибка чтения датчика DHT!");
    } else {
      Serial.print("Влажность: ");
      Serial.print(humidity);
      Serial.print("% | Температура: ");
      Serial.print(temperature);
      Serial.println("°C");
      
      if (humidity > moistureThreshold) {
        triggerBrokenState();
        return;
      }
    }
  }

  // Проверка времени проверки влажности
  if (currentTime - moistureCheckStartTime >= moistureCheckDuration) {
    Serial.println("✅ Проверка влажности завершена, проблем не обнаружено");
    systemState = MONITORING;
  }
}

void handleBrokenState() {
  // Мигаем красным при поломке
  static unsigned long lastBlinkTime = 0;
  static bool ledState = false;
  
  if (millis() - lastBlinkTime >= 500) {
    ledState = !ledState;
    if (ledState) {
      setColor(255, 0, 0); // Красный
    } else {
      setColor(0, 0, 0);   // Выключено
    }
    lastBlinkTime = millis();
  }
}

void startMonitoring() {
  Serial.println("🚀 Начало мониторинга звука (20 сек)");
  systemState = MONITORING;
  monitoringStartTime = millis();
  setColor(0, 255, 0);  // Зелёный (мониторинг)
}

void stopMonitoring() {
  Serial.println("🛑 Мониторинг завершён");
  systemState = IDLE;
  setColor(0, 0, 255);  // Синий (ожидание)
}

void triggerBrokenState() {
  systemState = BROKEN;
  Serial.println("‼️ ОБНАРУЖЕНА ПОЛОМКА! Влажность превысила 30%");
}

// Функция установки цвета RGB
void setColor(int red, int green, int blue) {
  analogWrite(redPin, red);
  analogWrite(greenPin, green);
  analogWrite(bluePin, blue);
}