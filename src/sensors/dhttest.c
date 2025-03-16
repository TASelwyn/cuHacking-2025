#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <unistd.h>
#include <pthread.h>
#include <sys/mman.h>
#include "rpi_gpio.h"

// DHT22 Pin and Timing Constants
#define DHT_GPIO             4       // Using GPIO4
#define DHT_START_HIGH      1001    // 1ms high before start
#define DHT_START_LOW       20000   // 20ms low for start signal
#define DHT_BIT_TIMEOUT     150     // Increased timeout for bit reading
#define DHT_RETRY_DELAY     3000000  // 3s between retries

// Add debug timing constants
#define DHT_BIT_0_HIGH      28      // Typical timing for '0' bit
#define DHT_BIT_1_HIGH      70      // Typical timing for '1' bit
#define DHT_BIT_LOW         50      // 50µs typical for bit low

// Adjusted refined timing constants
#define DHT_BIT_THRESHOLD   50      // Moved threshold up to be more conservative
#define HYSTERESIS          4       // Adjusted hysteresis

// DHT22 Sensor Characteristics
#define DHT22_TEMP_MAX     125.0f
#define DHT22_TEMP_MIN    -40.0f
#define DHT22_HUM_MAX     100.0f
#define DHT22_HUM_MIN       0.0f
#define DHT22_RESOLUTION    0.1f

typedef struct {
    int gpio_pin;
    uint8_t data[5];
} dht_sensor_t;

// Thread synchronization
static pthread_mutex_t gpio_mutex = PTHREAD_MUTEX_INITIALIZER;

// Get microsecond-precision timestamp
static uint64_t get_microseconds(void) {
    struct timespec ts;
    clock_gettime(CLOCK_MONOTONIC, &ts);
    return (uint64_t)ts.tv_sec * 1000000ULL + (uint64_t)ts.tv_nsec / 1000ULL;
}

// Wait for specified microseconds
static void delay_microseconds(uint32_t usec) {
    uint64_t start = get_microseconds();
    while ((get_microseconds() - start) < usec) {
        // Tight loop for precise timing
    }
}

// Modified read_bit function for cached timing and borderline hysteresis
static int read_bit(int gpio_pin) {
    unsigned level;
    uint64_t start = get_microseconds();
    
    // Wait for and measure low pulse
    while (rpi_gpio_input(gpio_pin, &level) == GPIO_SUCCESS && level == GPIO_HIGH) {
        uint64_t now = get_microseconds();
        if (now - start > DHT_BIT_TIMEOUT) {
            return -1;
        }
    }
    
    uint64_t low_start = get_microseconds();
    while (rpi_gpio_input(gpio_pin, &level) == GPIO_SUCCESS && level == GPIO_LOW) {
        uint64_t now = get_microseconds();
        if (now - low_start > DHT_BIT_TIMEOUT) {
            return -1;
        }
    }
    uint32_t low_time = get_microseconds() - low_start;
    
    // Measure high pulse with caching
    uint64_t high_start = get_microseconds();
    while (rpi_gpio_input(gpio_pin, &level) == GPIO_SUCCESS && level == GPIO_HIGH) {
        uint64_t now = get_microseconds();
        if (now - high_start > DHT_BIT_TIMEOUT) {
            return -1;
        }
    }
    uint32_t high_time = get_microseconds() - high_start;
    
    // Simple threshold-based decision
    int bit;
    if (high_time < 20) {  // Invalid reading
        return -1;
    }
    
    if (high_time < 35) {
        bit = 0;
    } else if (high_time > 65) {
        bit = 1;
    } else {
        bit = (high_time >= DHT_BIT_THRESHOLD);
    }
    
    return bit;
}

