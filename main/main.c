#include <stdio.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "driver/uart.h"



#include "lora.h"
#include "dht11.h"



void app_main(void)
{
    // 初始化Lora
    lora_init();
    // 初始化DHT11
    dht11_init();
    // 定义数据发送区
    int8_t sand_data[1024] = {0};
    // 定义温度变量
    float temp = 0.0;
    // 定义湿度变量
    float humi = 0.0;
    // 定义乙烯浓度变量
    float ethene = 0.0;

    while (1)
    {
        // 读取DHT11数据
        dht11_read_data(&temp, &humi);

        // 读取乙烯浓度数据，此处省略，假设读取到的值为0.5ppm
        ethene = 0.5;

        // 格式化发送数据
        int len = sprintf((char*)sand_data, "Temperature: %.2f C, Humidity: %.2f %%, Ethene: %.2f ppm\r\n", temp, humi, ethene);

        // 发送数据
        uart_write_bytes(UART_NUM_1, (const char*)sand_data, len);
        vTaskDelay(1000);  // 每秒发送一次

        // 清除发送区
        uart_flush(UART_NUM_1);
    };
    

}
