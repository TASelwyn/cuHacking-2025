#include "../define.h"
#include <curl/curl.h>
#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/json.h>
#include <sys/neutrino.h>
#include <time.h>

#include "rpi_gpio.h"

char *get_env_var(char *env_var_key) {
    char *env_var_value = getenv(env_var_key);

    if (env_var_value) {
        printf("ENV %s found: %s \n", env_var_key, env_var_value);
    } else {
        printf("ENV %s NOT Found. \n", env_var_key);
        return NULL;
    }

    return env_var_value;
}

apiDetails *get_api_details() {
    char *apiHost = get_env_var(SENSOR_API_HOSTNAME);
    if (apiHost == NULL) {
        printf("Terminating plantr remote sensor. \n");
        return NULL;
    }

    char *apiKey = get_env_var(SENSOR_API_AUTH_KEY);
    if (apiKey == NULL) {
        printf("Terminating plantr remote sensor. \n");
        return NULL;
    }

    char *sensorSerialChar = get_env_var(SENSOR_SERIAL_NUM);
    if (sensorSerialChar == NULL) {
        printf("Terminating plantr remote sensor. \n");
        return NULL;
    }
    char *endptr;
    int serialNum = (int)strtol(sensorSerialChar, &endptr, 10);

    apiDetails *api = (apiDetails *)malloc(sizeof(apiDetails));
    api->hostname = apiHost;
    api->bearer_token = apiKey;
    api->serial_number = serialNum;

    return api;
}

bool read_gpio_in(int gpio_pin) {
    /*if (rpi_gpio_read(gpio_pin)) {
        perror("rpi_gpio_read");
        return false;
    }*/

    return true;
}

bool init_gpio(int gpio_pin, enum gpio_config_t gpio_inout) {
    if (rpi_gpio_setup(gpio_pin, gpio_inout)) {
        perror("rpi_gpio_setup");
        return false;
    }

    return true;
}

bool set_gpio_out(int gpio_pin, enum gpio_config_t gpio_LOW_OR_HIGH) {
    if (rpi_gpio_output(gpio_pin, gpio_LOW_OR_HIGH)) {
        perror("rpi_gpio_output");
        return false;
    }

    return true;
}
