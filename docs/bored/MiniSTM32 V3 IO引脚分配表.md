# MiniSTM32 V3 IO引脚分配表

来源文件：`MiniSTM32 V3 IO引脚分配表.xlsx`

## Sheet1

> 广州市星翼电子科技有限公司（ALIENTEK）
> MiniSTM32 V3 IO资源分配表

| 引脚编号 | GPIO | 连接资源 | 列4 | 完全<br>独立 | 连接关系说明 | 列7 |
| --- | --- | --- | --- | --- | --- | --- |
| 14 | PA0 | WK_UP | 18B20_DQ | Y | 1，按键KEY_UP<br>2，可以做待机唤醒脚(WKUP)<br>3，可以接DS18B20传感器接口(P2设置) | 只要KEY_UP不按下，该IO完全独立<br>可通过跳线帽连接1820和PA0，实现连接DS18B20传感器接口，从而连接数字温度传感器<br>注意：PA0和1820用跳线帽连接后，WK_UP按键将"失灵",所有按键相关例程都将失效 |
| 15 | PA1 | NRF_IRQ | REMOTE_IN | Y | 1，NRF24L01接口IRQ信号<br>2，接HS0038红外接收头(P2设置) | 该IO直接接NRF24L01模块接口的IRQ引脚，还可以通过跳线帽连接HS0038红外接收头(短接RMT和PA1，有4.7K上拉电阻)，当断开该跳线帽,且不接NRF24L01模块时，则可以完全独立 |
| 16 | PA2 | F_CS |  | N | W25Q64的片选信号 | 该IO接W25Q64的片选信号，不建议做普通IO使用 |
| 17 | PA3 | SD_CS |  | N | SD卡接口的片选脚 | 仅连接SD卡接口的片选脚，有47K上拉电阻，当不使用SD卡时，可做普通IO使用 |
| 20 | PA4 | NRF_CE |  | Y | NRF24L01接口的CE信号 | 接NRF24L01接口的CE脚，当不使用NRF24L01接口时，该IO完全独立 |
| 21 | PA5 | SPI1_SCK |  | N | W25Q64、SD卡和NRF24L01接口的SCK信号 | 当不使用W25Q64(片选禁止)、SD卡和NRF24L01接口时，该IO可做普通IO使用（有47K上拉） |
| 22 | PA6 | SPI1_MISO |  | N | W25Q64、SD卡和NRF24L01接口的MISO信号 | 当不使用W25Q64(片选禁止)、SD卡和NRF24L01接口时，该IO可做普通IO使用（有47K上拉） |
| 23 | PA7 | SPI1_MOSI |  | N | W25Q64、SD卡和NRF24L01接口的MOSI信号 | 当不使用W25Q64(片选禁止)、SD卡和NRF24L01接口时，该IO可做普通IO使用（有47K上拉） |
| 41 | PA8 | LED0 |  | N | 接DS0 LED灯(红色) | 该IO连接DS0，即红色LED灯。如做普通IO用，则DS0也受控制，建议：仅做输出用 |
| 42 | PA9 | U1_TXD |  | Y | 串口1 TX脚，默认连接CH340的RX(P4设置) | 该IO通过P4选择是否连接CH340的RXD，如果不连接，则该IO完全独立 |
| 43 | PA10 | U1_RXD |  | Y | 串口1 RX脚，默认连接CH340的TX(P4设置) | 该IO通过P4选择是否连接CH340的TXD，如果不连接，则该IO完全独立 |
| 44 | PA11 | USB_D- |  | Y | 接USB D-引脚 | 该IO直接接USB D-脚，如果USB接口不接线，则该IO完全独立 |
| 45 | PA12 | USB_D+ |  | Y | 接USB D+引脚 | 该IO直接接USB D+脚，如果USB接口不接线，则该IO完全独立 |
| 46 | PA13 | JTMS | SWDIO | N | JTAG/SWD仿真接口,没接任何外设<br>注意：如要做普通IO，需先禁止JTAG&SWD | JTAG/SWD仿真接口，没连外设。建议仿真器选择SWD调试，这样仅SWDIO和SWDCLK两个信号即可仿真。该IO做普通IO用(有10K上/下拉电阻)，需先禁止JTAG&SWD！此时无法仿真！<br>库函数全禁止方法：GPIO_PinRemapConfig(GPIO_Remap_SWJ_Disable)<br>寄存器全禁止方法：JTAG_Set(JTAG_SWD_DISABLE) |
| 49 | PA14 | JTCK | SWDCLK | N | JTAG/SWD仿真接口,没接任何外设<br>注意：如要做普通IO，需先禁止JTAG&SWD |  |
| 50 | PA15 | JTDI | PS_CLK<br>KEY1 | N | 1，JTAG仿真口(JTDI)<br>2，PS/2接口的CLK信号<br>3，接按键KEY1 | JTAG仿真口，也接PS/2接口的CLK信号和按键KEY1，如不用JTAG&PS/2接口&KEY1按键，则可做普通IO用(有10K上拉电阻)。做普通IO用，需先禁止JTAG。此时可SWD仿真，但JTAG无法仿真。<br>库函数禁止JTAG方法：GPIO_PinRemapConfig(GPIO_Remap_SWJ_JTAGDisable)<br>寄存器禁止JTAG方法:JTAG_Set(SWD_ENABLE) |
| 26 | PB0 | LCD_D0 |  | Y | TFTLCD接口的D0脚 | 该IO接TFTLCD模块接口的D0，当不插TFTLCD模块时，该IO完全独立 |
| 27 | PB1 | LCD_D1 |  | Y | TFTLCD接口的D1脚 | 该IO接TFTLCD模块接口的D1，当不插TFTLCD模块时，该IO完全独立 |
| 28 | PB2 | BOOT1 | LCD_D2 | N | 1，BOOT1,启动选择配置引脚(仅上电时用)<br>2，TFTLCD接口的D2脚 | 该IO在上电时，作BOOT1用(由B1控制上拉/下拉，设置启动模式)，同时作为TFTLCD模块接口的D2，当不插TFTLCD模块时，则可做普通IO用(有10K上拉/下拉,B0控制) |
| 55 | PB3 | JTDO | LCD_D3 | N | 1，JTAG仿真口(JTDO)<br>2，TFTLCD接口的D3脚(使用时，需先禁止JTAG，才可以当普通IO使用) | JTAG仿真口，同时作为TFTLCD模块接口的D3，如不用JTAG和TFTLCD模块接口，则可做普通IO用(有10K上拉电阻)。做普通IO用时，需先禁止JTAG。此时可用SWD仿真，但JTAG无法仿真。设置方法参考PA15的用法。 |
| 56 | PB4 | JTRST | LCD_D4 | N | 1，JTAG仿真口(JTRST)<br>2，TFTLCD接口的D4脚(使用时，需先禁止JTAG，才可以当普通IO使用) | JTAG仿真口，同时作为TFTLCD模块接口的D4，如不用JTAG和TFTLCD模块接口，则可做普通IO用(有10K上拉电阻)。做普通IO用时，需先禁止JTAG。此时可用SWD仿真，但JTAG无法仿真。设置方法参考PA15的用法。 |
| 57 | PB5 | LCD_D5 |  | Y | TFTLCD接口的D5脚 | 该IO接TFTLCD模块接口的D5，当不插TFTLCD模块时，该IO完全独立 |
| 58 | PB6 | LCD_D6 |  | Y | TFTLCD接口的D6脚 | 该IO接TFTLCD模块接口的D6，当不插TFTLCD模块时，该IO完全独立 |
| 59 | PB7 | LCD_D7 |  | Y | TFTLCD接口的D7脚 | 该IO接TFTLCD模块接口的D7，当不插TFTLCD模块时，该IO完全独立 |
| 61 | PB8 | LCD_D8 |  | Y | TFTLCD接口的D8脚 | 该IO接TFTLCD模块接口的D8，当不插TFTLCD模块时，该IO完全独立 |
| 62 | PB9 | LCD_D9 |  | Y | TFTLCD接口的D9脚 | 该IO接TFTLCD模块接口的D9，当不插TFTLCD模块时，该IO完全独立 |
| 29 | PB10 | LCD_D10 |  | Y | TFTLCD接口的D10脚 | 该IO接TFTLCD模块接口的D10，当不插TFTLCD模块时，该IO完全独立 |
| 30 | PB11 | LCD_D11 |  | Y | TFTLCD接口的D11脚 | 该IO接TFTLCD模块接口的D11，当不插TFTLCD模块时，该IO完全独立 |
| 33 | PB12 | LCD_D12 |  | Y | TFTLCD接口的D12脚 | 该IO接TFTLCD模块接口的D12，当不插TFTLCD模块时，该IO完全独立 |
| 34 | PB13 | LCD_D13 |  | Y | TFTLCD接口的D13脚 | 该IO接TFTLCD模块接口的D13，当不插TFTLCD模块时，该IO完全独立 |
| 35 | PB14 | LCD_D14 |  | Y | TFTLCD接口的D14脚 | 该IO接TFTLCD模块接口的D14，当不插TFTLCD模块时，该IO完全独立 |
| 36 | PB15 | LCD_D15 |  | Y | TFTLCD接口的D15脚 | 该IO接TFTLCD模块接口的D15，当不插TFTLCD模块时，该IO完全独立 |
| 8 | PC0 | T_SCK |  | Y | TFTLCD接口触摸屏SCK信号 | 该IO接TFTLCD模块接口的触摸屏SCK信号，当不插TFTLCD模块时，该IO完全独立 |
| 9 | PC1 | T_PEN |  | Y | TFTLCD接口触摸屏PEN信号 | 该IO接TFTLCD模块接口的触摸屏PEN信号(中断)，当不插TFTLCD模块时，该IO完全独立 |
| 10 | PC2 | T_MISO |  | Y | TFTLCD接口触摸屏MISO信号 | 该IO接TFTLCD模块接口的触摸屏MISO信号，当不插TFTLCD模块时，该IO完全独立 |
| 11 | PC3 | T_MOSI |  | Y | TFTLCD接口触摸屏MOSI信号 | 该IO接TFTLCD模块接口的触摸屏MOSI信号，当不插TFTLCD模块时，该IO完全独立 |
| 24 | PC4 | NRF_CS |  | Y | NRF24L01接口的CS信号 | 接NRF24L01接口的CS脚，当不使用NRF24L01接口时，该IO完全独立 |
| 25 | PC5 | PS_DAT | KEY0 | Y | 1，PS/2接口的DAT信号<br>2，接按键KEY0 | 该IO同时连接PS/2接口的DAT信号和按键KEY0，当不使用PS/2接口和按键KEY0的时候，该IO完全独立 |
| 37 | PC6 | LCD_RD |  | Y | TFTLCD接口的RD脚 | 该IO接TFTLCD模块接口的RD，当不插TFTLCD模块时，该IO完全独立 |
| 38 | PC7 | LCD_WR |  | Y | TFTLCD接口的WR脚 | 该IO接TFTLCD模块接口的WR，当不插TFTLCD模块时，该IO完全独立 |
| 39 | PC8 | LCD_RS |  | Y | TFTLCD接口的RS脚 | 该IO接TFTLCD模块接口的RS，当不插TFTLCD模块时，该IO完全独立 |
| 40 | PC9 | LCD_CS |  | Y | TFTLCD接口的CS脚 | 该IO接TFTLCD模块接口的CS，当不插TFTLCD模块时，该IO完全独立 |
| 51 | PC10 | LCD_BL |  | Y | TFTLCD接口的BL脚 | 该IO接TFTLCD模块接口的BL(背光控制脚)，当不插TFTLCD模块时，该IO完全独立 |
| 52 | PC11 | IIC_SDA |  | N | 接24C02的SDA | 该IO连接24C02的SDA信号，有4.7K上拉电阻，控制SCL=1，则该IO可做普通IO使用 |
| 53 | PC12 | IIC_SCL |  | N | 接24C02的SCL | 该IO连接24C02的SCL信号，有4.7K上拉电阻，不建议作为普通IO使用 |
| 2 | PC13 | T_CS |  | Y | TFTLCD接口触摸屏CS信号 | 该IO接TFTLCD模块接口的触摸屏CS信号，当不插TFTLCD模块时，该IO完全独立 |
| 3 | PC14 |  | RTC晶振 | N | 接32.768K晶振，不可用做IO | 外接RTC晶振用，不建议做普通IO用 |
| 4 | PC15 |  | RTC晶振 | N | 接32.768K晶振，不可用做IO | 外接RTC晶振用，不建议做普通IO用 |
| 5 | PD0 |  | HSE晶振 | N | 接HSE晶振，不可用做IO | 外接HSE晶振用，不建议做普通IO用 |
| 6 | PD1 |  | HSE晶振 | N | 接HSE晶振，不可用做IO | 外接HSE晶振用，不建议做普通IO用 |
| 54 | PD2 | LED1 |  | N | 接DS1 LED灯(绿色) | 该IO连接DS1，即绿色LED灯。如做普通IO用，则DS1也受控制，建议：仅做输出用 |
| 引脚编号：对应STM32F103RCT6的引脚编号<br>      GPIO：STM32F103RCT6的IO口<br>   完全独立：指该IO通过一定的方法，可以达到完全悬空的效果（即不接任何其他外设，且不接任何上拉/下拉电阻）<br>连接关系说明：说明每个IO口与外设的连接关系<br>   使用提示：介绍每个IO的特点和使用方法，方便大家掌握开发板每一个IO口的使用 |  |  |  |  |  |  |

## Sheet2

空白工作表。

## Sheet3

空白工作表。

