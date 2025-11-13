#include <Wire.h>

#define AHT20_ADDRESS 0x38
#define LIGHT_SENSOR_PIN A0
#define RELAY_PIN 1

// === PARAMETRY KONTROLI (zgodnie z dokumentacją) ===
#define SETPOINT_BASE 45.0        // Bazowy punkt zadany 40-45% RH
#define DEADBAND 2.0              // ±2% RH (razem 4% deadband)
#define HUMIDITY_MAX 60.0         // Nigdy nie przekraczaj przez długi czas
#define HUMIDITY_CRITICAL 70.0    // Krytyczny próg ryzyka pleśni
#define MIN_OFF_TIME 300000       // 5 minut minimalny czas wyłączenia (ms)
#define MIN_ON_TIME 600000        // 10 minut minimalny czas włączenia (ms)
#define CONTROL_INTERVAL 60000    // 60 sekund między decyzjami (ms)

// Zmienne stanu
float previousHumidity = 0;
float previousTemp = 0;
unsigned long lastRelayChange = 0;
unsigned long lastControlUpdate = 0;
bool relayState = false;
int consecutiveHighReadings = 0;

void setup() {
  Serial.begin(9600);
  delay(2000);
  
  Serial.println("=== Inteligentny System Nawilzania ===");
  Serial.println("Bazujący na zaleceniach ASHRAE i badaniach Yale");
  Serial.println();
  
  // Inicjalizacja przekaźnika
  pinMode(RELAY_PIN, OUTPUT);
  digitalWrite(RELAY_PIN, LOW);
  relayState = false;
  Serial.println("[INIT] Relay: OFF");
  
  // Inicjalizacja I2C dla AHT20
  Wire.begin();
  delay(100);
  
  // Soft reset AHT20
  Wire.beginTransmission(AHT20_ADDRESS);
  Wire.write(0xBA);
  Wire.endTransmission();
  delay(20);
  
  // Inicjalizacja AHT20
  Wire.beginTransmission(AHT20_ADDRESS);
  Wire.write(0xBE);
  Wire.write(0x08);
  Wire.write(0x00);
  Wire.endTransmission();
  delay(10);
  
  Serial.println("[INIT] Czujnik AHT20: OK");
  Serial.println();
  Serial.println("=== PARAMETRY KONTROLI ===");
  Serial.print("Punkt zadany: ");
  Serial.print(SETPOINT_BASE, 1);
  Serial.println("% RH");
  Serial.print("Deadband: +-");
  Serial.print(DEADBAND, 1);
  Serial.println("% RH");
  Serial.print("Limit bezpieczenstwa: ");
  Serial.print(HUMIDITY_MAX, 1);
  Serial.println("% RH");
  Serial.println();
  
  lastControlUpdate = millis();
  lastRelayChange = millis();
}

