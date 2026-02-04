#include "lora.h"
#include "driver/uart.h"
#include "driver/gpio.h"

void lora_init(void)
{
    // 波特率9600
    // 8位数据位
    // 不开启流控
    // 不开启校验
    // 硬件流控阈值，因未开启流控，此处随意设置一个值
    // 默认时钟线
    // 一位停止位
    uart_config_t uart_structure = {
        .baud_rate = 9600,
        .data_bits = UART_DATA_8_BITS,
        .flow_ctrl = UART_HW_FLOWCTRL_DISABLE,
        .parity = UART_PARITY_DISABLE,
        .rx_flow_ctrl_thresh = 100,
        .source_clk = UART_SCLK_DEFAULT,
        .stop_bits = UART_STOP_BITS_1,
    };

    uart_param_config(UART_NUM_1, &uart_structure);

    // uart1引脚为pin17&pin18，pin17为u1txd，pin18为u1rxd
    uart_set_pin(UART_NUM_1, GPIO_NUM_17, GPIO_NUM_18, -1, -1);   

    uart_driver_install(UART_NUM_1, 1024 , 1024 , 0, NULL, 0);
}