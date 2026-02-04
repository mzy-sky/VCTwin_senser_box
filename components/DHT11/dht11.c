#include "dht11.h"
#include "driver/gpio.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"



void dht11_init(void)
{
    // 不使用中断
    // DHT11连接在GPIO4
    // 输入输出模式
    // 使能上拉
    // 禁用下拉
    gpio_config_t dht11_cfg = {
        .intr_type = GPIO_INTR_DISABLE,
        .pin_bit_mask = (1ULL << GPIO_NUM_4),
        .mode = GPIO_MODE_INPUT_OUTPUT,
        .pull_up_en = GPIO_PULLUP_ENABLE,
        .pull_down_en = GPIO_PULLDOWN_DISABLE,
    };
    gpio_config(&dht11_cfg);
}

// 读取DHT11数据
int dht11_read_data(float* temperature, float* humidity)
{
    uint8_t data[5] = {0};
    uint8_t byte_index = 0;
    uint8_t bit_index = 7;

    // 发送起始信号
    gpio_set_direction(GPIO_NUM_4, GPIO_MODE_OUTPUT);
    gpio_set_level(GPIO_NUM_4, 0);
    vTaskDelay(pdMS_TO_TICKS(20)); // 拉低至少18ms
    gpio_set_level(GPIO_NUM_4, 1);
    vTaskDelay(30); // 拉高20-40us

    // 切换到输入模式，等待DHT11响应
    gpio_set_direction(GPIO_NUM_4, GPIO_MODE_INPUT);

    // 等待DHT11拉低信号
    vTaskDelay(80); // 80ms
    if (gpio_get_level(GPIO_NUM_4) != 0) {
        return -1; // 响应失败
    }

    // 等待DHT11拉高信号
    vTaskDelay(80); // 80ms
    if (gpio_get_level(GPIO_NUM_4) != 1) {
        return -1; // 响应失败
    }

    // 读取40位数据
    for (int i = 0; i < 40; i++) {
        // 等待拉低信号
        while (gpio_get_level(GPIO_NUM_4) == 0);

        // 计时拉高信号持续时间
        vTaskDelay(40); // 40ms
        if (gpio_get_level(GPIO_NUM_4) == 1) {
            data[byte_index] |= (1 << bit_index); // 读到1
        }
        // 等待拉高信号结束
        while (gpio_get_level(GPIO_NUM_4) == 1);

        if (bit_index == 0) {
            bit_index = 7;
            byte_index++;
        } else {
            bit_index--;
        }
    }

    // 校验和验证
    if (data[4] != ((data[0] + data[1] + data[2] + data[3]) & 0xFF)) {
        return -2; // 校验失败
    }

    // 提取温度和湿度数据
    *humidity = data[0] + data[1] * 0.1f;
    *temperature = data[2] + data[3] * 0.1f;
    return 0; // 成功
}