#include "gpio_config.h"

void gpio_setup(void) {
    // Konfigurieren des Trigger-Pins als Ausgang
    gpio_config_t io_conf_trigger;
    io_conf_trigger.intr_type = GPIO_INTR_DISABLE;  // Keine Interrupts
    io_conf_trigger.mode = GPIO_MODE_OUTPUT;        // Output-Modus
    io_conf_trigger.pin_bit_mask = (1ULL << GPIO_OUTPUT_IO_TRIGGER);  // Bit-Maske für den Pin
    io_conf_trigger.pull_down_en = 0;               // Pull-down nicht aktivieren
    io_conf_trigger.pull_up_en = 0;                 // Pull-up nicht aktivieren
    gpio_config(&io_conf_trigger);

    // Konfigurieren des Echo-Pins als Eingang mit Interrupt bei jeder Flanke
    gpio_config_t io_conf_echo;
    io_conf_echo.intr_type = GPIO_INTR_ANYEDGE;     // Interrupt bei jeder Flankenänderung
    io_conf_echo.mode = GPIO_MODE_INPUT;            // Input-Modus
    io_conf_echo.pin_bit_mask = (1ULL << GPIO_OUTPUT_IO_ECHO);        // Bit-Maske für den Pin
    io_conf_echo.pull_down_en = 1;                  // Pull-down aktivieren
    io_conf_echo.pull_up_en = 0;                    // Pull-up nicht aktivieren
    gpio_config(&io_conf_echo);
}