#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <pthread.h>
#include <string.h>
#include <sys/neutrino.h>
#include <curl/curl.h>
#include <sys/json.h>
#include "define.h"

// Plantr Vars
char* apiHost;
char* apiKey;
int serialNum;

char* get_env_var(char* env_var_key) {
    char* env_var_value = getenv(env_var_key);

    if (env_var_value) {
        printf("ENV %s found: %s \n", env_var_key,  env_var_value);
    } else {
        printf("ENV %s NOT Found. \n", env_var_key);
        return NULL;
    }

    return env_var_value;
}

int post_sensor(sensorData* data, int updateCount) {
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
                    json_encoder_add_int(enc, "serialNum", serialNum);
                    json_encoder_add_bool(enc, "moisture", data->moisture);
                    json_encoder_add_int(enc, "health", data->health);
                    json_encoder_add_int(enc, "waterLevel", data->waterLevel*3);
                    json_encoder_add_string(enc, "waterTime", "Shall be null");
                json_encoder_end_object(enc);
            json_encoder_end_object(enc);

        json_encoder_end_object(enc);

        json_encoder_error_t status = json_encoder_get_status(enc);
        
        if ( status != JSON_ENCODER_OK ) {
            printf("Data preparation failed\n");
            return false;
        }

        // Init post request
        curl_easy_setopt(curl, CURLOPT_URL, apiHost);
        curl_easy_setopt(curl, CURLOPT_HTTPHEADER, headers);
        curl_easy_setopt(curl, CURLOPT_SSL_VERIFYPEER, 0);
        curl_easy_setopt(curl, CURLOPT_HTTPAUTH, CURLAUTH_BEARER);
        curl_easy_setopt(curl, CURLOPT_XOAUTH2_BEARER, apiKey);
        curl_easy_setopt(curl, CURLOPT_USERAGENT, "Devilish Plantr/0.69");        
        curl_easy_setopt(curl, CURLOPT_POSTFIELDS, json_encoder_buffer(enc));

        /* Perform the request, res gets the return code */
        res = curl_easy_perform(curl);
        
        /* Check for errors */
        if(res != CURLE_OK)
        fprintf(stderr, "curl_easy_perform() failed: %s\n",
                curl_easy_strerror(res));

        /* always cleanup */
        curl_easy_cleanup(curl);
        json_encoder_destroy(enc);
    }
    curl_global_cleanup();
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

    apiHost = get_env_var(SENSOR_API_HOSTNAME); 
    if (apiHost == NULL) { printf("Terminating sensor. \n"); return -1;}

    apiKey = get_env_var(SENSOR_API_AUTH_KEY); 
    if (apiKey == NULL) { printf("Terminating sensor. \n"); return -1;}

    char* sensorSerialChar = get_env_var(SENSOR_SERIAL_NUM); 
    if (sensorSerialChar == NULL) { printf("Terminating sensor. \n"); return -1;}
    char *endptr;      
    serialNum = (int) strtol(sensorSerialChar, &endptr, 10);

    int updateCount = 0;

    sensorData* latestData = (sensorData*) malloc(sizeof(sensorData));

    latestData->moisture = true;
    latestData->health = 123;
    latestData->waterLevel = 0;
    latestData->waterTime = NULL;

    for (;;) {
        rcvid = MsgReceive(chid, &msg, sizeof(msg), NULL);
        if (rcvid == 0) {
            if (msg.pulse.code == MY_PULSE_CODE) {
                latestData->waterLevel = updateCount*3;
                post_sensor(latestData, updateCount);

                updateCount++;
                printf("UPDATE COUNT: %d \n", updateCount);
            }
        }
    }

    return(EXIT_SUCCESS);
}
