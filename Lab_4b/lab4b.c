#include <stdio.h>
#include <signal.h>
#include <mraa/gpio.h>

sig_atomic_t volatile run_flag = 1;
int debug_flag = 0; /* flag debug option */

void intHandler(int sig) {
    if (sig == SIGINT) {
        run_flag = 0;
    }
}

int main() {
    printf("this is the test version\n");

    /* register a gpio context, for port 62, name buzzer */
    mraa_gpio_context buzzer;
    buzzer = mraa_gpio_init(62);
    
    /* configure buzzer to output pin */
    mraa_gpio_dir(buzzer, MRAA_GPIO_OUT);

    /* register a SIGINT handler */
    signal(SIGINT, intHandler);

    while (run_flag) {
        /* buzzer on */
        mraa_gpio_write(buzzer, 1);
        sleep(1);

        /* buzzer off */
        mraa_gpio_write(buzzer, 0);
        sleep(1);
    }

    /* turn off buzzer and close context */
    mraa_gpio_write(buzzer, 0);
    mraa_gpio_close(buzzer);

    return 0;
}