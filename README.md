# Driver for ST7789 displays using STM32 and uGUI library.
Using STM32's Hardware SPI to drive a ST7789 based IPS display.<br>
Forked from [Floyd-Fish](https://github.com/Floyd-Fish/ST7789-STM32)<br>
Added [uGUI](https://github.com/achimdoebler/UGUI) library, made some modifications for better performance.<br>
This fork is a lot faster, specially on filling with the DMA (13x faster).<br>
Everything has been optimized, font drawing is also a lot better due pixel packing, counts the same-color consecutive pixels,
then draw them in a single operation, thus removing a lot of overhead.<br>
The projects works right away, use STM32 CUBE IDE (Import... Exiting project... Select any of the demos).<br>
Tested on STM32F103 and STM32F411, using a 135x240 4-wire SPI screen.<br>
It can achieve 60FPS with 32MHZ SPI clock.<br>

Configuration is done in st7780.h<br>

For more information, check the [Original ST7789 project](https://github.com/Floyd-Fish/ST7789-STM32) and [uGUI](https://github.com/achimdoebler/UGUI) page.<br>
To convert fonts, use [ttf2uGUI](https://github.com/AriZuu/ttf2ugui). I've compiled it for windows (ttf2ugui-win.zip).<br>
Bitmaps can be converted with[Lcd image converter](https://sourceforge.net/projects/lcd-image-converter/), use 16 bit packing, Little endian.<br>
