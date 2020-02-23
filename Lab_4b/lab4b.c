#include <stdio.h>
#include <signal.h>
#include <mraa/gpio.h>

sig_atomic_t volatile run_flag = 1;
int debug_flag = 0; /* flag debug option */

void intHandler() {
    run_flag = 0;
}

int main() {
    printf("this is the test version\n");

    /* register a gpio context, for port 62, name buzzer 
       and */
    mraa_gpio_context buzzer, button;
    buzzer = mraa_gpio_init(62);
    button = mraa_gpio_init(60);
    
    /* configure buzzer to output pin */
    mraa_gpio_dir(buzzer, MRAA_GPIO_OUT);
    mraa_gpio_dir(button, MRAA_GPIO_IN);

    /* register a SIGINT handler */
    signal(SIGINT, intHandler);
    mraa_gpio_isr(button, MRAA_GPIO_EDGE_RISING, &intHandler, NULL);

    while (run_flag) {
        /* buzzer on */
        printf("buzzzz\n");
        sleep(1);
    }

    /* turn off buzzer and close context */
    mraa_gpio_write(buzzer, 0);
    mraa_gpio_close(buzzer);
    mraa_gpio_close(button);

    return 0;
}