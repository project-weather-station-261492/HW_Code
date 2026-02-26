# สถานีวัดสภาพอากาศแบบอัจฉริยะ (Weather Station)

## 📋 ภาพรวมโปรเจค

โปรเจคนี้เป็นระบบสถานีวัดสภาพอากาศที่ทำงานบน **ESP32** และสามารถ:
- วัดข้อมูลสภาพอากาศแบบต่อเนื่อง
- ติดต่อกับเซนเซอร์หลายตัวผ่าน **RS485 Modbus RTU**
- ส่งข้อมูลไปยังเซิร์ฟเวอร์ผ่านเครือข่าย **4G/LTE** (modem SIM7600)
- ประหยัดพลังงานด้วยการเข้า Deep Sleep

---

## 🎯 ฟีเจอร์หลัก

### 1. **वัดข้อมูลสภาพอากาศแบบครบถ้วน**
- 🌡️ อุณหภูมิ (Temperature)
- 💧 ความชื้น (Humidity)
- 🔬 ความดันลม (Air Pressure)
- 💨 ความเร็วลม (Wind Speed)
- 🧭 ทิศทางลม (Wind Direction)
- ☀️ อุณหภูมิแผง PV (Solar Temperature)
- 💡 ความเข้มแสง/ลักซ์ (Light Lux)

### 2. **การจัดการพลังงาน**
- ระบบ Relay ควบคุมการป้อนไฟ 5 ช่อง
  - Relay 1: SIM โมเด็ม
  - Relay 2-5: เซนเซอร์ RS485
- Deep Sleep mode ประหยัด < 5mA
- Watchdog Timer (90 วินาที) ป้องกันการ Hang

### 3. **การสื่อสารแบบโมดูลาร์**
- **RS485 Handler**: ติดต่อกับ 4 เซนเซอร์ Modbus RTU
- **Modem Core**: AT commands กับ SIM7600
- **HTTP Client**: ส่ง HTTPS Post ไปยังเซิร์ฟเวอร์

### 4. **วงจรการทำงาน**
- ⏰ ตื่นทุก **15 นาที** (สามารถปรับได้)
- 📍 ส่ง Hardware Status ทุกครั้ง
- 🕐 ส่ง Sensor Data ทุก **1 ชั่วโมง** (รวม 4 รอบ)
- 💤 เข้า Deep Sleep เพื่อประหยัดพลังงาน

---

## 🛠️ สถาปัตยกรรมซอฟต์แวร์

```
HW_Code/
├── src/
│   └── main.cpp                         # จุดเริ่มต้นโปรแกรม
├── lib/
│   ├── ModemCore/                       # ควบคุมโมเด็ม AT Commands
│   ├── RS485Handler/                    # ติดต่อเซนเซอร์ Modbus RTU
│   ├── SensorManager/                   # จัดการข้อมูลเซนเซอร์
│   ├── HttpClient/                      # ส่งข้อมูลผ่าน HTTPS
│   ├── PowerManager/                    # บริหารจัดการไฟ Relay
│   ├── HttpClient/
│   └── ModbusRTUMaster/                 # Library Modbus (Third-party)
└── platformio.ini                       # PlatformIO Configuration
```

---

## 📌 การกำหนดค่า Pin

### RS485 (Modbus RTU)
- **RX**: GPIO 25
- **TX**: GPIO 26
- **DE**: GPIO 27 (Driver Enable)
- **บอด**: 9600 bps

### Modem (SIM7600)
- **RX**: GPIO 16
- **TX**: GPIO 17
- **บอด**: 115200 bps

### Power Relay
| Relay | GPIO | อุปกรณ์ |
|-------|------|--------|
| 1 | 32 | SIM Modem |
| 2 | 13 | Wind Speed (RS485 ID=1) |
| 3 | 14 | Wind Direction (RS485 ID=2) |
| 4 | 18 | PV Temp/PT100 (RS485 ID=3) |
| 5 | 19 | BME280 + VEML770 (RS485 ID=4) |

---

## 🔧 การติดตั้งและคอมไพล์

### ข้อกำหนด
- PlatformIO IDE
- Board: ESP32 Dev Module
- Python (for serial communication)

### ขั้นตอนการติดตั้ง

1. **Clone/Download โปรเจค**
   ```bash
   cd HW_Code
   ```

2. **เปิด PlatformIO**
   - Open Folder → HW_Code
   - หรือ `platformio.ini` จะโหลดอัตโนมัติ

3. **ติดตั้ง Dependencies**
   ```bash
   pio run
   ```

4. **อัปโหลดโปรแกรม**
   ```bash
   pio run -t upload
   ```

5. **ดูเอาต์พุต Serial**
   ```bash
   pio device monitor -b 115200
   ```

---

## 📊 ไฟล์ JSON ที่ส่งไป API

### 1. Hardware Status (ส่งทุกรอบ 15 นาที)
```json
{
  "station_id": 1,
  "sensor_BME280": "Online",
  "sensor_VEML770": "Online",
  "sensor_WindSpeed": "Online",
  "sensor_WindDir": "Online",
  "sensor_PT100": "Online",
  "sensor_RG15": "Offline"
}
```

### 2. Weather Data (ส่งทุก 1 ชั่วโมง)
```json
{
  "station_id": 1,
  "air_temp": 2550,
  "humidity": 7200,
  "air_pressure": 10132,
  "rainfall": 0.0,
  "wind_speed": 45,
  "wind_direction": "NE",
  "wind_direction_num": 45,
  "light_lux": 50000,
  "solar_temp": 6200
}
```

