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

// 微秒延时函数
static void dht11_delay_us(uint32_t us)
{
    uint32_t start = esp_timer_get_time();
    while (esp_timer_get_time() - start < us);
}

// 等待引脚状态改变，超时返回-1
static int dht11_wait_pin(uint32_t pin, uint32_t level, uint32_t timeout_us)
{
    uint32_t start = esp_timer_get_time();
    while (gpio_get_level(pin) != level)
    {
        if (esp_timer_get_time() - start > timeout_us)
            return -1;
    }
    return 0;
}

int dht11_read_data(float* temperature, float* humidity)
{
    uint8_t data[5] = {0};
    uint32_t bit_count = 0;

    // 1. MCU发送启动信号：拉低至少18ms
    gpio_set_direction(GPIO_NUM_4, GPIO_MODE_OUTPUT);
    gpio_set_level(GPIO_NUM_4, 0);
    dht11_delay_us(20000);  // 拉低20ms

    // 2. MCU释放引脚，设为输入（通过上拉电阻拉高）
    gpio_set_direction(GPIO_NUM_4, GPIO_MODE_INPUT);

    // 3. 等待DHT11响应：DHT11拉低80us
    if (dht11_wait_pin(GPIO_NUM_4, 0, 100) < 0)
        return -1;  // 传感器无响应

    // 4. 等待DHT11拉高80us（响应完成）
    if (dht11_wait_pin(GPIO_NUM_4, 1, 100) < 0)
        return -1;

    // 5. 等待DHT11拉低（开始发送数据）
    if (dht11_wait_pin(GPIO_NUM_4, 0, 100) < 0)
        return -1;

    // 6. 读取40bit数据
    for (bit_count = 0; bit_count < 40; bit_count++)
    {
        // 等待引脚拉高（数据位开始）
        if (dht11_wait_pin(GPIO_NUM_4, 1, 100) < 0)
            return -1;

        uint32_t start = esp_timer_get_time();

        // 等待引脚拉低（数据位结束）
        if (dht11_wait_pin(GPIO_NUM_4, 0, 100) < 0)
            return -1;

        uint32_t duration = esp_timer_get_time() - start;

        // 若高电平时长 > 50us，则为bit 1，否则为bit 0
        uint8_t bit = (duration > 50) ? 1 : 0;

        // 将bit存入数据数组
        data[bit_count / 8] = (data[bit_count / 8] << 1) | bit;
    }

    // 7. 校验数据（前4字节和与校验码比较）
    uint8_t checksum = data[0] + data[1] + data[2] + data[3];
    if (checksum != data[4])
        return -2;  // 校验失败

    // 8. 提取温度和湿度
    // data[0]：湿度整数部分
    // data[1]：湿度小数部分
    // data[2]：温度整数部分
    // data[3]：温度小数部分
    *humidity = (float)data[0] + (float)data[1] / 256.0f;
    *temperature = (float)data[2] + (float)data[3] / 256.0f;

    return 0;  // 读取成功
}