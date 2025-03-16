#ifndef DHT_SENSOR_H
#define DHT_SENSOR_H

#include <stdint.h>

// ----- Configuration Constants ----- //
#define DHT_GPIO            GPIO4   // The GPIO pin connected to the DHT sensor
#define DHT_NUM_BYTES       5       // The sensor transmits 5 bytes (40 bits)
#define DHT_TIMEOUT_US      1000    // Timeout for pulse measurements (in µs)
#define DHT_START_DELAY_MS  20      // Start signal duration (20ms for DHT11)
#define DHT_MAX_RETRIES     5       // Number of retries on failed read

// ----- Data Structure for DHT Sensor ----- //
typedef struct {
    int gpio_pin;               // GPIO pin connected to the sensor
    uint8_t data[DHT_NUM_BYTES]; // Buffer to store 40-bit sensor data
} dht_sensor_t;

// ----- Function Prototypes ----- //
/**
 * @brief Initializes a DHT sensor structure.
 * @param sensor Pointer to a dht_sensor_t structure.
 * @param gpio_pin GPIO pin number where the sensor is connected.
 */
void dht_init(dht_sensor_t *sensor, int gpio_pin);

/**
 * @brief Reads temperature and humidity data from the DHT sensor.
 * @param sensor Pointer to a dht_sensor_t structure.
 * @return 0 on success, -1 on failure (timeout, checksum error, etc.).
 */
int read_dht(dht_sensor_t *sensor);

/**
 * @brief Gets the current time in microseconds.
 * @return Current time in microseconds.
 */
uint32_t get_microseconds();

/**
 * @brief Measures the duration of a GPIO pulse.
 * @param expected_level The expected GPIO level (HIGH or LOW).
 * @return Pulse duration in microseconds, or 0 on timeout.
 */
uint32_t expect_pulse(uint8_t expected_level);

#endif // DHT_SENSOR_H
