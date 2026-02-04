#include "dht11.h"
#include "esp_timer.h"
#include "rom/ets_sys.h"
#include <string.h>

// 微秒级延时封装（ESP-IDF推荐使用ets_delay_us，精准度满足单总线需求）
#define DHT11_DELAY_US(us)  ets_delay_us(us)

/**
 * @brief  释放总线，设置引脚为输入模式
 */
static void dht11_bus_release(void)
{
    gpio_set_direction(DHT11_GPIO_PIN, GPIO_MODE_INPUT);
}

/**
 * @brief  拉低总线，设置引脚为输出低电平
 */
static void dht11_bus_pull_down(void)
{
    gpio_set_direction(DHT11_GPIO_PIN, GPIO_MODE_OUTPUT_OD);
    gpio_set_level(DHT11_GPIO_PIN, 0);
}

/**
 * @brief  等待总线电平变化，带超时检测
 * @param  level 等待的目标电平
 * @param  timeout_us 超时时间（微秒）
 * @return 成功返回0，超时返回-1
 */
static int8_t dht11_wait_level(uint8_t level, uint32_t timeout_us)
{
    uint32_t start = esp_timer_get_time();
    while (gpio_get_level(DHT11_GPIO_PIN) != level)
    {
        if (esp_timer_get_time() - start > timeout_us)
        {
            return -1; // 超时
        }
        DHT11_DELAY_US(1);
    }
    return 0;
}

void dht11_init(void)
{
    // 复位GPIO配置
    gpio_reset_pin(DHT11_GPIO_PIN);
    // 初始化为输入模式，释放总线
    dht11_bus_release();
    // 初始状态拉高总线
    gpio_set_level(DHT11_GPIO_PIN, 1);
}

dht11_err_t dht11_read(dht11_data_t *data)
{
    // 入参校验
    if (data == NULL)
    {
        return DHT11_ERR_TIMEOUT;
    }
    uint8_t buf[5] = {0}; // 存储40bit原始数据（5字节）

    // 1. 主机发送起始信号：拉低18~20ms
    dht11_bus_pull_down();
    DHT11_DELAY_US(18000);

    // 2. 主机拉高总线20~40us，等待传感器响应
    dht11_bus_release();
    DHT11_DELAY_US(30);

    // 3. 检测传感器响应信号（低电平80us）
    if (dht11_wait_level(0, 100) != 0)
    {
        return DHT11_ERR_NO_RESPONSE;
    }

    // 检测响应高电平80us
    if (dht11_wait_level(1, 100) != 0)
    {
        return DHT11_ERR_NO_RESPONSE;
    }

    // 4. 读取40bit数据
    for (uint8_t i = 0; i < 40; i++)
    {
        // 等待数据位起始低电平
        if (dht11_wait_level(0, 50) != 0)
        {
            return DHT11_ERR_TIMEOUT;
        }

        // 读取高电平持续时间，判断是0还是1
        uint32_t start = esp_timer_get_time();
        dht11_wait_level(1, 50);
        uint32_t duration = esp_timer_get_time() - start;

        // 左移缓存，写入数据位：>28us为1，≤28us为0
        buf[i / 8] <<= 1;
        if (duration > 28)
        {
            buf[i / 8] |= 1;
        }
    }

    // 5. 校验和验证：前4字节之和等于第5字节为有效数据
    if ((buf[0] + buf[1] + buf[2] + buf[3]) != buf[4])
    {
        return DHT11_ERR_CRC;
    }

    // 6. 解析数据到结构体
    data->humidity_int = buf[0];
    data->humidity_dec = buf[1];
    data->temp_int = buf[2];
    data->temp_dec = buf[3];

    return DHT11_OK;
}