#include "driver/can.h"
#include "driver/gpio.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

#define LED 2

bool led_status = 0;

void toggle_LED(void *pvParameter) {
    while (1) {
        led_status = !led_status;
        gpio_set_level(LED, led_status);
        vTaskDelay(100 / portTICK_PERIOD_MS);
    }
}

void CAN_Send(void *pvParameter) {

    while (1) {
        // Configure message to transmit
        can_message_t message;
        message.identifier = 0xAAAA;
        message.flags = CAN_MSG_FLAG_EXTD;
        message.data_length_code = 4;
        for (int i = 0; i < 4; i++) {
            message.data[i] = i;
        }

        // Queue message for transmission
        if (can_transmit(&message, pdMS_TO_TICKS(1000)) == ESP_OK) {
            printf("Message queued for transmission\n");
        } else {
            printf("Failed to queue message for transmission\n");
        }
    }
}

void CAN_Receive(void *pvParameter) {
    while (1) {

        // Wait for message to be received
        can_message_t message;
        if (can_receive(&message, pdMS_TO_TICKS(10000)) == ESP_OK) {
            printf("Message received\n");
        } else {
            printf("Failed to receive message\n");
            return;
        }

        // Process received message
        if (message.flags & CAN_MSG_FLAG_EXTD) {
            printf("Message is in Extended Format\n");
        } else {
            printf("Message is in Standard Format\n");
        }
        printf("ID is %d\n", message.identifier);
        if (!(message.flags & CAN_MSG_FLAG_RTR)) {
            for (int i = 0; i < message.data_length_code; i++) {
                printf("Data byte %d = %d\n", i, message.data[i]);
            }
        }
    }
}

void app_main() {
    // configure LED
    gpio_set_direction(LED, GPIO_MODE_OUTPUT);

    // Initialize configuration structures using macro initializers
    can_general_config_t g_config =
        CAN_GENERAL_CONFIG_DEFAULT(GPIO_NUM_21, GPIO_NUM_19, CAN_MODE_NORMAL);
    can_timing_config_t t_config = CAN_TIMING_CONFIG_500KBITS();
    can_filter_config_t f_config = CAN_FILTER_CONFIG_ACCEPT_ALL();

    // Install CAN driver
    if (can_driver_install(&g_config, &t_config, &f_config) == ESP_OK) {
        printf("Driver installed\n");
    } else {
        printf("Failed to install driver\n");
        return;
    }

    // Start CAN driver
    if (can_start() == ESP_OK) {
        printf("Driver started\n");
    } else {
        printf("Failed to start driver\n");
        return;
    }

    // xTaskCreate(&CAN_Send, "can_send", 2048, NULL, 5, NULL);
    xTaskCreate(&CAN_Receive, "can_receive", 2048, NULL, 5, NULL);
    xTaskCreate(&toggle_LED, "led", 512, NULL, tskIDLE_PRIORITY, NULL);
}