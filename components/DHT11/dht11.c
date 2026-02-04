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

// 读取DHT11数据，使用 esp_timer_get_time() 精确到微秒
int dht11_read_data(float* temperature, float* humidity)
{
    if (!temperature || !humidity) return ESP_ERR_INVALID_ARG;

    const gpio_num_t pin = GPIO_NUM_4;
    uint8_t data[5] = {0};
    uint8_t byte_index = 0;
    uint8_t bit_index = 7;
    const int64_t bit_high_threshold_us = 50; // 高电平大于此为1
    const int64_t resp_timeout_us = 1000 * 1000; // 1s 超时保护

    // 发送起始信号：拉低至少18ms -> 拉高20-40us -> 切换为输入
    gpio_set_direction(pin, GPIO_MODE_OUTPUT);
    gpio_set_level(pin, 0);
    // 延迟>=18ms（使用esp_timer精确延迟）
    int64_t t0 = esp_timer_get_time();
    while (esp_timer_get_time() - t0 < 18000) { ; }
    gpio_set_level(pin, 1);
    // 等待约30-40us
    t0 = esp_timer_get_time();
    while (esp_timer_get_time() - t0 < 40) { ; }

    gpio_set_direction(pin, GPIO_MODE_INPUT);

    int64_t start = esp_timer_get_time();
    // 等待设备拉低（应答开始：约80us低）
    while (gpio_get_level(pin) == 1) {
        if (esp_timer_get_time() - start > resp_timeout_us) return ESP_ERR_TIMEOUT;
    }
    // 等待设备拉高（应答高约80us）
    start = esp_timer_get_time();
    while (gpio_get_level(pin) == 0) {
        if (esp_timer_get_time() - start > resp_timeout_us) return ESP_ERR_TIMEOUT;
    }
    // 等待设备再次拉低，之后开始发送数据
    start = esp_timer_get_time();
    while (gpio_get_level(pin) == 1) {
        if (esp_timer_get_time() - start > resp_timeout_us) return ESP_ERR_TIMEOUT;
    }

    // 读取40位数据
    for (int i = 0; i < 40; i++) {
        // 等待低电平开始（每位前约50us低电平）
        start = esp_timer_get_time();
        while (gpio_get_level(pin) == 1) {
            if (esp_timer_get_time() - start > resp_timeout_us) return ESP_ERR_TIMEOUT;
        }

        // 等待高电平开始
        start = esp_timer_get_time();
        while (gpio_get_level(pin) == 0) {
            if (esp_timer_get_time() - start > resp_timeout_us) return ESP_ERR_TIMEOUT;
        }

        // 高电平开始，计时其持续时间
        int64_t high_start = esp_timer_get_time();
        while (gpio_get_level(pin) == 1) {
            if (esp_timer_get_time() - high_start > resp_timeout_us) return ESP_ERR_TIMEOUT;
        }
        int64_t high_end = esp_timer_get_time();
        int64_t high_duration = high_end - high_start; // 单位us

        if (high_duration > bit_high_threshold_us) {
            data[byte_index] |= (1 << bit_index);
        }

        if (bit_index == 0) {
            bit_index = 7;
            byte_index++;
        } else {
            bit_index--;
        }
    }

    // 校验和验证
    if (data[4] != ((data[0] + data[1] + data[2] + data[3]) & 0xFF)) {
        return ESP_ERR_INVALID_CRC;
    }

    // 提取温湿度（DHT11 高字节整数，低字节小数，通常低字节为0）
    *humidity = data[0] + data[1] * 0.1f;
    *temperature = data[2] + data[3] * 0.1f;
    return ESP_OK;
}