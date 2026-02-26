#include "HttpClient.h"
#include "../ModemCore/ModemCore.h"

// TinyGSM for raw TCP via cellular modem
#define TINY_GSM_MODEM_SIM7600
#include <TinyGsmClient.h>
#include <SSLClient.h>

// External hardware interface
extern HardwareSerial Modem;

// TinyGSM modem instance (uses the same UART)
TinyGsm modemGSM(Modem);

// ============================================
// HTTP POST via TinyGSM + SSLClient
// ============================================
/**
 * Architecture:
 *   ESP32 (SSLClient/BearSSL) → A7670 modem (raw TCP) → Server HTTPS
 *   
 * The modem provides raw TCP transport only.
 * ESP32 handles the TLS handshake via SSLClient.
 * This bypasses the A7670's broken SSL.
 */
bool httpPost(const char* url, const char* payload) {
    Serial.println("[HTTP] POST start");
    Serial.println(payload);

    // Extract host, path, port
    String host = "";
    String path = "/";
    int port = 443;

    if (strncmp(url, "https://", 8) == 0) {
        int slashIdx = String(url).indexOf('/', 8);
        if (slashIdx != -1) {
            host = String(url).substring(8, slashIdx);
            path = String(url).substring(slashIdx);
        } else {
            host = String(url).substring(8);
        }
        port = 443;
    } else if (strncmp(url, "http://", 7) == 0) {
        int slashIdx = String(url).indexOf('/', 7);
        if (slashIdx != -1) {
            host = String(url).substring(7, slashIdx);
            path = String(url).substring(slashIdx);
        } else {
            host = String(url).substring(7);
        }
        port = 80;
    }

    Serial.printf("[HTTP] Host: %s, Path: %s, Port: %d\n", 
                  host.c_str(), path.c_str(), port);

    for (int r = 0; r < MAX_HTTP_RETRY; r++) {
        Serial.printf("[HTTP] Try %d\n", r + 1);

        // Initialize TinyGSM
        Serial.println("[GSM] Init modem...");
        if (!modemGSM.testAT(5000)) {
            Serial.println("[GSM] Modem not responding");
            rebootModemHard();
            continue;
        }

        // Wait for network
        Serial.println("[GSM] Waiting for network...");
        if (!modemGSM.waitForNetwork(30000)) {
            Serial.println("[GSM] Network failed");
            continue;
        }
        Serial.println("[GSM] Network OK");

        // Connect GPRS
        Serial.println("[GSM] GPRS connecting...");
        if (!modemGSM.gprsConnect(APN)) {
            Serial.println("[GSM] GPRS failed");
            continue;
        }
        Serial.println("[GSM] GPRS connected");

        // Create TLS connection
        // TinyGsmClient = raw TCP via modem
        // SSLClient wraps it with TLS (ESP32 side)
        TinyGsmClient tcpClient(modemGSM, 0);
        SSLClient sslClient(&tcpClient);
        sslClient.setInsecure();  // Skip cert verification

        Serial.printf("[HTTP] Connecting SSL to %s:%d...\n", host.c_str(), port);

        if (!sslClient.connect(host.c_str(), port)) {
            Serial.println("[HTTP] SSL connect failed");
            modemGSM.gprsDisconnect();
            delay(2000);
            continue;
        }
        Serial.println("[HTTP] SSL connected!");

        // Build and send HTTP POST
        String httpReq = "POST " + path + " HTTP/1.1\r\n";
        httpReq += "Host: " + host + "\r\n";
        httpReq += "Content-Type: application/json\r\n";
        httpReq += "Content-Length: " + String(strlen(payload)) + "\r\n";
        httpReq += "Connection: close\r\n\r\n";
        httpReq += String(payload);

        sslClient.print(httpReq);
        Serial.println("[HTTP] Request sent");

        // Read response
        uint32_t start = millis();
        String response = "";
        while (millis() - start < 15000) {
            while (sslClient.available()) {
                response += (char)sslClient.read();
            }
            if (!sslClient.connected() && response.length() > 0) break;
            delay(10);
        }

        sslClient.stop();
        modemGSM.gprsDisconnect();

        Serial.println("[HTTP] Response:");
        Serial.println(response.substring(0, 200));

        if (response.indexOf("200 OK") != -1 || response.indexOf("200 ") != -1) {
            Serial.println("[HTTP] SUCCESS");
            return true;
        } else {
            Serial.println("[HTTP] POST failed, retrying...");
            delay(2000);
        }
    }

    Serial.println("[HTTP] POST FAILED");
    return false;
}
