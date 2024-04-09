/*
 * LED blink with FreeRTOS
 */
#include <FreeRTOS.h>
#include <task.h>
#include <semphr.h>
#include <queue.h>

#include "pico/stdlib.h"
#include <stdio.h>
#include "hardware/adc.h"

#include <math.h>
#include <stdlib.h>

QueueHandle_t xQueueAdc;

typedef struct adc {
    int axis;
    int val;
} adc_t;

const int pinoX = 26;
const int pinoXADC = 0;
const int pinoY = 27;
const int pinoYADC = 1;

void write_package(adc_t data) {
    int val = data.val;
    int msb = val >> 8;
    int lsb = val & 0xFF ;

    uart_putc_raw(uart0, data.axis); 
    uart_putc_raw(uart0, lsb);
    uart_putc_raw(uart0, msb); 
    uart_putc_raw(uart0, -1); 
}

void x_task(void *p) {
    adc_init();
    adc_gpio_init(pinoX);    
    while (1) {
        adc_select_input(pinoXADC);
        int x = adc_read();

        struct adc x_base = {0,x};
        xQueueSend(xQueueAdc, &x_base, portMAX_DELAY);

        vTaskDelay(pdMS_TO_TICKS(100));
    }
}

void y_task(void *p) {
    adc_init();
    adc_gpio_init(pinoY);
    while (1) {
        adc_select_input(pinoYADC);
        int y = adc_read();

        struct adc y_base = {1,y};
        xQueueSend(xQueueAdc, &y_base, portMAX_DELAY);

        vTaskDelay(pdMS_TO_TICKS(100));
    }
}






void uart_task(void *p) {
    adc_t data;

    while (1) {
        xQueueReceive(xQueueAdc, &data, portMAX_DELAY);

        data.val = (data.val-2047)/8;
        int zone_limit = 80;
        if (data.val <=zone_limit && data.val >= -1*(zone_limit)) {
            data.val = 0;
        }
        // printf("Axis: %d, Value: %d\n", data.axis, data.val);
        write_package(data);
    }
}

int main() {
    stdio_init_all();
    xQueueAdc = xQueueCreate(32, sizeof(adc_t));

    xTaskCreate(x_task, "x_task", 256, NULL, 1, NULL);
    xTaskCreate(y_task, "y_task", 256, NULL, 1, NULL);
    xTaskCreate(uart_task, "uart_task", 4096, NULL, 1, NULL);
    vTaskStartScheduler();

    while (true);
}