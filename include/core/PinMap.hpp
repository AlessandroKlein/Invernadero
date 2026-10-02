#pragma once
// Mapa de pines por defecto del controlador.
// Estos valores son los asignados en el PCB/DevKit de referencia.
// Pueden modificarse sin tocar la lógica del firmware (solo cambia el hardware).

namespace gh::pins {

// --- Bus I²C (sensores: SHT31, AHT20, ADS1115, BH1750, MCP23017) ---
constexpr int I2C_SDA = 21;
constexpr int I2C_SCL = 22;
constexpr uint32_t I2C_FREQ = 100000; // 100 kHz

// --- Bus SPI para 74HC595 (expansión de salidas, sección 21) ---
constexpr int HC595_MOSI = 23;  // DATA  -> DS
constexpr int HC595_SCLK = 18;  // CLOCK -> SHCP
constexpr int HC595_LATCH = 5;  // LATCH -> STCP
constexpr int HC595_COUNT = 4;  // 4 x 74HC595 = 32 salidas

// --- Bus SPI nativo (V8.1: MCP23S17, ADC externo, SD, W5500) ---
// Nota de compatibilidad: el 74HC595 usa SPI bit-banged (23/18/5). El SPI nativo
// puede compartir MISO/MOSI/SCK, pero no debe operar a la vez que el 595 si
// comparten los mismos GPIO. Los pines CS se asignan por instalación, evitando
// los GPIO de strapping (0, 2, 12, 15 en ESP32 clásico).
constexpr int SPI_SCK  = 18;
constexpr int SPI_MISO = 19;
constexpr int SPI_MOSI = 23;

// --- Bus 1-Wire (DS18B20) ---
constexpr int ONEWIRE_PIN = 4;  // con pull-up de 4.7 kΩ a 3.3 V (común a todos)

// --- Entradas de pulsos (caudal, lluvia, viento) ---
constexpr int FLOW_PIN = 34;    // Caudalímetro (pulso por litro)
constexpr int RAIN_PIN = 35;    // Pluviómetro (cazoleta)
constexpr int WIND_PIN = 36;    // Anemómetro (pulso por rotación)

// --- Nivel de depósito (ultrasónico + flotadores de seguridad) ---
constexpr int TANK_TRIG = 25;   // Trigger del sensor ultrasónico
constexpr int TANK_ECHO = 26;   // Echo del sensor ultrasónico
constexpr int FLOAT_LOW_PIN = 32;  // Flotador nivel bajo (seguridad)
constexpr int FLOAT_HIGH_PIN = 33; // Flotador nivel alto (seguridad)

// --- Entradas de seguridad (sección 25) ---
constexpr int EMERGENCY_STOP_PIN = 27; // Parada de emergencia

// --- RS485 / Modbus RTU (sección 18.2) ---
constexpr int RS485_RX = 16;     // RO
constexpr int RS485_TX = 17;     // DI
constexpr int RS485_DE = 14;     // DE/RE (control de dirección del transceiver)

// --- Direcciones I²C ---
constexpr uint8_t I2C_ADDR_SHT31 = 0x44;
constexpr uint8_t I2C_ADDR_AHT20 = 0x38;
constexpr uint8_t I2C_ADDR_ADS1115 = 0x48; // A0..A2 = 0x48..0x4B
constexpr uint8_t I2C_ADDR_BH1750 = 0x23;
constexpr uint8_t I2C_ADDR_SCD41 = 0x62;
constexpr uint8_t I2C_ADDR_MCP23017_1 = 0x20; // A0=A1=A2=0
constexpr uint8_t I2C_ADDR_MCP23017_2 = 0x21; // A0=1

} // namespace gh::pins
