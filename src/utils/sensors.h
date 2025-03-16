#ifndef SENSORS
#define SENSORS
int gather_data();
int update_sensor_data(sensorData* data);
sensorData *initialize_sensor_data();
int post_sensor_data(apiDetails* api, sensorData* data);
#endif