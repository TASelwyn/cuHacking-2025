#ifndef SENSORS
#define SENSORS
int initialize_sensors();
sensorData *initialize_sensor_data();
int update_sensor_data(sensorData* data, int updateCount);
int post_sensor_data(apiDetails* api, sensorData* data);
#endif