#ifndef DHT11_H
#define DHT11_H

#include "driver/gpio.h"
#include "esp_err.h"

// 配置DHT11数据引脚，可根据硬件修改
#define DHT11_GPIO_PIN    GPIO_NUM_4

// DHT11数据结构体
typedef struct {
    uint8_t humidity_int;    // 湿度整数部分
    uint8_t humidity_dec;    // 湿度小数部分（DHT11固定为0）
    uint8_t temp_int;        // 温度整数部分
    uint8_t temp_dec;        // 温度小数部分（DHT11固定为0）
} dht11_data_t;

// 函数返回状态枚举
typedef enum {
    DHT11_OK = 0,             // 读取成功
    DHT11_ERR_TIMEOUT,        // 通信超时
    DHT11_ERR_CRC,            // 校验和错误
    DHT11_ERR_NO_RESPONSE     // 传感器无响应
} dht11_err_t;

/**
 * @brief  初始化DHT11的GPIO引脚
 * @return 无
 */
void dht11_init(void);

/**
 * @brief  读取DHT11温湿度数据
 * @param  data 指向存储解析后数据的结构体指针
 * @return dht11_err_t 状态码
 */
dht11_err_t dht11_read(dht11_data_t *data);

#endif // DHT11_H