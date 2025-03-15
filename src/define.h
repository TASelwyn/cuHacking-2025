#define SENSOR_API_HOSTNAME "PLANTR_API_HOSTNAME"
#define SENSOR_API_API_KEY  "PLANTR_API_KEY"
#define SENSOR_API_SERIAL   "PLANTR_SERIAL_NUM"
#define SENSOR_API_INTERVAL 15

#define SENSOR_API_QUERY_STR "mutation UpsertPlant($plantData: UpsertPlantDataInput) { upsert(plantData: $plantData) { success result errors { message } } }"

#define MY_PULSE_CODE   _PULSE_CODE_MINAVAIL

#define handle_error(msg) \
	do {perror(msg); exit(EXIT_FAILURE);} while(0)

typedef union {
    struct _pulse   pulse;
} my_message_t;


typedef struct {
	char *data;
	size_t size;
} RESPONSE;

typedef struct {
    int serialNum;
    bool moisture;
    int health;
    int waterLevel;
    char* waterTime;
} PLANTDATA;