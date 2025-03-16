#include <assert.h>
#include <curl/curl.h>
#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/json.h>
#include <sys/neutrino.h>
#include <time.h>

#include "define.h"
#include "utils/sensors.h"
#include "utils/tools.h"

#include "rpi_gpio.h"
#include <unistd.h>

// Define the GPIO pin number (pin 36 on Raspberry Pi 4, which corresponds to
// GPIO16)
#define GPIO_PIN GPIO4

int main() {
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
    if (SchedGet(0, 0, &scheduling_params) != -1) {
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

    apiDetails *api = get_api_details();
    if (api == NULL) {
        printf("Terminating plantr sensor. \n");
        return -1;
    }

    sensorData *latestData = (sensorData *)malloc(sizeof(sensorData));
    if (latestData == NULL) {
        printf("Terminating plantr sensor. \n");
        return -1;
    }

    latestData->moisture = true;
    latestData->health = 123;
    latestData->waterLevel = 0;
    latestData->waterTime = NULL;

    for (;;) {
        rcvid = MsgReceive(chid, &msg, sizeof(msg), NULL);
        if (rcvid == 0) {
            if (msg.pulse.code == MY_PULSE_CODE) {
                latestData->waterLevel = updateCount * 3;
                post_sensor(api, latestData);

                updateCount++;
                printf("\nUPDATE COUNT: %d \n", updateCount);
                if (updateCount % 2 == 1) {
                    led_on(GPIO_PIN);
                } else {
                    led_off(GPIO_PIN);
                }
            }
        }
    }

    return (EXIT_SUCCESS);
}
