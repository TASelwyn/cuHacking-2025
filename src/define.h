#define SENSOR_API_HOSTNAME "PLANTR_API_HOSTNAME"
#define SENSOR_API_API_KEY  "PLANTR_API_KEY"
#define SENSOR_API_SERIAL   "PLANTR_SERIAL_NUM"

#define SENSOR_INTERVAL_SECS 15

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