#ifndef MODEM_CORE_H
#define MODEM_CORE_H

#include <Arduino.h>

// ============================================
// MODEM CONFIGURATION PINS & SETTINGS
// ============================================
#define APN "internet"              // Access Point Name for cellular connection
#define MODEM_RX 16                 // Modem UART RX pin
#define MODEM_TX 17                 // Modem UART TX pin
#define SIM_PWR_RELAY_PIN 32        // SIM card power relay pin
#define RELAY_ON HIGH                // Logic level to turn relay ON
#define RELAY_OFF LOW              // Logic level to turn relay OFF

// ============================================
// MODEM AT COMMAND FUNCTIONS
// ============================================
/**
 * Send AT command to modem and wait for response
 * @param cmd - AT command string to send
 * @param resp - Reference to string that will store the response
 * @param timeout - Timeout in milliseconds (default 5000ms)
 * @return true if OK received, false if ERROR or timeout
 */
bool sendAT(const char* cmd, String &resp, uint32_t timeout = 5000);

/**
 * Send AT command without capturing response details
 * @param cmd - AT command string to send
 * @param timeout - Timeout in milliseconds (default 5000ms)
 * @return true if OK received, false otherwise
 */
bool sendATSimple(const char* cmd, uint32_t timeout = 5000);

/**
 * Clear any pending data in modem RX buffer
 */
void clearModemRX();

/**
 * Perform hard reset of modem via power relay
 * Powers off for 3 seconds, then powers on for 6 seconds
 */
void rebootModemHard();

// ============================================
// NETWORK CONNECTION FUNCTIONS
// ============================================
/**
 * Connect to cellular network
 * Retries connection up to MAX_NET_RETRY times
 * Performs hard reset if connection fails
 * @return true if successfully connected, false if all retries failed
 */
bool connectNetwork();

// ============================================
// EXTERNAL STATE VARIABLES
// ============================================
extern char HW_sim_status[16];          // SIM card status ("Normal" or "Error")
extern uint32_t modem_fail_count;       // Count of modem connection failures

#endif
