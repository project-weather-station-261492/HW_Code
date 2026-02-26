#ifndef HTTP_CLIENT_H
#define HTTP_CLIENT_H

#include <Arduino.h>

// ============================================
// HTTP CONFIGURATION
// ============================================
#define API_URL_SENSOR "https://solarwatch.cpe.eng.cmu.ac.th/api/status_sensor"   // API endpoint for sensor data (status)
#define API_URL_WEATHER "https://solarwatch.cpe.eng.cmu.ac.th/api/weather_db"     // API endpoint for weather values
#define MAX_HTTP_RETRY 3                                           // Maximum HTTP POST attempts

// ============================================
// HTTP CLIENT FUNCTIONS
// ============================================
/**
 * Send JSON payload to remote API via HTTPS POST
 * Uses SIM7670E modem: TCP+SSL (CIPSSL/CIPSTART/CIPSEND)
 * ESP32 builds payload -> sends via modem TLS -> server receives
 * Retries up to MAX_HTTP_RETRY times on failure
 * @param url - Destination URL (https://...)
 * @param payload - JSON string to send to API
 * @return true if HTTP 200 received, false otherwise
 */
bool httpPost(const char* url, const char* payload);

#endif
