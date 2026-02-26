[English](/README.md) | [ 简体中文](/README_zh-Hans.md) | [繁體中文](/README_zh-Hant.md) | [日本語](/README_ja.md) | [Deutsch](/README_de.md) | [한국어](/README_ko.md)

<div align=center>
<img src="/doc/image/logo.svg" width="400" height="150"/>
</div>

## LibDriver SSD1681

[![MISRA](https://img.shields.io/badge/misra-compliant-brightgreen.svg)](/misra/README.md) [![API](https://img.shields.io/badge/api-reference-blue.svg)](https://www.libdriver.com/docs/ssd1681/index.html) [![License](https://img.shields.io/badge/license-MIT-brightgreen.svg)](/LICENSE)

SSD1681 is an Active Matrix EPD display driver with controller for Red/Black/White EPD displays.It consists of 200 source outputs, 200 gate outputs, 1 VCOM and 1VBD (for border), which can support displays with resolution up to 200x200.In the SSD1681, data and commands are sent from MCU through hardware selectable serial peripheral interface.It has embedded booster, regulator and oscillator which is suitable for EPD display applications.

LibDriver SSD1681 is a full-featured driver for SSD1681, launched by LibDriver.It provides single point drawing, block filling, image display and additional features. LibDriver is MISRA compliant.

### Table of Contents

  - [Instruction](#Instruction)
  - [Install](#Install)
  - [Usage](#Usage)
    - [example basic](#example-basic)
  - [Document](#Document)
  - [Contributing](#Contributing)
  - [License](#License)
  - [Contact Us](#Contact-Us)

### Instruction

/src includes LibDriver SSD1681 source files.

/interface includes LibDriver SSD1681 SPI platform independent template.

/test includes LibDriver SSD1681 driver test code and this code can test the chip necessary function simply.

/example includes LibDriver SSD1681 sample code.

/doc includes LibDriver SSD1681 offline document.

/datasheet includes SSD1681 datasheet.

/project includes the common Linux and MCU development board sample code. All projects use the shell script to debug the driver and the detail instruction can be found in each project's README.md.

/misra includes the LibDriver MISRA code scanning results.

### Install

Reference /interface SPI platform independent template and finish your platform SPI driver.

Add the /src directory, the interface driver for your platform, and your own drivers to your project, if you want to use the default example drivers, add the /example directory to your project.

### Usage

You can refer to the examples in the /example directory to complete your own driver. If you want to use the default programming examples, here's how to use them.

#### example basic

The image extraction format is as follows and every pixel stored in one byte.

<div align=center>
<img src="/doc/image/image_format.svg" width="220" height="220"/>
</div>

```C
#include "driver_ssd1681_basic.h"

uint8_t data;
char str[] = "test";
uint8_t g_image[40000];

/* basic init */
if (ssd1681_basic_init() != 0)
{
    (void)ssd1681_basic_deinit();
    
    return 1;
}

/* clear the red color */
if (ssd1681_basic_clear(SSD1681_COLOR_RED) != 0)
{
    (void)ssd1681_basic_deinit();
    
    return 1;
}

/* clear the black color */
if (ssd1681_basic_clear(SSD1681_COLOR_BLACK) != 0)
{
    (void)ssd1681_basic_deinit();
    
    return 1;
}

...
    
/* basic string */
if (ssd1681_basic_string(SSD1681_COLOR_RED, 0, 0, str, (uint16_t)strlen(str), 1, SSD1681_FONT_16) != 0)
{
    (void)ssd1681_basic_deinit();
    
    return 1;
}

...
    
/* basic point read */
if (ssd1681_basic_read_point(SSD1681_COLOR_RED, 16, 16, &data) != 0)
{
    (void)ssd1681_basic_deinit();
    
    return 1;
}

...
    
/* basic write point */
if (ssd1681_basic_write_point(SSD1681_COLOR_BLACK, 20, 20, 1) != 0)
{
    (void)ssd1681_basic_deinit();
    
    return 1;
}

...
    
/* basic write rect */
if (ssd1681_basic_rect(SSD1681_COLOR_BLACK, 0, 0, 32, 32), 1) != 0)
{
    (void)ssd1681_basic_deinit();
    
    return 1;
}

...
    
/* basic draw the picture */
if (ssd1681_basic_picture(SSD1681_COLOR_BLACK, 0, 0, 200, 200, g_image) != 0)
{
    (void)ssd1681_basic_deinit();
    
    return 1;
}

/* basic deinit */
if (ssd1681_basic_deinit() != 0)
{
    return 1;
}

return 0;
```

### Document

Online documents: [https://www.libdriver.com/docs/ssd1681/index.html](https://www.libdriver.com/docs/ssd1681/index.html).

Offline documents: /doc/html/index.html.

### Contributing

Please refer to CONTRIBUTING.md.

### License

Copyright (c) 2015 - present LibDriver All rights reserved



The MIT License (MIT) 



Permission is hereby granted, free of charge, to any person obtaining a copy

of this software and associated documentation files (the "Software"), to deal

in the Software without restriction, including without limitation the rights

to use, copy, modify, merge, publish, distribute, sublicense, and/or sell

copies of the Software, and to permit persons to whom the Software is

furnished to do so, subject to the following conditions: 



The above copyright notice and this permission notice shall be included in all

copies or substantial portions of the Software. 



THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR

IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,

FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE

AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER

LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,

OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE

SOFTWARE. 

### Contact Us

Please send an e-mail to lishifenging@outlook.com.