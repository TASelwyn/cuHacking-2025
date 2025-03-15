#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <pthread.h>
#include <sys/neutrino.h>
#include <curl/curl.h>
#include <string.h>
#include "define.h"

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

int post_sensor(char* apiHost, char* apiKey, char* apiSerial) {
    CURL *curl;
    CURLcode res;
   
    /* In Windows, this inits the Winsock stuff */
    curl_global_init(CURL_GLOBAL_ALL);
   
    /* get a curl handle */
    curl = curl_easy_init();
    if (curl) {

      struct curl_slist *headers = NULL;
      headers = curl_slist_append(headers, "Content-Type: application/json");
      // Init post request
      curl_easy_setopt(curl, CURLOPT_URL, apiHost);
      curl_easy_setopt(curl, CURLOPT_HTTPHEADER, headers);
      curl_easy_setopt(curl, CURLOPT_HTTPAUTH, CURLAUTH_BEARER);
      curl_easy_setopt(curl, CURLOPT_XOAUTH2_BEARER, apiKey);
      curl_easy_setopt(curl, CURLOPT_USERAGENT, "Devilish Plantr/0.69");
      curl_easy_setopt(curl, CURLOPT_POSTFIELDS, "sensor=69420&project=curl");
   
      /* Perform the request, res gets the return code */
      res = curl_easy_perform(curl);
      /* Check for errors */
      if(res != CURLE_OK)
        fprintf(stderr, "curl_easy_perform() failed: %s\n",
                curl_easy_strerror(res));
   
      /* always cleanup */
      curl_easy_cleanup(curl);
    }
    curl_global_cleanup();
    return 0;
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
    itime.it_interval.tv_sec = SENSOR_INTERVAL_SECS;
    itime.it_interval.tv_nsec = 0;
    timer_settime(timer_id, 0, &itime, NULL);

    char* apiHost = get_env_var(SENSOR_API_HOSTNAME); 
    if (apiHost == NULL) { printf("Terminating sensor. \n"); return -1;}

    char* apiKey = get_env_var(SENSOR_API_API_KEY); 
    if (apiKey == NULL) { printf("Terminating sensor. \n"); return -1;}

    char* apiSerial = get_env_var(SENSOR_API_SERIAL); 
    if (apiSerial == NULL) { printf("Terminating sensor. \n"); return -1;}

    for (;;) {
        rcvid = MsgReceive(chid, &msg, sizeof(msg), NULL);
        if (rcvid == 0) { /* we got a pulse */
            if (msg.pulse.code == MY_PULSE_CODE) {
            	//fetch_quote();
                post_sensor(apiHost, apiKey, apiSerial);
                printf("POST API KEY: %s \n", apiKey);
            }
        }
    }

    return(EXIT_SUCCESS);
}
