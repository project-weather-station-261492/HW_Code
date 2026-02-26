#include "ModemCore.h"
#include <esp_task_wdt.h>   // For esp_task_wdt_reset() to feed WDT in long operations

// External hardware interface
extern HardwareSerial Modem;            // UART interface to modem

// ============================================
// Helper Functions
// ============================================

/**
 * Clear any pending data in modem RX buffer
 */
void clearModemRX() {
    while (Modem.available()) Modem.read();
}

/**
 * Send AT command and capture response
 * Waits for OK or ERROR response with timeout
 * Resets WDT periodically during wait
 * @param cmd - AT command to send
 * @param resp - Reference to string storing response
 * @param timeout - Maximum time to wait for response (ms)
 * @return true if OK returned, false if ERROR or timeout
 */
bool sendAT(const char* cmd, String &resp, uint32_t timeout) {

    Serial.print("[MODEM] >> ");
    Serial.println(cmd);

    resp = "";
    clearModemRX();
    Modem.println(cmd);

    uint32_t start = millis();

    while (millis() - start < timeout) {
        while (Modem.available()) resp += (char)Modem.read();

        if (resp.indexOf("OK") != -1) {
            Serial.println("[MODEM] OK");
            return true;
        }
        if (resp.indexOf("ERROR") != -1) {
            Serial.println("[MODEM] ERROR");
            return false;
        }

        delay(10);
    }

    Serial.println("[MODEM] TIMEOUT");
    return false;
}

/**
 * Send AT command without response details
 * Wrapper for sendAT() when capturing full response not needed
 * @param cmd - AT command to send
 * @param timeout - Maximum time to wait for response (ms)
 * @return true if OK returned, false otherwise
 */
bool sendATSimple(const char* cmd, uint32_t timeout) {
    String r;
    return sendAT(cmd, r, timeout);
}

/**
 * Perform hard power reset of modem module
 * Powers down for 3 seconds, then powers up
 * Waits 6 seconds for modem to stabilize after power on
 * Used when modem becomes unresponsive
 */
void rebootModemHard() {
    Serial.println("[MODEM] HARD RESET...");
    digitalWrite(SIM_PWR_RELAY_PIN, RELAY_OFF);
    delay(3000);
    digitalWrite(SIM_PWR_RELAY_PIN, RELAY_ON);
    delay(6000);
}

/**
 * Establish connection to cellular network
 * Process:
 * 1. Verify modem responds to AT command
 * 2. Disable echo and attach to network
 * 3. Configure APN (Access Point Name)
 * 4. Open network connection
 * Retries entire sequence up to MAX_NET_RETRY times
 * Performs hard reset if AT command fails
 * WDT is fed at each blocking step to prevent watchdog reset (WDT=90s, worst-case=114s)
 * @return true if connected successfully, false if all retries exhausted
 */
bool connectNetwork() {

    Serial.println("[NET] Connecting network...");
    String resp;

    // Retry loop for network connection attempts
    for (int i = 0; i < 3; i++) {

        esp_task_wdt_reset();  // Feed WDT at start of each retry
        Serial.printf("[NET] Try %d\n", i + 1);

        // Check if modem is responsive (retry AT command 5 times before Hard Reset)
        bool modem_ok = false;
        for (int atRetry = 0; atRetry < 5; atRetry++) {
            esp_task_wdt_reset();  // Feed WDT on each AT retry
            Serial.printf("[NET] AT test %d/5\n", atRetry + 1);
            if (sendAT("AT", resp, 2000)) {
                modem_ok = true;
                break;
            }
            Serial.println("[NET] AT fail, retrying...");
            delay(500);
        }

        if (!modem_ok) {
            Serial.println("[NET] Modem unresponsive after 5 retries -> Hard Reset");
            esp_task_wdt_reset();  // Feed WDT before reboot delay (9s)
            rebootModemHard();
            continue;
        }

        // Prepare modem for network connection
        esp_task_wdt_reset();  // Feed WDT before AT sequence
        sendATSimple("ATE0");                    // Disable echo
        sendATSimple("AT+CGATT=1", 3000);       // Attach to network
        delay(2000);                             // Wait for network registration to stabilize

        // Configure APN for data connection
        char cmd[64];
        sprintf(cmd, "AT+CGDCONT=1,\"IP\",\"%s\"", APN);
        sendATSimple(cmd);

        // Activate PDP context (required before NETOPEN on SIM7x00 series)
        esp_task_wdt_reset();
        sendATSimple("AT+CGACT=1,1", 10000);    // Activate PDP context #1

        // Close any existing socket before opening (ensure clean state)
        sendATSimple("AT+NETCLOSE", 5000);

        // Attempt to open network connection
        esp_task_wdt_reset();  // Feed WDT before NETOPEN (up to 15s)
        if (sendATSimple("AT+NETOPEN", 15000)) {
            Serial.println("[NET] Connected");
            modem_fail_count = 0;
            return true;
        }

        // NETOPEN failed — close existing socket before hard reset
        // ERROR = socket already open; TIMEOUT = modem stuck
        Serial.println("[NET] NETOPEN fail -> closing socket & reboot modem");
        sendATSimple("AT+NETCLOSE", 5000);  // Clean up open socket
        esp_task_wdt_reset();              // Feed WDT before reboot delay (9s)
        rebootModemHard();
    }

    modem_fail_count++;
    Serial.println("[NET] FAILED ALL RETRIES");
    return false;
}
