# humidifier

> Automatic humidifier control based on ASHRAE recommendations and Yale University research  
> Protects against sore throat in winter when heaters dry out the air

---

## Table of Contents

- [About](#about)
- [Features](#features)
- [Hardware Requirements](#hardware-requirements)
- [Wiring Diagram](#wiring-diagram)
- [Installation](#installation)
- [Configuration](#configuration)
- [How It Works](#how-it-works)
- [Control Parameters](#control-parameters)
- [Log Interpretation](#log-interpretation)
- [Troubleshooting](#troubleshooting)
- [Scientific Background](#scientific-background)
- [License](#license)

---

## About

The system automatically controls an air humidifier (atomizer) to maintain optimal humidity of 40-50% RH in the room. It prevents:

- Sore throat from excessively dry air (< 40%)
- Mold and condensation from excessively humid air (> 60%)
- Increased infection risk during heating season

### Why This Matters

In winter, when heaters are running, indoor humidity drops to 15-20% RH (Sahara Desert levels!). This leads to:

- Drying of throat and nasal mucous membranes
- Weakened immune system
- Increased virus transmission (flu, cold)
- Eye and skin irritation

Yale University research (2019) showed that low humidity weakens respiratory defense mechanisms by over 50%.

---

## Features

### Basic

- Automatic humidity regulation at 45% +/- 2%
- Safety lockouts against mold (60% and 70% RH)
- Anti-cycling timers preventing too frequent on/off switching
- Real-time monitoring (temperature, humidity, light)
- Health risk assessment based on current conditions

### Advanced

- Hysteresis control algorithm (deadband +/- 2% RH)
- Three levels of safety lockouts
- Intelligent energy management (minimal cycling)
- Light sensor (informational)
- Detailed diagnostic logs

---

## Hardware Requirements

### Components

| Component | Model | Quantity | Function |
|-----------|-------|----------|----------|
| Microcontroller | Seeeduino XIAO | 1 | Main control unit |
| Temp/Humidity Sensor | Grove AHT20 | 1 | Condition monitoring (I2C) |
| Light Sensor | Grove Light Sensor | 1 | Light monitoring (analog) |
| Relay Module | Grove Relay v1.2 | 1 | Atomizer control |
| Atomizer/Humidifier | Any 230V | 1 | Humidification device |

### Optional

- Grove Shield for easier assembly
- Enclosure (3D printed or commercial)
- USB-C power supply for Seeeduino XIAO

---

## Wiring Diagram

### Seeeduino XIAO Pinout
```
Grove AHT20 (I2C) → Pins 4-5
├─ SDA → Pin 4 (D4)
├─ SCL → Pin 5 (D5)
├─ VCC → 3V3
└─ GND → GND

Grove Light Sensor → Pins 0-1
├─ SIG → Pin A0 (D0)
├─ VCC → 3V3
└─ GND → GND

Grove Relay Module → Pins 1-2
├─ SIG → Pin 1 (D1)
├─ VCC → 3V3
└─ GND → GND

Relay → Atomizer
├─ COM → Phase 230V
├─ NO  → To atomizer
└─ NC  → (not used)
```

### Visual Diagram
```
┌─────────────────┐
│ Seeeduino XIAO  │
├─────────────────┤
│ Pin 4-5 (I2C)   ├──→ AHT20 (Temp/Humidity)
│ Pin 0-1 (A0)    ├──→ Light Sensor
│ Pin 1 (D1)      ├──→ Relay → Atomizer 230V
│ 3V3             ├──→ Sensor power
│ GND             ├──→ Common ground
└─────────────────┘
```

---

## Installation

### 1. Arduino IDE Setup
```bash
# Install Arduino IDE 2.x
# https://www.arduino.cc/en/software

# Add Seeeduino board:
# File → Preferences → Additional Board URLs:
https://files.seeedstudio.com/arduino/package_seeeduino_boards_index.json

# Tools → Board → Boards Manager → Search "Seeeduino XIAO"
```

### 2. Library Installation
```
Sketch → Include Library → Manage Libraries

Install:
- Wire (built-in - I2C)
```

### 3. Upload Code
```bash
# 1. Connect Seeeduino XIAO via USB-C
# 2. Select: Tools → Board → Seeeduino XIAO
# 3. Select: Tools → Port → COMx (where x is port number)
# 4. Click: Upload (right arrow)
```

### 4. Open Serial Monitor
```
Tools → Serial Monitor
Set baudrate: 9600
```

---

## Configuration

### Adjustable Parameters

Open the .ino file and modify as needed:
```cpp
// === YOUR SETTINGS ===

#define SETPOINT_BASE 45.0        // Target humidity (40-50% recommended)
#define DEADBAND 2.0              // Tolerance ±2% (total 4%)
#define HUMIDITY_MAX 60.0         // Safety limit (mold!)
#define HUMIDITY_CRITICAL 70.0    // Critical limit (immediate shutdown)

#define MIN_OFF_TIME 300000       // 5 minutes (300000 ms) - minimum OFF time
#define MIN_ON_TIME 600000        // 10 minutes (600000 ms) - minimum ON time
#define CONTROL_INTERVAL 60000    // 60 seconds between decisions
```

### For Different Conditions

| Situation | Recommended SETPOINT_BASE |
|-----------|---------------------------|
| Dry winter (heaters) | 45% |
| Moderate winter | 42% |
| Spring/autumn | 40% |
| For plants | 50-55% |
| For infants | 40-45% |

WARNING: NEVER set above 55% - mold risk!

---

## How It Works

### Program Cycle
```
START
  ↓
┌─────────────────────┐
│ SETUP (once)        │
│ - Init sensors      │
│ - Init relay (OFF)  │
│ - Record start time │
└──────────┬──────────┘
           ↓
┌─────────────────────┐ ◄─┐
│ LOOP (every 2s)     │   │
│                     │   │
│ 1. Read sensors     │   │
│    - AHT20 (I2C)    │   │
│    - Light (A0)     │   │
│                     │   │
│ 2. Decision (60s)   │   │
│    ├─ Lockouts      │   │
│    ├─ Logic         │   │
│    └─ Timers        │   │
│                     │   │
│ 3. Display status   │   │
└──────────┬──────────┘   │
           └──────────────┘
```

### Decision Algorithm
```
EVERY MINUTE:

1. Read humidity (hum)
2. Calculate error = 45% - hum

3. CHECK LOCKOUTS:
   ├─ hum > 70%? → SHUT OFF IMMEDIATELY! (mold!)
   ├─ hum > 60%? → SHUT OFF (2x in a row)
   └─ temp < 15°C? → Warning

4. LOGIC:
   ├─ Lockout? → SHUT OFF
   ├─ hum < 43%? → TURN ON (if 5 min OFF elapsed)
   ├─ hum > 47%? → TURN OFF (if 10 min ON elapsed)
   └─ 43-47%? → Maintain state (deadband)

5. Display risk assessment
```

---

## Control Parameters

### Humidity Zones
```
  0%        30%       40%     45%     50%       60%       70%     100%
  ├──────────┼─────────┼───────┼───────┼─────────┼─────────┼───────┤
  │CRITICALLY│TOO LOW  │  OPTIMAL  │TOO HIGH │  MOLD!  │CRITICAL│
  │   DRY    │  RISK   │   TARGET  │ WARNING │  STOP   │  STOP! │
  └──────────┴─────────┴───────┴───────┴─────────┴─────────┴───────┘
                            43%  45%  47%
                             └───┼───┘
                             DEADBAND
```

### Anti-Cycling Timers

| Timer | Time | Reason |
|-------|------|--------|
| MIN_OFF_TIME | 5 minutes | Energy savings, relay protection |
| MIN_ON_TIME | 10 minutes | Effective humidification, stability |
| CONTROL_INTERVAL | 60 seconds | Avoid excessive reactions |

EXCEPTION: Safety lockout (> 60% RH) bypasses ON timer - shuts off immediately!

---

## Log Interpretation

### Example Output - Normal Operation
```
=== Intelligent Humidification System ===
Based on ASHRAE recommendations and Yale research

[INIT] Relay: OFF
[INIT] Sensor AHT20: OK

=== CONTROL PARAMETERS ===
Setpoint: 45.0% RH
Deadband: +-2.0% RH
Safety limit: 60.0% RH

T:22.5°C | H:38.2% | L:45% | Atomizer:OFF
T:22.5°C | H:38.3% | L:45% | Atomizer:OFF
...

========================================
         CONDITION ASSESSMENT
========================================
Temperature:  22.5 °C
Humidity:     38.3 % RH
Light:        462 (45%)

Setpoint: 45.0% RH
Error:    6.7% RH

>>> ACTION: TURN ON ATOMIZER <
Reason: Humidity below threshold (43.0%)

--- RISK ASSESSMENT ---
WARNING: Humidity below optimal (40%)
         Increased infection risk
========================================

T:22.6°C | H:38.5% | L:46% | Atomizer:ON
T:22.6°C | H:38.9% | L:46% | Atomizer:ON
...
```

### Example Output - Safety Lockout
```
========================================
         CONDITION ASSESSMENT
========================================
Temperature:  23.1 °C
Humidity:     62.4 % RH
Light:        512 (50%)

[SAFETY] 60% RH LIMIT EXCEEDED!
Consecutive high readings: 2
[SAFETY] FORCING SHUTDOWN!

>>> ACTION: TURN OFF ATOMIZER <
Reason: Safety lockout

--- RISK ASSESSMENT ---
WARNING: Condensation risk!
========================================
```

### Message Meanings

| Message | Meaning | Action |
|---------|---------|--------|
| `[INIT]` | Component initialization | Informational |
| `[ERROR]` | Sensor read error | Check connections |
| `[SAFETY]` | Safety lockout | System protecting against mold |
| `[CRITICAL]` | Critical situation | Immediate shutdown |
| `[WARNING]` | Warning | Pay attention, but system working |
| `>>> ACTION: ...` | Relay state change | Atomizer on/off |
| `>>> LOCKOUT: ...` | Anti-cycling timer | Waiting for minimum time |

---

## Troubleshooting

### Problem: "Invalid reading from AHT20"

CAUSES:
- Loose I2C connection (SDA/SCL)
- Poor sensor calibration
- Damaged sensor

SOLUTION:
```bash
1. Check connections:
   - SDA (Pin 4) OK
   - SCL (Pin 5) OK
   - VCC → 3V3 OK
   - GND → GND OK

2. Run I2C scanner:
   # Should detect address 0x38
   
3. Reset sensor (disconnect power for 10s)
```

### Problem: Atomizer won't turn on

CAUSES:
- Anti-cycling timer active
- Humidity already in deadband (43-47%)
- Safety lockout

SOLUTION:
```bash
1. Check logs:
   ">>> LOCKOUT: Minimum time..." → Wait
   "In deadband zone" → System OK
   "[SAFETY]" → Humidity too high
   
2. Check if relay works:
   - Hear click? OK
   - LED on module lit? OK
   
3. Check atomizer power (230V!)
```

### Problem: Humidity too high (> 60%)

CAUSES:
- Insufficient ventilation
- Moisture sources (drying laundry, cooking)
- Damaged atomizer (leaking)

SOLUTION:
```bash
1. System automatically shuts off atomizer
2. Increase ventilation (open window)
3. Remove moisture sources
4. If humidity doesn't drop → check atomizer

WARNING: Above 70% → MOLD RISK!
```

### Problem: Frequent on/off switching

This is NORMAL! System has timers:
- Minimum 5 minutes OFF
- Minimum 10 minutes ON

However, if switching MORE frequently:
```bash
1. Check deadband (should be 4%)
2. Check MIN_OFF_TIME and MIN_ON_TIME
3. Increase CONTROL_INTERVAL to 120s
4. Check if atomizer too powerful for room
```

---

## Scientific Background

### Why 40-50% RH?

System based on:

1. ASHRAE Standard 55 - Thermal Comfort
   - Recommended: 30-60% RH
   - Optimal: 40-50% RH

2. Yale University Research (2019, PNAS)
   - Low humidity (< 40%) weakens immune system by 50%
   - Three mechanisms: cilia, cell repair, interferon signaling

3. Health Risk Minimization
   - < 40% RH: Mucous membrane drying, increased virus transmission
   - 40-60% RH: MINIMAL all hazards
   - > 60% RH: Mold growth, dust mites, bacteria

### Hysteresis Control Algorithm
```
Setpoint: 45% RH
Deadband: ±2% RH

Turn-on zone:     < 43% RH
Neutral zone:     43-47% RH (no change)
Turn-off zone:    > 47% RH

Advantages:
- Prevents cycling
- Saves energy
- Protects components
- Stable control
```

---

## Additional Information

### Compatibility

- Seeeduino XIAO (SAMD21)
- Arduino Uno/Nano (change I2C pins)
- ESP32 (add Wire library)
- Raspberry Pi Pico (Arduino framework)

### Project Extensions

Possible improvements:

- Outdoor temperature compensation (add DS18B20 sensor)
- WiFi + Blynk/MQTT (ESP32)
- LCD/OLED display
- Alarms (buzzer at > 70% RH)
- SD card logging
- Home Assistant integration

### Safety

WARNING:
- Relay switches 230V AC - electric shock hazard!
- Relay → atomizer wiring by licensed electrician only
- Use appropriate wires (min. 1.5mm²)
- Protect connections from moisture
- DO NOT touch 230V components during operation

---

## License

MIT License

Copyright (c) 2025

Permission is hereby granted, free of charge, to any person obtaining a copy
of this software and associated documentation files (the "Software"), to deal
in the Software without restriction, including without limitation the rights
to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
copies of the Software.

Use at your own risk. Author is not responsible for:
- Equipment damage
- Electric shock
- Mold problems
- Any other damages

---

## Credits

- Algorithm based on ASHRAE Handbook - HVAC Systems and Equipment
- Health parameters from Yale University research (2019)
- Sensor documentation from Seeed Studio Grove AHT20
- Inspiration: Building Science Corporation - Relative Humidity Research

---

## Support

Questions? Problems?

1. Check Troubleshooting section
2. Read diagnostic logs
3. Measure humidity with another device (calibration)

---

Built for your throat health in winter
```
         _______________
        /               \
       /     22.5°C     \
      /      45.2%       \
     /     OPTIMAL        \
    /________________________\
    |   Seeeduino XIAO      |
    |   AHT20 Sensor        |
    |   Relay Control       |
    |________________________|
            ||      ||
         ATOMIZER  POWER
```

---

Last updated: 2025-01-15  
Version: 1.0.0