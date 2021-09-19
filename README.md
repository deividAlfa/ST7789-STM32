# ST7789-STM32
Using STM32's Hardware SPI to drive a ST7789 based IPS display.<br>
Forked from [Floyd-Fish](https://github.com/Floyd-Fish/ST7789-STM32)<br>

This fork is a lot faster, specially on filling (6x faster in sw mode), but the best is the DMA (13x faster).<br>
The projects works right away, use STM32 CUBE IDE.<br>
Tested on STM32F103 and STM32F411, using a 135x240 4-wire SPI screen.<br>

Configuration is done in st7780.h<br>

For more information, check the [original project](https://github.com/Floyd-Fish/ST7789-STM32)  
