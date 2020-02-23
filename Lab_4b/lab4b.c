#include <stdio.h>
#include <signal.h>
#include <mraa/gpio.h>
#include <mraa/aio.h>

sig_atomic_t volatile run_flag = 1;
int debug_flag = 0; /* flag debug option */

void intHandler() {
    run_flag = 0;
}

int main() {
    printf("this is the test version\n");

    uint16_t analog_in;

    /* register a gpio context, for pin 60, name button */
    mraa_gpio_context button;
    button = mraa_gpio_init(60);

    /* register a AIO context, for pin 1, name t_sensor */
    mraa_aio_context t_sensor;
    t_sensor = mraa_aio_init(1);
    
    /* configure buzzer to output pin */
    mraa_gpio_dir(button, MRAA_GPIO_IN);

    /* register a button handler */
    signal(SIGINT, intHandler);
    mraa_gpio_isr(button, MRAA_GPIO_EDGE_RISING, &intHandler, NULL);

    while (run_flag) {
        /* buzzer on */
        analog_in = mraa_aio_read(t_sensor);
        printf("%d\n", analog_in);
    }

    /* close context */
    mraa_gpio_close(button);
    mraa_aio_close(t_sensor);

    return 0;
}