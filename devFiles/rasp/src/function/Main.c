#include "pico/stdlib.h"

int main() {
    // Initialisiere die Standard-Bibliothek (GPIO, UART, etc.)
    stdio_init_all();

    // Definiere die Pin-Nummer für die onboard LED (GP25)
    const uint LED_PIN = 25;
    gpio_init(LED_PIN);
    gpio_set_dir(LED_PIN, true);

    while (1) {
        gpio_put(LED_PIN, 1);   // LED an
        sleep_ms(500);
        gpio_put(LED_PIN, 0);   // LED aus
        sleep_ms(500);
    }

    return 0;
}
