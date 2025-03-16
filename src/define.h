#ifndef DEFINE
#define DEFINE

#define SENSOR_API_HOSTNAME "PLANTR_API_HOSTNAME"
#define SENSOR_API_AUTH_KEY "PLANTR_API_KEY"
#define SENSOR_SERIAL_NUM   "PLANTR_SERIAL_NUM"
#define SENSOR_UPDATE_INTERVAL 10
#define GPIO_LED           GPIO17
#define GPIO_WATER         GPIO27
 
#define SENSOR_API_QUERY_STR "mutation UpsertPlant($plantData: UpsertPlantDataInput) { upsert(plantData: $plantData) { success result errors { message } } }"

#define MY_PULSE_CODE   _PULSE_CODE_MINAVAIL

#define handle_error(msg) \
	do {perror(msg); exit(EXIT_FAILURE);} while(0)

typedef struct {
    char* hostname;
    char* bearer_token;
    int serial_number;
} apiDetails;

typedef struct {
    int temperature;
    int moisture;
    int health;
    int waterLevel;
    int humidity;
} sensorData;
#endif