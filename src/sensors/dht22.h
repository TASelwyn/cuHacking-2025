/**
 * DHT22 Temperature and Humidity Sensor Reader for QNX 8.0 on Raspberry Pi 4B
 */

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <time.h>
#include "rpi_gpio.h"  // Include the provided Raspberry Pi GPIO API

#define DHT22_PIN 4           // GPIO pin connected to DHT22 data line
#define MAX_TIMINGS 85        // Maximum number of timing transitions to expect
#define DHT22_TIMEOUT_USEC 100 // Maximum microseconds to wait for state change

// Function to delay microseconds
void delay_us(unsigned int us);
// Read data from DHT22 sensor
int read_dht22_data(float *humidity, float *temperature);
int run_dht22();