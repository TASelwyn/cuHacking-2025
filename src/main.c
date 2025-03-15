#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <pthread.h>
#include <string.h>
#include <sys/neutrino.h>
#include <curl/curl.h>
#include <sys/json.h>
#include <assert.h>

#include "define.h"
#include "utils/tools.h"
#include "utils/sensors.h"

#include <unistd.h>
#include "rpi_gpio.h"
 
// Define the GPIO pin number (pin 36 on Raspberry Pi 4, which corresponds to GPIO16)
#define GPIO_PIN GPIO4

// Initializes the specified GPIO pin for controlling an LED by setting it as an output pin.
static bool init_led(int gpio_pin)
{
// Configure the given GPIO pin as an output pin.
if (rpi_gpio_setup(gpio_pin, GPIO_OUT))
{
    perror("rpi_gpio_setup");
    return false;
}

return true;
}

// Turns on the LED connected to the specified GPIO pin.
static bool led_on(int gpio_pin)
{
// Set selected GPIO to high.
if (rpi_gpio_output(gpio_pin, GPIO_HIGH))
{
    perror("rpi_gpio_output");
    return false;
}

return true;
}

// Turns off the LED connected to the specified GPIO pin.
static bool led_off(int gpio_pin)
{
// Set selected GPIO to low.
if (rpi_gpio_output(gpio_pin, GPIO_LOW))
{
    perror("rpi_gpio_output");
    return false;
}

return true;
}
int main()
{
    printf("Starting plantr sensor... \n");

    struct sigevent event;
    struct itimerspec itime;
    timer_t timer_id;
    int chid;
    rcvid_t rcvid;
    my_message_t msg;
    struct sched_param scheduling_params;
    int prio;

    chid = ChannelCreate(0);

    /* Get our priority. */
    if (SchedGet( 0, 0, &scheduling_params) != -1) {
        prio = scheduling_params.sched_priority;
    } else {
        prio = 10;
    }

    int coid = ConnectAttach(0, 0, chid, _NTO_SIDE_CHANNEL, 0);
    SIGEV_PULSE_INIT(&event, coid, prio, MY_PULSE_CODE, 0);

    timer_create(CLOCK_MONOTONIC, &event, &timer_id);

    itime.it_value.tv_sec = 2;
    itime.it_value.tv_nsec = 500 * 1E6;
    itime.it_interval.tv_sec = SENSOR_UPDATE_INTERVAL;
    itime.it_interval.tv_nsec = 0;
    timer_settime(timer_id, 0, &itime, NULL);

    int updateCount = 0;
    init_led(GPIO_PIN);
    apiDetails* api = get_api_details();
    if (api == NULL) { printf("Terminating plantr sensor. \n"); return -1;}

    sensorData* latestData = (sensorData*) malloc(sizeof(sensorData));
    assert(latestData != NULL);

    latestData->moisture = true;
    latestData->health = 123;
    latestData->waterLevel = 0;
    latestData->waterTime = NULL;

    for (;;) {
        rcvid = MsgReceive(chid, &msg, sizeof(msg), NULL);
        if (rcvid == 0) {
            if (msg.pulse.code == MY_PULSE_CODE) {
                latestData->waterLevel = updateCount*3;
                //post_sensor(latestData, updateCount);

                updateCount++;
                printf("UPDATE COUNT: %d \n", updateCount);
                if (updateCount %2 == 1) {
                    led_on(GPIO_PIN);
                } else {
                    led_off(GPIO_PIN);
                }
            }
        }
    }

    return(EXIT_SUCCESS);
}
