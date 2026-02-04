#include <stdio.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "driver/uart.h"
#include "esp_log.h"

#include "lora.h"
#include "dht11.h"

static const char *TAG = "MAIN";

void app_main(void)
{
    // 初始化Lora
    lora_init();
    // 初始化DHT11
    dht11_init();
    
    // 定义DHT11数据结构体
    dht11_data_t dht11_data = {0};
    // 定义数据发送缓冲区
    uint8_t send_data[1024] = {0};
    // 定义乙烯浓度变量
    float ethene = 0.5;

    while (1)
    {
        // 读取DHT11数据
        dht11_err_t ret = dht11_read(&dht11_data);
        
        if (ret == DHT11_OK)
        {
            // 计算温度和湿度（小数部分通常为0）
            float temp = dht11_data.temp_int + dht11_data.temp_dec / 100.0;
            float humi = dht11_data.humidity_int + dht11_data.humidity_dec / 100.0;
            
            // 格式化发送数据
            int len = snprintf((char*)send_data, sizeof(send_data), 
                             "Temperature: %.2f C, Humidity: %.2f %%, Ethene: %.2f ppm\r\n", 
                             temp, humi, ethene);
            
            // 通过LoRa发送数据
            uart_write_bytes(UART_NUM_1, (const char*)send_data, len);
            
            // 打印日志
            ESP_LOGI(TAG, "Data sent: %s", send_data);
        }
        else
        {
            // 错误处理
            ESP_LOGW(TAG, "DHT11 read error: %d", ret);
        }
        
        // 清除发送缓冲区
        uart_flush(UART_NUM_1);
        
        // 延迟2秒再读取（DHT11最快2秒读取一次）
        vTaskDelay(pdMS_TO_TICKS(2000));
    }
}