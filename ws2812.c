#include <stdio.h>
#include <stdlib.h>
#include "pico/stdlib.h"
#include "hardware/pio.h"
#include "hardware/clocks.h"
#include "ws2812.pio.h"

#define LED_R 13      // LED vermelho do RGB
#define BTN_A 5       // Botão A da BitDogLab
#define BTN_B 6       // Botão B da BitDogLab
#define RGBW_MODE false // LEDs apenas RGB, sem canal branco
#define MATRIX_SIZE 25 // Número total de LEDs na matriz 5x5
#define WS2812_PORT 7  // Pino de controle dos LEDs
#define TIME_DELAY 100 // Intervalo de 100 ms

static volatile uint32_t last_press_time = 0; 
static void handle_button_press(uint gpio, uint32_t events);

bool digits[10][MATRIX_SIZE] = {
    { 0,1,1,1,0, 0,1,0,1,0, 0,1,0,1,0, 0,1,0,1,0, 0,1,1,1,0 },
    { 0,1,0,0,0, 0,0,0,1,0, 0,1,0,0,0, 0,0,0,1,0, 0,1,0,0,0 },
    { 0,1,1,1,0, 0,1,0,0,0, 0,1,1,1,0, 0,0,0,1,0, 0,1,1,1,0 },
    { 0,1,1,1,0, 0,0,0,1,0, 0,1,1,1,0, 0,0,0,1,0, 0,1,1,1,0 },
    { 0,1,0,0,0, 0,0,0,1,0, 0,1,1,1,0, 0,1,0,1,0, 0,1,0,1,0 },
    { 0,1,1,1,0, 0,0,0,1,0, 0,1,1,1,0, 0,1,0,0,0, 0,1,1,1,0 },
    { 0,1,1,1,0, 0,1,0,1,0, 0,1,1,1,0, 0,1,0,0,0, 0,1,1,1,0 },
    { 0,1,0,0,0, 0,0,0,1,0, 0,1,0,0,0, 0,0,0,1,0, 0,1,1,1,0 },
    { 0,1,1,1,0, 0,1,0,1,0, 0,1,1,1,0, 0,1,0,1,0, 0,1,1,1,0 },
    { 0,1,1,1,0, 0,0,0,1,0, 0,1,1,1,0, 0,1,0,1,0, 0,1,1,1,0 }
};

uint32_t led_state[MATRIX_SIZE] = {0};
uint8_t current_digit = 0;

static inline void set_pixel(uint32_t color) {
    pio_sm_put_blocking(pio0, 0, color << 8u);
}

static inline uint32_t rgb_color(uint8_t r, uint8_t g, uint8_t b) {
    return ((uint32_t)(r) << 8) | ((uint32_t)(g) << 16) | (uint32_t)(b);
}

void refresh_led_state() {
    for (int i = 0; i < MATRIX_SIZE; i++) {
        led_state[i] = digits[current_digit][i] ? rgb_color(90, 0, 70) : 0;
    }
}

void apply_led_state() {
    for (int i = 0; i < MATRIX_SIZE; i++) {
        set_pixel(led_state[i]);
    }
}

void handle_button_press(uint gpio, uint32_t events) {
    uint32_t now = to_us_since_boot(get_absolute_time());

    if (now - last_press_time > 200000) {
        last_press_time = now;

        if (gpio == BTN_A) {
            current_digit = (current_digit + 1) % 10;
        } else if (gpio == BTN_B) {
            current_digit = (current_digit + 9) % 10;
        }
    }
}

int main() {
    gpio_init(LED_R);
    gpio_set_dir(LED_R, GPIO_OUT);
    gpio_put(LED_R, 0);

    gpio_init(BTN_A);
    gpio_set_dir(BTN_A, GPIO_IN);
    gpio_pull_up(BTN_A);

    gpio_init(BTN_B);
    gpio_set_dir(BTN_B, GPIO_IN);
    gpio_pull_up(BTN_B);

    PIO pio = pio0;
    int sm = 0;
    uint offset = pio_add_program(pio, &ws2812_program);
    ws2812_program_init(pio, sm, offset, WS2812_PORT, 800000, RGBW_MODE);

    gpio_set_irq_enabled_with_callback(BTN_A, GPIO_IRQ_EDGE_FALL, true, &handle_button_press);
    gpio_set_irq_enabled_with_callback(BTN_B, GPIO_IRQ_EDGE_FALL, true, &handle_button_press);

    absolute_time_t next_toggle = delayed_by_us(get_absolute_time(), TIME_DELAY * 1000);
    bool led_status = false;

    while (1) {
        if (time_reached(next_toggle)) {
            led_status = !led_status;
            gpio_put(LED_R, led_status);
            next_toggle = delayed_by_us(next_toggle, TIME_DELAY * 1000);
        }
        
        refresh_led_state();
        apply_led_state();
        sleep_ms(100);
    }

    return 0;
}
