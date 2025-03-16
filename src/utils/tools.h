#ifndef TOOLS
#define TOOLS
#include "rpi_gpio.h"

char* get_env_var(char* env_var_key);
apiDetails* get_api_details();
bool init_gpio(int gpio_pin, enum gpio_config_t gpio_inout);
bool read_gpio_in(int gpio_pin);
bool set_gpio_out(int gpio_pin, enum gpio_config_t gpio_LOW_OR_HIGH);
#endif