---

## 📡 ทำไมใช้ Modbus RTU?

- ✅ โปรโตคอลอุตสาหกรรมมาตรฐาน
- ✅ ใช้ RS485 ได้ระยะไกล (ถึง 1200 เมตร)
- ✅ ไม่ต้องมี Modbus Gateway
- ✅ เซนเซอร์ราคาประหยัด

---

## 🔌 Modbus Device Map

| ID | ชื่ออุปกรณ์ | ข้อมูล | Register |
|----|-----------|--------|----------|
| 1 | Wind Speed | ความเร็วลม | HC Reg 0 |
| 2 | Wind Direction | ทิศทาง/เลขที่ | HC Reg 0-1 |
| 3 | PV Temp (PT100) | อุณหภูมิแผง | HC Reg 0 |
| 4 | BME280 + VEML770 | Temp, Humidity, Pressure, Lux | IR Reg 0-4 |

---

## ⚡ การบริหารพลังงาน

### วงจรชีวิต Deep Sleep
```
1. ตื่น (Wake) ← Timer
2. เชื่อมต่อ Modem & RS485
3. อ่านข้อมูล
4. ส่งข้อมูลไปเซิร์ฟเวอร์
5. ปิดทุกอุปกรณ์ (Power Off)
6. เข้า Deep Sleep 15 นาที
```

### เวลาการทำงาน
- **รวมการทำงาน**: ~30-60 วินาที
- **เวลา Deep Sleep**: 15 นาที
- **กระแส Deep Sleep**: < 5mA (ประหยัด 99%)

---

## 🐛 การแก้ไขปัญหา

### โมเด็มไม่ตอบสนอง
```
[FATAL] Modem unresponsive after 5 retries. Shutting down.
```
- ✅ ตรวจสอบ Antenna
- ✅ ตรวจสอบ SIM Card
- ✅ ลอง Hard Reset (ตัดไฟ 3 วิ)

### เซนเซอร์ RS485 ขาด
```
[RS485] READ FAIL id1 reg0
```
- ✅ ตรวจสอบสายเคเบิล RS485
- ✅ ยืนยันที่อยู่ Modbus (ID 1-4)
- ✅ ตรวจสอบ Baud Rate (9600)

### Deep Sleep ไม่ทำงาน
- ✅ ตรวจสอบ Pin 32 (SIM Power) มีประเภท GPIO ที่ใช้ได้

---

## 📝 Watchdog Timer (WDT)

- **Timeout**: 90 วินาที
- **Feed Points** (รีเซ็ត WDT):
  - ก่อน Modem Connect
  - ทุก AT Command Retry
  - ก่อน HTTP POST
  
⚠️ ถ้าโปรแกรม Hang เกิน 90 วิ → ESP32 จะ Reboot

---

## 📚 Libraries ที่ใช้

| Library | รุ่น | การใช้งาน |
|---------|-----|---------|
| ModbusRTUMaster | 1.0.5 | สื่อสาร Modbus RTU |
| TinyGSM | 0.11.7 | ควบคุมโมเด็ม 4G |
| SSLClient | 1.3.2 | HTTPS ผ่าน TCP ของโมเด็ม |
| Arduino Framework | - | Core Framework |

---

## 🌐 API Endpoints

ปลายทาง API ที่ใช้ส่งข้อมูล:

```
⚙️ Hardware Status:   https://solarwatch.cpe.eng.cmu.ac.th/api/status_sensor
📊 Weather Data:      https://solarwatch.cpe.eng.cmu.ac.th/api/weather_db
```

---

## 📋 Configuration (platformio.ini)

```ini
[env:esp32dev]
platform = espressif32
board = esp32dev
framework = arduino
monitor_speed = 115200

lib_deps =
  vshymanskyy/TinyGSM@^0.11.7
  digitaldragon/SSLClient@^1.3.2

build_flags =
  -D TINY_GSM_MODEM_SIM7600
  -D TINY_GSM_RX_BUFFER=1024
```

---

## 🔄 วงจรโปรแกรม Main

```cpp
// setup() - เรียกทุกครั้งที่ตื่น
1. Initialize Watchdog (90s)
2. Power ON ทุกอุปกรณ์
3. Initialize Modem UART
4. Test SIM Status
5. Test RS485 Devices
6. Send Hardware Status
7. Send Sensor Data (hourly)
8. Power OFF ทั้งหมด
9. Deep Sleep 15 นาที

// loop() - ไม่ใช้ (Deep Sleep resets MCU)
```

---

## 💡 เคล็ดลับ

- 📌 เปลี่ยน `SLEEP_INTERVAL_SECONDS` ใน `main.cpp` เพื่อปรับช่วงเวลาตื่น
- 📌 ใช้ Serial Monitor เพื่อดู Debug Log
- 📌 ตรวจสอบ RTC Memory เพื่อดู Cycle Counter
- 📌 ใช้ Modbus Probe Tool เพื่อ Test เซนเซอร์

---

## 📄 ใบอนุญาต

โปรเจคนี้ใช้ไลบรารี่ที่อยู่ภายใต้ MIT License

---

## 👨‍💻 ผู้เขียน

Weather Station Hardware Controller - ESP32 Based

---

**ติดตั้งเวียนขึ้นบนวันที่:** 26 กุมภาพันธ์ 2026
