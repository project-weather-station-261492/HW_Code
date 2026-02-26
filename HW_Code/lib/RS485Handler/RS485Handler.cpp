#include "RS485Handler.h"

// External modbus interface
extern ModbusRTUMaster modbus;          // Modbus RTU master instance
char HW_rs485_status[5][16];            // Status for slaves 0-4 (index 0 unused)
bool HW_s4_bme_ok  = false;            // BME280 status from Slave4 Addr0 bit0
bool HW_s4_veml_ok = false;            // VEML770 status from Slave4 Addr0 bit1

// ============================================
// RS485/MODBUS FUNCTIONS
// ============================================

/**
 * Initialize RS485 serial interface for Modbus RTU communication
 * Configures:
 * - UART1 on pins RX1/TX1
 * - Baud rate: 9600
 * - Format: 8N1 (8 bits, no parity, 1 stop bit)
 * - DE pin for RS485 driver enable control
 */
void initRS485() {
    Serial.println("[INIT] RS485 UART");
    Serial1.begin(9600, SERIAL_8N1, RX1, TX1);
    modbus.begin(9600);
    modbus.setTimeout(1000);  // 1000ms response timeout (default was only 100ms)
}

/**
 * Test all RS485 slave devices by reading their first holding register
 * Tests all 4 slaves (IDs 1-4)
 * Updates HW_rs485_status array with result for each slave
 * Status values: "Normal" (read successful) or "Error" (read failed)
 * Useful for hardware diagnostics and startup verification
 */
void testRS485() {
    Serial.println("[RS485] Testing devices...");
    uint16_t buf[1];

    // Test Slaves 1-3 with FC3 (Read Holding Registers)
    for (int id = 1; id <= 3; id++) {
        Serial.printf("[RS485] Slave %d ... ", id);
        if (modbus.readHoldingRegisters(id, 0, buf, 1)) {
            strcpy(HW_rs485_status[id], "Normal");
            Serial.println("OK");
        } else {
            strcpy(HW_rs485_status[id], "Error");
            Serial.println("FAIL");
        }
        delay(500);
    }

    // Test Slave 4 with FC4 (Read Input Registers) — BME280 + VEML770
    // Addr 0 = Status register: bit0=BME_ERR, bit1=VEML_ERR (0 = OK)
    Serial.printf("[RS485] Slave 4 ... ");
    if (modbus.readInputRegisters(4, 0, buf, 1)) {
        strcpy(HW_rs485_status[4], "Normal");
        uint16_t status = buf[0];
        HW_s4_bme_ok  = !(status & 0x01);   // bit0: 0 = BME280 OK
        HW_s4_veml_ok = !(status & 0x02);   // bit1: 0 = VEML770 OK
        Serial.printf("OK (status=0x%02X BME280=%s VEML770=%s)\n",
                      status,
                      HW_s4_bme_ok  ? "OK" : "ERR",
                      HW_s4_veml_ok ? "OK" : "ERR");
    } else {
        strcpy(HW_rs485_status[4], "Error");
        HW_s4_bme_ok  = false;
        HW_s4_veml_ok = false;
        Serial.println("FAIL");
    }
    delay(500);
}

/**
 * Read a holding register from an RS485 Modbus slave device
 * Implements retry logic to handle temporary communication issues
 * @param id - Modbus slave address (1-4)
 * @param reg - Holding register address to read
 * @param val - Pointer to uint16_t to store the read value
 * @return true if read successful, false if all 3 retry attempts failed
 */
bool readReg(int id, int reg, uint16_t* val) {
    uint16_t buf[1];

    // Retry up to 3 times with 200ms delay between attempts
    for (int i = 0; i < 3; i++) {
        if (modbus.readHoldingRegisters(id, reg, buf, 1)) {
            *val = buf[0];
            Serial.printf("[RS485] id%d reg%d = %u\n", id, reg, *val);
            return true;
        }
        delay(200);  // Brief delay before retry
    }

    // Log failure after all retries exhausted
    Serial.printf("[RS485] READ FAIL id%d reg%d\n", id, reg);
    return false;
}

/**
 * Read Slave 4 input registers using Modbus FC4 (Read Input Registers)
 * Single request reads Addr 0..4 (Qty = 5) from SlaveID = 4
 *
 * Register map:
 *   Addr 0  Status  bit0 = BME280_ERR, bit1 = VEML770_ERR (0 = OK)
 *   Addr 1  Temperature  × 100  (divide by 100.0 for °C)
 *   Addr 2  Humidity     × 100  (divide by 100.0 for %)
 *   Addr 3  Pressure     × 10   (divide by 10.0  for hPa)
 *   Addr 4  Lux          raw
 *
 * @param out - Pointer to Slave4Data struct to populate
 * @return true on success, false after 3 failed attempts
 */
bool readSlave4(Slave4Data* out) {
    uint16_t buf[5];

    for (int i = 0; i < 3; i++) {
        if (modbus.readInputRegisters(4, 0, buf, 5)) {
            uint16_t status = buf[0];
            out->ok_bme       = !(status & 0x01);   // bit0 = 0 → BME OK
            out->ok_veml      = !(status & 0x02);   // bit1 = 0 → VEML OK
            out->temperature  = buf[1];
            out->humidity     = buf[2];
            out->pressure     = buf[3];
            out->lux          = buf[4];

            Serial.printf("[RS485] Slave4: status=0x%02X t=%u h=%u p=%u lx=%u\n",
                          status,
                          out->temperature, out->humidity,
                          out->pressure, out->lux);
            return true;
        }
        delay(200);
    }

    Serial.println("[RS485] READ FAIL id4 (FC4 block read)");
    return false;
}
