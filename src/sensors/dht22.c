/**
 * DHT22 Temperature and Humidity Sensor Reader for QNX 8.0 on Raspberry Pi 4B
 */

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <time.h>
#include "dht22.h"
#include "rpi_gpio.h"  // Include the provided Raspberry Pi GPIO API

#define DHT22_PIN 4           // GPIO pin connected to DHT22 data line
#define MAX_TIMINGS 85        // Maximum number of timing transitions to expect
#define DHT22_TIMEOUT_USEC 100 // Maximum microseconds to wait for state change

// Function to delay microseconds
void delay_us(unsigned int us) {
    struct timespec ts;
    ts.tv_sec = us / 1000000;
    ts.tv_nsec = (us % 1000000) * 1000;
    nanosleep(&ts, NULL);
}

// Read data from DHT22 sensor
int read_dht22_data(float *humidity, float *temperature) {
    int data[5] = {0, 0, 0, 0, 0};
    unsigned level;
    int bit = 0;
    int i = 0;
    
    // Set up GPIO pin
    if (rpi_gpio_setup(DHT22_PIN, GPIO_OUT) != GPIO_SUCCESS) {
        fprintf(stderr, "Failed to set up GPIO pin for output\n");
        return -1;
    }
    
    // Send start signal
    rpi_gpio_output(DHT22_PIN, GPIO_LOW);
    delay_us(1000);  // Hold low for at least 1ms to start
    rpi_gpio_output(DHT22_PIN, GPIO_HIGH);
    delay_us(40);    // DHT22 needs a short delay before it takes over the bus
    
    // Switch to input mode to read sensor response
    if (rpi_gpio_setup(DHT22_PIN, GPIO_IN) != GPIO_SUCCESS) {
        fprintf(stderr, "Failed to set up GPIO pin for input\n");
        return -1;
    }
    
    // Configure pull-up resistor (optional if you have an external one)
    rpi_gpio_setup_pull(DHT22_PIN, GPIO_IN, GPIO_PUD_UP);
    
    // Wait for DHT22 to pull the line low (ACK)
    struct timespec start, now;
    long elapsed_us;
    clock_gettime(CLOCK_MONOTONIC, &start);
    
    do {
        rpi_gpio_input(DHT22_PIN, &level);
        clock_gettime(CLOCK_MONOTONIC, &now);
        elapsed_us = (now.tv_sec - start.tv_sec) * 1000000 + (now.tv_nsec - start.tv_nsec) / 1000;
        if (elapsed_us > DHT22_TIMEOUT_USEC) {
            fprintf(stderr, "DHT22 timed out waiting for response\n");
            return -2;
        }
    } while (level == GPIO_HIGH);
    
    // DHT22 sends 40 bits of data: 16 bits RH, 16 bits temp, 8 bits checksum
    // Each bit starts with a 50us LOW followed by HIGH (70us for '1', 27us for '0')
    for (i = 0; i < MAX_TIMINGS; i++) {
        // Wait for pin to go high (end of LOW pulse)
        clock_gettime(CLOCK_MONOTONIC, &start);
        do {
            rpi_gpio_input(DHT22_PIN, &level);
            clock_gettime(CLOCK_MONOTONIC, &now);
            elapsed_us = (now.tv_sec - start.tv_sec) * 1000000 + (now.tv_nsec - start.tv_nsec) / 1000;
            if (elapsed_us > DHT22_TIMEOUT_USEC) goto timeout;
        } while (level == GPIO_LOW);
        
        // Measure duration of HIGH pulse
        clock_gettime(CLOCK_MONOTONIC, &start);
        do {
            rpi_gpio_input(DHT22_PIN, &level);
            clock_gettime(CLOCK_MONOTONIC, &now);
            elapsed_us = (now.tv_sec - start.tv_sec) * 1000000 + (now.tv_nsec - start.tv_nsec) / 1000;
            if (elapsed_us > DHT22_TIMEOUT_USEC) goto timeout;
        } while (level == GPIO_HIGH);
        
        // If pulse width is greater than ~50us, it's a '1'
        if (elapsed_us > 50) {
            data[bit / 8] |= (1 << (7 - (bit % 8)));
        }
        
        bit++;
        if (bit == 40) break;  // We've read all 40 bits
    }
    
    // Verify checksum
    if (bit != 40) {
        fprintf(stderr, "Data incomplete from DHT22 (received %d bits)\n", bit);
        return -3;
    }
    
    if ((data[0] + data[1] + data[2] + data[3]) & 0xFF != data[4]) {
        fprintf(stderr, "DHT22 checksum failed\n");
        return -4;
    }
    
    // Convert the data
    *humidity = ((data[0] << 8) + data[1]) / 10.0;
    
    // Temperature is signed, check if negative
    if (data[2] & 0x80) {
        *temperature = -((((data[2] & 0x7F) << 8) + data[3]) / 10.0);
    } else {
        *temperature = ((data[2] << 8) + data[3]) / 10.0;
    }
    
    return 0;
    
timeout:
    fprintf(stderr, "DHT22 read timed out\n");
    return -5;
}

int run_dht22() {
    float humidity, temperature;
    
    printf("DHT22 Temperature and Humidity Reader\n");
    
    // Try to read the DHT22 sensor
    while (1) {
        int result = read_dht22_data(&humidity, &temperature);
        
        if (result == 0) {
            printf("Temperature: %.1f°C, Humidity: %.1f%%\n", temperature, humidity);
        } else {
            printf("Failed to read from DHT22 sensor, error code: %d\n", result);
        }
        
        // DHT22 sampling rate is 0.5Hz (once every 2 seconds)
        sleep(2);
    }
    
    return 0;
}