// Modified read_dht_sensor to restore priority immediately after reading bits and robust timing
static int read_dht_sensor(dht_sensor_t *sensor) {
    pthread_mutex_lock(&gpio_mutex);
    
    // Initial state
    if (rpi_gpio_setup(sensor->gpio_pin, GPIO_OUT) != GPIO_SUCCESS ||
        rpi_gpio_output(sensor->gpio_pin, GPIO_HIGH) != GPIO_SUCCESS) {
        goto error;
    }
    delay_microseconds(DHT_START_HIGH);
    
    // Start signal
    if (rpi_gpio_output(sensor->gpio_pin, GPIO_LOW) != GPIO_SUCCESS) goto error;
    delay_microseconds(DHT_START_LOW);
    if (rpi_gpio_output(sensor->gpio_pin, GPIO_HIGH) != GPIO_SUCCESS) goto error;
    
    // Switch to input and wait for response
    if (rpi_gpio_setup(sensor->gpio_pin, GPIO_IN) != GPIO_SUCCESS) goto error;
    
    // Wait for initial response
    unsigned level;
    uint64_t start = get_microseconds();
    
    // Wait for LOW
    while (rpi_gpio_input(sensor->gpio_pin, &level) == GPIO_SUCCESS && level == GPIO_HIGH) {
        if (get_microseconds() - start > DHT_BIT_TIMEOUT) {
            goto error;
        }
    }
    
    // Critical timing: set optimal realtime priority.
    ThreadCtl(_NTO_TCTL_IO, 0);  // Request I/O privileges
    struct sched_param param;
    int old_policy;
    pthread_getschedparam(pthread_self(), &old_policy, &param);
    param.sched_priority = sched_get_priority_max(SCHED_FIFO);
    pthread_setschedparam(pthread_self(), SCHED_FIFO, &param);
    
    // Read data - read 41 bits, skip first one
    memset(sensor->data, 0, sizeof(sensor->data));
    
    // Read and discard first bit
    if (read_bit(sensor->gpio_pin) < 0) {
        goto error;
    }
    
    // Now read the actual 40 bits of data
    for (int i = 0; i < 40; i++) {
        int bit = read_bit(sensor->gpio_pin);
        if (bit < 0) {
            goto error;
        }
        sensor->data[i/8] = (sensor->data[i/8] << 1) | bit;
    }
    
    // Restore original priority immediately after reading bits
    param.sched_priority = old_policy;
    pthread_setschedparam(pthread_self(), SCHED_FIFO, &param);
    
    // Verify checksum strictly
    uint8_t sum = sensor->data[0] + sensor->data[1] + sensor->data[2] + sensor->data[3];
    if (sensor->data[4] != sum) {
        goto error;
    }
    
    pthread_mutex_unlock(&gpio_mutex);
    return 0;

error:
    pthread_mutex_unlock(&gpio_mutex);
    return -1;
}

// Modified process_dht_data function with direct humidity division
static void process_dht_data(const dht_sensor_t *sensor, float *temp, float *humidity) {
    // Get raw humidity value and divide by 100 directly (no * 0.1 step needed)
    uint16_t hum_raw = (uint16_t)sensor->data[0] << 8 | sensor->data[1];
    *humidity = (float)hum_raw / 10.0f;  // Direct division to convert to percentage
    
    // Ensure humidity stays within valid range
    if (*humidity > DHT22_HUM_MAX) *humidity = DHT22_HUM_MAX;
    if (*humidity < DHT22_HUM_MIN) *humidity = DHT22_HUM_MIN;
    
    // Fix temperature calculation - ensure sign bit handling is correct
    uint16_t temp_raw = ((uint16_t)sensor->data[2] & 0x7F) << 8 | sensor->data[3];
    *temp = (float)temp_raw * 0.1f;  // Convert from 10ths of degree to degrees
    if (sensor->data[2] & 0x80) {    // Check sign bit
        *temp = -*temp;              // Make negative if sign bit is set
    }
}

// Modified to return both temperature and humidity values
int run_dht_test(float *temperature, float *humidity) {
    if (!temperature || !humidity) {
        return -1;
    }

    dht_sensor_t sensor = { .gpio_pin = DHT_GPIO };
    
    // Lock memory to prevent page faults during timing-critical sections
    mlockall(MCL_CURRENT | MCL_FUTURE);
    
    for (int retry = 0; retry < 5; retry++) {
        if (read_dht_sensor(&sensor) == 0) {
            process_dht_data(&sensor, temperature, humidity);
            return 0;
        }
        
        if (retry < 4) {  // Don't sleep on last attempt
            usleep(DHT_RETRY_DELAY);
        }
    }
    
    return -1;
}