void loop() {
  unsigned long currentTime = millis();
  
  // === ODCZYT CZUJNIKÓW (co 2 sekundy) ===
  
  Wire.beginTransmission(AHT20_ADDRESS);
  Wire.write(0xAC);
  Wire.write(0x33);
  Wire.write(0x00);
  Wire.endTransmission();
  
  delay(80);
  
  Wire.requestFrom(AHT20_ADDRESS, 7);
  
  float temp = 0;
  float hum = 0;
  bool validReading = false;
  
  if (Wire.available() >= 7) {
    uint8_t data[7];
    for (int i = 0; i < 7; i++) {
      data[i] = Wire.read();
    }
    
    // Sprawdź bit kalibracji
    if ((data[0] & 0x08) != 0) {
      uint32_t humidity = ((uint32_t)data[1] << 12) | 
                          ((uint32_t)data[2] << 4) | 
                          (data[3] >> 4);
      
      uint32_t temperature = (((uint32_t)data[3] & 0x0F) << 16) | 
                            ((uint32_t)data[4] << 8) | 
                            data[5];
      
      hum = (humidity / 1048576.0) * 100.0;
      temp = (temperature / 1048576.0) * 200.0 - 50.0;
      
      validReading = true;
    }
  }
  
  if (!validReading) {
    Serial.println("[ERROR] Nieprawidlowy odczyt z AHT20!");
    delay(2000);
    return;
  }
  
  // Odczyt światła (informacyjny)
  int lightValue = analogRead(LIGHT_SENSOR_PIN);
  
  // === LOGIKA STEROWANIA (co 60 sekund) ===
  
  if (currentTime - lastControlUpdate >= CONTROL_INTERVAL) {
    lastControlUpdate = currentTime;
    
    Serial.println("========================================");
    Serial.println("         OCENA WARUNKÓW");
    Serial.println("========================================");
    
    // Wyświetl odczyty
    Serial.print("Temperatura:  ");
    Serial.print(temp, 1);
    Serial.println(" °C");
    
    Serial.print("Wilgotność:   ");
    Serial.print(hum, 1);
    Serial.println(" % RH");
    
    Serial.print("Światło:      ");
    Serial.print(lightValue);
    Serial.print(" (");
    Serial.print((lightValue / 1023.0) * 100.0, 0);
    Serial.println("%)");
    
    Serial.println();
    
    // Oblicz błąd względem punktu zadanego
    float error = SETPOINT_BASE - hum;
    
    Serial.print("Punkt zadany: ");
    Serial.print(SETPOINT_BASE, 1);
    Serial.println("% RH");
    
    Serial.print("Błąd:         ");
    Serial.print(error, 1);
    Serial.println("% RH");
    
    // === BLOKADY BEZPIECZEŃSTWA ===
    
    bool safetyBlock = false;
    
    // 1. Wilgotność powyżej limitu bezpieczeństwa
    if (hum > HUMIDITY_MAX) {
      consecutiveHighReadings++;
      Serial.println();
      Serial.println("[SAFETY] PRZEKROCZONY LIMIT 60% RH!");
      Serial.print("Kolejne wysokie odczyty: ");
      Serial.println(consecutiveHighReadings);
      
      if (consecutiveHighReadings >= 2) {
        safetyBlock = true;
        Serial.println("[SAFETY] WYMUSZENIE WYLACZENIA!");
      }
    } else {
      consecutiveHighReadings = 0;
    }
    
    // 2. Wilgotność krytyczna (ryzyko pleśni)
    if (hum > HUMIDITY_CRITICAL) {
      safetyBlock = true;
      Serial.println();
      Serial.println("[CRITICAL] WILGOTNOSC >70% - RYZYKO PLESNI!");
      Serial.println("[CRITICAL] WYMUSZENIE WYLACZENIA!");
    }
    
    // 3. Temperatura za niska (opcjonalnie, jeśli masz czujnik zewnętrzny)
    if (temp < 15.0) {
      Serial.println();
      Serial.println("[WARNING] Temperatura ponizej 15°C");
      Serial.println("[INFO] Rozważ zwiekszenie ogrzewania");
    }
    
    Serial.println();
    
    // === LOGIKA DECYZYJNA ===
    
    bool shouldTurnOn = false;
    bool shouldTurnOff = false;
    String reason = "";
    
    if (safetyBlock) {
      // Blokada bezpieczeństwa - wymuś wyłączenie
      shouldTurnOff = true;
      reason = "Blokada bezpieczeństwa";
      
    } else if (abs(error) <= DEADBAND) {
      // W deadband - utrzymaj obecny stan
      reason = "W strefie deadband - bez zmian";
      
    } else if (error > DEADBAND) {
      // Wilgotność za niska - potrzeba nawilżania
      shouldTurnOn = true;
      reason = "Wilgotnosc ponizej progu (" + String(SETPOINT_BASE - DEADBAND, 1) + "%)";
      
    } else if (error < -DEADBAND) {
      // Wilgotność za wysoka - zatrzymaj nawilżanie
      shouldTurnOff = true;
      reason = "Wilgotnosc powyzej progu (" + String(SETPOINT_BASE + DEADBAND, 1) + "%)";
    }
    
    // === ZASTOSUJ DECYZJĘ Z TIMERS ANTY-CYKLOWANIA ===
    
    unsigned long timeSinceChange = currentTime - lastRelayChange;
    
    if (shouldTurnOn && !relayState) {
      // Chcemy włączyć - sprawdź minimalny czas wyłączenia
      if (timeSinceChange >= MIN_OFF_TIME) {
        digitalWrite(RELAY_PIN, HIGH);
        relayState = true;
        lastRelayChange = currentTime;
        Serial.println(">>> AKCJA: WLACZ ATOMIZER <<<");
        Serial.print("Powód: ");
        Serial.println(reason);
      } else {
        Serial.println(">>> BLOKADA: Minimalny czas wylaczenia nie uplynal <<<");
        Serial.print("Pozostalo: ");
        Serial.print((MIN_OFF_TIME - timeSinceChange) / 1000);
        Serial.println(" sekund");
      }
      
    } else if (shouldTurnOff && relayState) {
      // Chcemy wyłączyć - sprawdź minimalny czas włączenia
      if (timeSinceChange >= MIN_ON_TIME || safetyBlock) {
        digitalWrite(RELAY_PIN, LOW);
        relayState = false;
        lastRelayChange = currentTime;
        Serial.println(">>> AKCJA: WYLACZ ATOMIZER <<<");
        Serial.print("Powód: ");
        Serial.println(reason);
      } else if (!safetyBlock) {
        Serial.println(">>> BLOKADA: Minimalny czas wlaczenia nie uplynal <<<");
        Serial.print("Pozostalo: ");
        Serial.print((MIN_ON_TIME - timeSinceChange) / 1000);
        Serial.println(" sekund");
      }
      
    } else {
      // Brak zmiany stanu
      Serial.print("Status: ");
      Serial.println(relayState ? "WLACZONY" : "WYLACZONY");
      Serial.print("Powód: ");
      Serial.println(reason);
    }
    
    // === OCENA RYZYKA ===
    Serial.println();
    Serial.println("--- OCENA RYZYKA ---");
    
    if (hum < 30.0) {
      Serial.println("RYZYKO: Bardzo suche powietrze!");
      Serial.println("        Zagrozone blony sluzowe i gardlo");
    } else if (hum < 40.0) {
      Serial.println("UWAGA:  Wilgotnosc ponizej optymalnej (40%)");
      Serial.println("        Zwiekszony ryzyko infekcji");
    } else if (hum >= 40.0 && hum <= 50.0) {
      Serial.println("OPTYMALNE: Wilgotnosc w zakresie zdrowia");
    } else if (hum > 50.0 && hum <= 60.0) {
      Serial.println("UWAGA:  Wilgotnosc powyzej optymalnej");
    } else if (hum > 60.0 && hum <= 70.0) {
      Serial.println("OSTRZEZENIE: Ryzyko kondensacji!");
    } else {
      Serial.println("KRYTYCZNE: Wysokie ryzyko plesni!");
    }
    
    Serial.println("========================================\n");
  }
  
  // === PODSTAWOWE WYSWIETLANIE (co 2 sekundy) ===
  else {
    Serial.print("T:");
    Serial.print(temp, 1);
    Serial.print("°C | H:");
    Serial.print(hum, 1);
    Serial.print("% | L:");
    Serial.print((lightValue / 1023.0) * 100.0, 0);
    Serial.print("% | Atomizer:");
    Serial.println(relayState ? "ON" : "OFF");
  }
  
  previousHumidity = hum;
  previousTemp = temp;
  
  delay(2000); // Podstawowy cykl odczytu
}