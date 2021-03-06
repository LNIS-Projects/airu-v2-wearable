/*
 * led_if.c
 *
 *  Created on: Jan 18, 2019
 *      Author: tombo
 */

#include <stdio.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/event_groups.h"
#include "driver/ledc.h"
#include "esp_err.h"
#include "esp_log.h"
#include "../inc/led_if.h"

#define GP_LED    21	/* RED 	 */

#define STAT1_CH	LEDC_CHANNEL_0
#define STAT2_CH	LEDC_CHANNEL_1
#define STAT3_CH	LEDC_CHANNEL_2

#define LEDC_RESOLUTION		LEDC_TIMER_8_BIT
#define LEDC_NUM_LEDS     	(1)
#define LEDC_DUTY         	(0x1 << (LEDC_RESOLUTION - 2))		/* 1/4 Max Duty */

#define GPIO_EN_PAM2401    2
#define GPIO_OUTPUT_PIN_SEL  (1ULL<<GPIO_EN_PAM2401)

static const char* TAG = "LED";

static EventGroupHandle_t led_event_group;
static ledc_channel_config_t ledc_channel[LEDC_NUM_LEDS] = {
    {
        .channel    = STAT1_CH,
        .duty       = LEDC_DUTY,
        .gpio_num   = GP_LED,
        .speed_mode = LEDC_LOW_SPEED_MODE,
        .timer_sel  = LEDC_TIMER_0
    },
};


void LED_Initialize()
{
    int ch;

    ledc_timer_config_t ledc_timer = {
        .duty_resolution = LEDC_TIMER_8_BIT,
        .freq_hz = 500,
        .speed_mode = LEDC_LOW_SPEED_MODE,
        .timer_num = LEDC_TIMER_0
    };

    ledc_timer_config(&ledc_timer);

    // Set LED Controller with previously prepared configuration
    for (ch = 0; ch < LEDC_NUM_LEDS; ch++) {
        ledc_channel_config(&ledc_channel[ch]);
    }

	led_event_group = xEventGroupCreate();
	xEventGroupClearBits(led_event_group, LED_EVENT_ALL_BITS);
}


void LED_SetEventBit(led_events_t bit)
{
	xEventGroupSetBits(led_event_group, bit);
}


void led_task(void *pvParameters)
{
	int ch;
	esp_err_t err;
	EventBits_t uxBits;
	ESP_LOGI(TAG, "led_task Enterred");

	for(;;) {
		uxBits = xEventGroupWaitBits(led_event_group, LED_EVENT_ALL_BITS, pdTRUE, pdFALSE, portMAX_DELAY);

		if (uxBits & LED_EVENT_WIFI_DISCONNECTED_BIT) {
			ch = STAT1_CH;
			ledc_set_duty(ledc_channel[ch].speed_mode, ledc_channel[ch].channel, LEDC_DUTY);
			ledc_update_duty(ledc_channel[ch].speed_mode, ledc_channel[ch].channel);
		}
		if (uxBits & LED_EVENT_WIFI_CONNECTED_BIT) {
			ch = STAT1_CH;
			ledc_set_duty(ledc_channel[ch].speed_mode, ledc_channel[ch].channel, 0);
			ledc_update_duty(ledc_channel[ch].speed_mode, ledc_channel[ch].channel);
		}
	}
}

uint8_t airu_gpio_init()
{
	uint8_t ret = ESP_OK;
	gpio_config_t io_conf;
	//disable interrupt
	io_conf.intr_type = GPIO_PIN_INTR_DISABLE;
	//set as output mode
	io_conf.mode = GPIO_MODE_OUTPUT;
	//bit mask of the pins that you want to set,e.g.GPIO18/19
	io_conf.pin_bit_mask = GPIO_OUTPUT_PIN_SEL;
	//disable pull-down mode
	io_conf.pull_down_en = 0;
	//disable pull-up mode
	io_conf.pull_up_en = 0;
	//configure GPIO with the given settings
	gpio_config(&io_conf);

	// Set default GPIO level to 1
	gpio_set_level(GPIO_EN_PAM2401, 1);
	return ret;
}

