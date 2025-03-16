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

typedef union {
    struct _pulse pulse;
} my_message_t;

int main() {
    printf("--------------------------------------- \n");
    printf("Starting Plantr remote sensor v0.1... \n");

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
    init_gpio(GPIO_LED, GPIO_OUT);
    init_gpio(GPIO_WATER, GPIO_IN);

    printf("Prepping sensor data storage... \n");
    sensorData *latestData = initialize_sensor_data();
    assert(latestData != NULL);

    printf("Loading API Details... \n");
    apiDetails *api = get_api_details();
    assert(api != NULL);

    printf("Initializing sensors... \n");
    int initSens = initialize_sensors();
    assert(!initSens);

    for (;;) {
        rcvid = MsgReceive(chid, &msg, sizeof(msg), NULL);
        if (rcvid == 0) {
            if (msg.pulse.code == MY_PULSE_CODE) {
                latestData->waterLevel = updateCount * 3;

                printf("Updating sensor data... \n");
                update_sensor_data(latestData, updateCount);
                printf("Posting sensor data... \n");
                post_sensor_data(api, latestData);

                updateCount++;
                printf("\npostSensorDataSent: %d \n", updateCount);
                if (updateCount % 2 == 1) {
                //if (read_gpio_in(GPIO_WATER)) {
                    set_gpio_out(GPIO_LED, GPIO_HIGH);
                } else {
                    set_gpio_out(GPIO_LED, GPIO_LOW);
                }
            }
        }
    }

    return (EXIT_SUCCESS);
}
