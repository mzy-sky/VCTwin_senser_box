#include <stdio.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "driver/gpio.h"
#include "sdkconfig.h"

#include "dht11.h"
#include "lcd.h"
#include "spi.h"



void app_main()
{
    DHT11_init(GPIO_NUM_4);
    lcd_init();

    // 创建一个任务周期性读取 DHT11 并在 LCD 上显示
    extern void vTaskDelay(const TickType_t xTicksToDelay);

    void dht_task(void *pvParameters){
        struct dht11_reading reading;
        char buf[32];
        for(;;) {
            reading = DHT11_read();
            if (reading.status == DHT11_OK) {
                sprintf(buf, "Temp:%dC", reading.temperature);
                lcd_show_string(1, 1, buf, WHITE, BLACK);
                sprintf(buf, "Hum :%d%%", reading.humidity);
                lcd_show_string(2, 1, buf, WHITE, BLACK);
            } else if (reading.status == DHT11_TIMEOUT_ERROR) {
                lcd_show_string(1, 1, "DHT11 TIMEOUT", WHITE, BLACK);
            } else if (reading.status == DHT11_CRC_ERROR) {
                lcd_show_string(1, 1, "DHT11 CRC ERR", WHITE, BLACK);
            } else {
                lcd_show_string(1, 1, "DHT11 ERR", WHITE, BLACK);
            }
            vTaskDelay(2000 / portTICK_PERIOD_MS);
        }
    }

    xTaskCreate(dht_task, "dht_task", 4096, NULL, 5, NULL);
}