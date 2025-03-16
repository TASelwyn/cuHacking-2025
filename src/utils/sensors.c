#include <curl/curl.h>
#include <fcntl.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/ioctl.h>
#include <sys/json.h>
#include <sys/neutrino.h>
#include <sys/types.h>
#include <time.h>
#include <unistd.h>

#include "rpi_gpio.h"

#include "../define.h"
#include "../sensors/bmp280.h"

int initialize_sensors() {
    // init_bmt280();
    // init_dht22();
    init_gpio(GPIO27);

    return EXIT_SUCCESS;
}

sensorData *initialize_sensor_data() {
    sensorData *data = (sensorData *)malloc(sizeof(sensorData));
    if (data == NULL) {
        printf("Terminating plantr sensor. \n");
        return NULL;
    }

    data->temperature = 0;
    data->moisture = false;
    data->health = 0;
    data->waterLevel = 0;
    data->waterTime = NULL;

    return data;
}

int update_sensor_data(sensorData *data, int updateCount) {
    // gather data and update the struct here
    
    FILE *file;
    char filename[] = "numbers.txt";  // Change this to your file name
    char buffer[256];  // Buffer to store each line
    int number;

    // Open the file for reading
    file = fopen(filename, "r");
    if (file == NULL) {
        perror("Error opening file");
        return EXIT_FAILURE;
    }

    // Read each line, parse it as a number, and print it
    int lineNum = 0;
    while (fgets(buffer, sizeof(buffer), file) != NULL) {
        if (lineNum == updateCount % 50) {
            number = atoi(buffer);  // Convert string to integer
            printf("Read number: %d\n", number);
            data->health=number;
        }
        
        lineNum++;
    }

    // Close the file
    fclose(file);

    //data->health = 73;
    data->moisture = true;
    data->temperature = 24;
    return EXIT_SUCCESS;
}

int post_sensor_data(apiDetails *api, sensorData *data) {
    CURL *curl;
    CURLcode res;

    /* In Windows, this inits the Winsock stuff */
    curl_global_init(CURL_GLOBAL_ALL);

    /* get a curl handle */
    curl = curl_easy_init();

    if (curl) {
        struct curl_slist *headers = NULL;
        headers = curl_slist_append(headers, "Content-Type: application/json");

        json_encoder_t *enc = json_encoder_create();

        json_encoder_start_object(enc, NULL);
        json_encoder_add_string(enc, "query", SENSOR_API_QUERY_STR);

        json_encoder_start_object(enc, "variables");
        json_encoder_start_object(enc, "plantData");
        json_encoder_add_int(enc, "serialNum", api->serial_number);
        json_encoder_add_int(enc, "temperature", data->temperature);
        json_encoder_add_bool(enc, "moisture", data->moisture);
        json_encoder_add_int(enc, "health", data->health);
        json_encoder_add_int(enc, "waterLevel", data->waterLevel);
        json_encoder_add_string(enc, "waterTime", "qnx i2c is funky");
        json_encoder_end_object(enc);
        json_encoder_end_object(enc);

        json_encoder_end_object(enc);

        json_encoder_error_t status = json_encoder_get_status(enc);

        if (status != JSON_ENCODER_OK) {
            printf("Data preparation failed\n");
            return EXIT_FAILURE;
        }

        // Init post request
        curl_easy_setopt(curl, CURLOPT_URL, api->hostname);
        curl_easy_setopt(curl, CURLOPT_HTTPHEADER, headers);
        curl_easy_setopt(curl, CURLOPT_SSL_VERIFYPEER, 0);
        curl_easy_setopt(curl, CURLOPT_HTTPAUTH, CURLAUTH_BEARER);
        curl_easy_setopt(curl, CURLOPT_XOAUTH2_BEARER, api->bearer_token);
        curl_easy_setopt(curl, CURLOPT_USERAGENT, "Devilish Plantr/v4.20");
        curl_easy_setopt(curl, CURLOPT_POSTFIELDS, json_encoder_buffer(enc));

        /* Perform the request, res gets the return code */
        res = curl_easy_perform(curl);

        /* Check for errors */
        if (res != CURLE_OK)
            fprintf(stderr, "curl_easy_perform() failed: %s\n",
                    curl_easy_strerror(res));

        /* always cleanup */
        curl_easy_cleanup(curl);
        json_encoder_destroy(enc);
    }
    curl_global_cleanup();
    return EXIT_SUCCESS;
}
