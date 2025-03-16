#ifndef TOOLS
#define TOOLS

char* get_env_var(char* env_var_key);
apiDetails* get_api_details();
sensorData *initialize_sensor_data();
bool init_led(int gpio_pin);
bool led_on(int gpio_pin);
bool led_off(int gpio_pin);
#endif