#include "SensorManager.h"
#include "../HttpClient/HttpClient.h"
#include "../RS485Handler/RS485Handler.h"

// ============================================
// SENSOR DATA COLLECTION & TRANSMISSION
// ============================================

/**
 * Send hardware status report to remote API
 * Reports:
 * - SIM modem status
 * - Status of all 4 RS485 sensor devices
 * - Current measurement cycle counter
 * Useful for monitoring system health and device connectivity
 * @param deep_sleep_cycle - Current cycle number from RTC memory
 */
/**
 * Send hardware status report to remote API
 * Now sends to /status_sensor with online/offline status
 * @param deep_sleep_cycle - Current cycle number (unused in new schema but kept for signature)
 */
void sendHW(uint32_t deep_sleep_cycle) {
    Serial.println("[SEND] Hardware status");

    // Helper lambda or macro could be used, but inline text comparison is simple enough
    // Mapping: "Normal" -> "online", anything else -> "offline"

    char payload[512];
    snprintf(payload, sizeof(payload),
        "{\"station_id\":%d,"
        "\"sensor_BME280\":\"%s\","
        "\"sensor_VEML770\":\"%s\","
        "\"sensor_WindSpeed\":\"%s\","
        "\"sensor_WindDir\":\"%s\","
        "\"sensor_PT100\":\"%s\","
        "\"sensor_RG15\":\"Offline\"}",
        STATION_ID,
        HW_s4_bme_ok  ? "Online" : "Offline",                           // BME280  (bit0)
        HW_s4_veml_ok ? "Online" : "Offline",                           // VEML770 (bit1)
        (strcmp(HW_rs485_status[1], "Normal") == 0) ? "Online" : "Offline", // RS1 (WindSpeed)
        (strcmp(HW_rs485_status[2], "Normal") == 0) ? "Online" : "Offline", // RS2 (WindDir)
        (strcmp(HW_rs485_status[3], "Normal") == 0) ? "Online" : "Offline"  // RS3 (PV Temp/PT100)
    );

    httpPost(API_URL_SENSOR, payload);
}

/**
 * Collect sensor data from all RS485 devices and send to API
 * Reads from all sensors and sends two payloads:
 * 1. Sensor values to /weather_db
 * 2. Sensor status (online/offline) to /status_sensor
 * @param hourly_cycle_count - Reference to cycle counter (reset after send)
 */
void sendSensor(uint32_t& hourly_cycle_count) {
    Serial.println("[SEND] Sensor data");

    // Initialize sensor values
    uint16_t t = 0, h = 0, p = 0, ws = 0, wd = 0, wdn = 0, lx = 0, st = 0;
    bool s_st = false, s_wdn = false, s_wd = false, s_ws = false;
    bool s_t = false, s_h = false, s_p = false, s_lx = false;

    // Read sensor data and capture status
    s_ws  = readReg(1, 0, &ws);     // Wind speed (SlaveID=1)
    s_wdn = readReg(2, 0, &wdn);    // Wind direction number (SlaveID=2)
    s_wd  = readReg(2, 1, &wd);     // Wind direction (SlaveID=2)
    s_st  = readReg(3, 0, &st);     // PV Temp / Solar temperature (SlaveID=3)

    // SlaveID=4: BME280 + VEML770 — FC4 block read (Addr 0, Qty 5)
    // Addr 0: Status (bit0=BME_ERR, bit1=VEML_ERR)
    // Addr 1: Temperature ×100  Addr 2: Humidity ×100
    // Addr 3: Pressure ×10      Addr 4: Lux
    Slave4Data s4 = {};
    bool s4_ok = readSlave4(&s4);
    s_t  = s4_ok && s4.ok_bme;    // BME280 temp  valid
    s_h  = s4_ok && s4.ok_bme;    // BME280 hum   valid
    s_p  = s4_ok && s4.ok_bme;    // BME280 pres  valid
    s_lx = s4_ok && s4.ok_veml;   // VEML770 lux  valid
    if (s4_ok) {
        t  = s4.temperature;       // raw ×100 (°C)
        h  = s4.humidity;          // raw ×100 (%)
        p  = s4.pressure;          // raw ×10  (hPa)
        lx = s4.lux;               // raw lux
    }

    // 1. Send Sensor Values to /weather_db
    char payload_weather[512];
    snprintf(payload_weather, sizeof(payload_weather),
        "{\"station_id\":%d,"
        "\"air_temp\":%u,\"humidity\":%u,\"air_pressure\":%u,"
        "\"rainfall\":0.0,"
        "\"wind_speed\":%u,\"wind_direction\":\"%u\","
        "\"wind_direction_num\":%u,\"light_lux\":%u,"
        "\"solar_temp\":%u}",
        STATION_ID, t, h, p, ws, wd, wdn, lx, st);

    httpPost(API_URL_WEATHER, payload_weather);

    // 2. Send Sensor Status to /status_sensor
    char payload_status[512];
    snprintf(payload_status, sizeof(payload_status),
        "{\"station_id\":%d,"
        "\"sensor_BME280\":\"%s\","
        "\"sensor_VEML770\":\"%s\","
        "\"sensor_WindSpeed\":\"%s\","
        "\"sensor_WindDir\":\"%s\","
        "\"sensor_PT100\":\"%s\","
        "\"sensor_RG15\":\"offline\"}",
        STATION_ID,
        (s_t && s_h && s_p) ? "Online" : "Offline", // BME280 (Temp, Hum, Pres)
        s_lx ? "Online" : "Offline",                  // VEML770 (Light)
        s_ws ? "Online" : "Offline",                  // Wind Speed
        (s_wd && s_wdn) ? "Online" : "Offline",       // Wind Dir
        s_st ? "Online" : "Offline"                   // PT100 (Solar Temp)
    );

    httpPost(API_URL_SENSOR, payload_status);
}
