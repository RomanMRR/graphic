/**
 * @ingroup     drivers_ra8835
 *
 * @{
 * @file
 * @brief       Driver for the RA8835 graphic LCD
 *
 *
 * @author      Alexander Podshivalov <a_podshivalov@mail.ru>
 *
 * @}
 */

#include <assert.h>
#include <string.h>

#include "log.h"
#include "periph/gpio.h"
#include "xtimer.h"
#include <lptimer.h>

#define ENABLE_DEBUG (0)
#include "debug.h"

#include "ra8835.h"
#include "ra8835_internal.h"
#include <stdlib.h> 

/* Need this for upside-down graphic displays */
static const char reverse[256] = {
  0x00, 0x80, 0x40, 0xC0, 0x20, 0xA0, 0x60, 0xE0, 0x10, 0x90, 0x50, 0xD0, 0x30, 0xB0, 0x70, 0xF0, 
  0x08, 0x88, 0x48, 0xC8, 0x28, 0xA8, 0x68, 0xE8, 0x18, 0x98, 0x58, 0xD8, 0x38, 0xB8, 0x78, 0xF8, 
  0x04, 0x84, 0x44, 0xC4, 0x24, 0xA4, 0x64, 0xE4, 0x14, 0x94, 0x54, 0xD4, 0x34, 0xB4, 0x74, 0xF4, 
  0x0C, 0x8C, 0x4C, 0xCC, 0x2C, 0xAC, 0x6C, 0xEC, 0x1C, 0x9C, 0x5C, 0xDC, 0x3C, 0xBC, 0x7C, 0xFC, 
  0x02, 0x82, 0x42, 0xC2, 0x22, 0xA2, 0x62, 0xE2, 0x12, 0x92, 0x52, 0xD2, 0x32, 0xB2, 0x72, 0xF2, 
  0x0A, 0x8A, 0x4A, 0xCA, 0x2A, 0xAA, 0x6A, 0xEA, 0x1A, 0x9A, 0x5A, 0xDA, 0x3A, 0xBA, 0x7A, 0xFA,
  0x06, 0x86, 0x46, 0xC6, 0x26, 0xA6, 0x66, 0xE6, 0x16, 0x96, 0x56, 0xD6, 0x36, 0xB6, 0x76, 0xF6, 
  0x0E, 0x8E, 0x4E, 0xCE, 0x2E, 0xAE, 0x6E, 0xEE, 0x1E, 0x9E, 0x5E, 0xDE, 0x3E, 0xBE, 0x7E, 0xFE,
  0x01, 0x81, 0x41, 0xC1, 0x21, 0xA1, 0x61, 0xE1, 0x11, 0x91, 0x51, 0xD1, 0x31, 0xB1, 0x71, 0xF1,
  0x09, 0x89, 0x49, 0xC9, 0x29, 0xA9, 0x69, 0xE9, 0x19, 0x99, 0x59, 0xD9, 0x39, 0xB9, 0x79, 0xF9, 
  0x05, 0x85, 0x45, 0xC5, 0x25, 0xA5, 0x65, 0xE5, 0x15, 0x95, 0x55, 0xD5, 0x35, 0xB5, 0x75, 0xF5,
  0x0D, 0x8D, 0x4D, 0xCD, 0x2D, 0xAD, 0x6D, 0xED, 0x1D, 0x9D, 0x5D, 0xDD, 0x3D, 0xBD, 0x7D, 0xFD,
  0x03, 0x83, 0x43, 0xC3, 0x23, 0xA3, 0x63, 0xE3, 0x13, 0x93, 0x53, 0xD3, 0x33, 0xB3, 0x73, 0xF3, 
  0x0B, 0x8B, 0x4B, 0xCB, 0x2B, 0xAB, 0x6B, 0xEB, 0x1B, 0x9B, 0x5B, 0xDB, 0x3B, 0xBB, 0x7B, 0xFB,
  0x07, 0x87, 0x47, 0xC7, 0x27, 0xA7, 0x67, 0xE7, 0x17, 0x97, 0x57, 0xD7, 0x37, 0xB7, 0x77, 0xF7, 
  0x0F, 0x8F, 0x4F, 0xCF, 0x2F, 0xAF, 0x6F, 0xEF, 0x1F, 0x9F, 0x5F, 0xDF, 0x3F, 0xBF, 0x7F, 0xFF
};

static void _send(const ra8835_t *dev, uint8_t value, ra8835_state_t state);

static void _send(const ra8835_t *dev, uint8_t value, ra8835_state_t state){
    gpio_set(dev->rd);
    gpio_set(dev->wr);
    if( state == RA8835_DATA ){
        gpio_clear(dev->a0);
    } else {
        gpio_set(dev->a0);
    }
    gpio_clear(dev->cs);
    gpio_clear(dev->wr);
    
    /* like in HD44870 driver
       not a very brilliant idea to bit-
       band a large graphic lcd, so
       TODO: use something better */
    xtimer_usleep(1);
    for (unsigned i = 0; i < 8; ++i) {
        if ((value >> i) & 0x01) {
            gpio_set(dev->data[i]);
        }
        else {
            gpio_clear(dev->data[i]);
        }
    }
    xtimer_usleep(1);
    
    gpio_set(dev->wr);
    gpio_set(dev->cs);
}

int ra8835_init(ra8835_t *dev){
    uint16_t addr;
    
    gpio_init(dev->wr, GPIO_OUT); // ~WR
    gpio_init(dev->rd, GPIO_OUT); // ~RD
    gpio_init(dev->cs, GPIO_OUT); // ~CS
    gpio_init(dev->a0, GPIO_OUT); // A0
    gpio_init(dev->rst, GPIO_OUT);// ~RST
    
    for( int i = 0; i < 8; i++){
        gpio_init(dev->data[i], GPIO_OUT); // D[i]
    }
    
    /* These lines are default high */
    gpio_set(dev->wr);
    gpio_set(dev->rd);
    gpio_set(dev->cs);
    gpio_set(dev->rst);
    
    /* Reset pulse */
    xtimer_usleep(RA8835_RESET_PULSE);
    gpio_clear(dev->rst);
    xtimer_usleep(RA8835_RESET_PULSE);
    gpio_set(dev->rst);
    
    _send(dev, RA8835_SYSTEM_SET, RA8835_CMD);//System Set
    _send(dev, 0x31, RA8835_DATA);//P1: IV =1;M0=1, "External" CGRAM (or last ROM pages);M1=0,No D6 correction; W/S=0,Single-Panel; M2=0,8-Pixel character
    _send(dev, 0x87, RA8835_DATA);//P2: WF=1,two-frame AC Driver;FX=8,Set Horizontal Character Size 8
    _send(dev, 8-1, RA8835_DATA);//P3: Set Vertical Character Size
    _send(dev, dev->cols/8 - 1, RA8835_DATA);//P4: CR,Bytes per display line
    _send(dev, 0x2F, RA8835_DATA);//P5: T/CR,Line Length
    _send(dev, dev->rows - 1, RA8835_DATA);//P6: L/F,Lines per frame
    _send(dev, 0x28, RA8835_DATA);//P7: APL
    _send(dev, 0x00, RA8835_DATA);//P8: APH,define the horizontal address range of the virtual address
    
    /* Memory allocation setup */
    /* First layer (text), 8*8 characters, no scroll */
    /* Starts at 0000 */
    /* Second layer (graphics) */
    /* Allocated after first layer */
    addr = (dev->rows / 8) * (dev->cols / 8);
    _send(dev, RA8835_SCROLL, RA8835_CMD);
    _send(dev, 0x00, RA8835_DATA);//P1: SAD 1L
    _send(dev, 0x00, RA8835_DATA);//P2: SAD 1H
    _send(dev, dev->rows, RA8835_DATA);//P3: SL1
    _send(dev, addr & 0xFF, RA8835_DATA); ;//P4: SAD 2L
    _send(dev, (addr >> 8) & 0xFF, RA8835_DATA); //P5: SAD 2H
    _send(dev, dev->rows, RA8835_DATA);//P6: SL2
    _send(dev, 0x00, RA8835_DATA);//P7: SAD 3L
    _send(dev, 0x00, RA8835_DATA);//P8: SAD 3H
    _send(dev, 0x00, RA8835_DATA);//P9: SAD 4L
    _send(dev, 0x00, RA8835_DATA);//P10: SAD 4H
    
    /* Set Cursor Size and Shape */
    _send(dev, RA8835_CSRFORM, RA8835_CMD);
    _send(dev, 0x04, RA8835_DATA);//P1: Set Horizontal Size
    _send(dev, 0x86, RA8835_DATA);//p2: Set Vertical Size; CM = 1 for gfx mode
    
    _send(dev, RA8835_HDOT_SCR, RA8835_CMD);//HDOT SCR
    _send(dev, 0x00, RA8835_DATA);
    
    /* Selects layered screen composition and screen text/graphics mode */
    _send(dev, RA8835_OVLAY, RA8835_CMD);
    /* MX[1:0] = 00, OR mode, DM[1:2] = 00, text mode, OV = 0, two-layer mixed text and graphics */
    _send(dev, 0x00, RA8835_DATA);
    
    /* Load a custom font with Cyrillic characters */
    /* Also suitable for upside-down displays */
    /* Set cursor adress to start of CG "ROM" */
    /* See comments about A15 line in MELT displays, also tested with Winstar */
    _send(dev, RA8835_CSRW, RA8835_CMD);
    _send(dev, 0x00, RA8835_DATA);
    _send(dev, 0x70, RA8835_DATA);
    /* Set cursor autoincrement to move it properly */
    _send(dev, RA8835_CSRDIR_RIGHT, RA8835_CMD);
    /* Write character glyphs to LCD RAM */
    _send(dev, RA8835_MWRITE, RA8835_CMD);
    for(unsigned c = 0; c < 256; c++){
        for(unsigned l = 0; l < 8; l++){
            char line;
            if( !dev->upside_down ){
                line = ra8835_font[c*8 + l];
            } else {
                line = reverse[ra8835_font[c*8 + 7 - l]];
            }
            _send(dev, line, RA8835_DATA);
        }
    }
    
    /* Also need to set CG RAM? */
    _send(dev, RA8835_CGRAM_ADR, RA8835_CMD);
    _send(dev, 0x00, RA8835_DATA);
    _send(dev, 0x70, RA8835_DATA);
    
    ra8835_clear(dev);
    ra8835_text_clear(dev);
    
    /* Display on */
    _send(dev, RA8835_DISPLAY_ON, RA8835_CMD);
    /* SAD3 blank, SAD2+SAD4 no flashing, SAD1 no flashing, cursor blank */
    _send(dev, 0x14, RA8835_DATA);
    
    return 0;
}

void ra8835_text_clear(const ra8835_t *dev){
    ra8835_text_home(dev);
    
    /* Write blanks to LCD RAM */
    _send(dev, RA8835_MWRITE, RA8835_CMD);
    for(int y = 0; y < dev->rows/8; y++){
        for(int x = 0; x < dev->cols/8; x++){
            _send(dev, ' ', RA8835_DATA);
        }
    }
}

void ra8835_text_home(const ra8835_t *dev){
    ra8835_text_set_cursor(dev, 0, 0);
}

void ra8835_text_set_cursor(const ra8835_t *dev, uint8_t col, uint8_t row){
    uint16_t addr = row * (dev->cols / 8) + col;
    
    if( dev->upside_down ){
        addr = (dev->cols / 8) * (dev->rows / 8) - addr -1;
    }
    
    /* Set cursor adress to upper left corner */
    _send(dev, RA8835_CSRW, RA8835_CMD);
    _send(dev, addr & 0xFF, RA8835_DATA);
    _send(dev, (addr >> 8) & 0xFF, RA8835_DATA);
    
    /* Set cursor autoincrement to move it properly */
    if( !dev->upside_down ){
        _send(dev, RA8835_CSRDIR_RIGHT, RA8835_CMD);
    } else {
        _send(dev, RA8835_CSRDIR_LEFT, RA8835_CMD);
    }
}

void ra8835_text_write(const ra8835_t *dev, uint8_t value){
    /* Write text data to LCD RAM */
    _send(dev, RA8835_MWRITE, RA8835_CMD);
    _send(dev, value, RA8835_DATA);
}

void ra8835_text_print(const ra8835_t *dev, const char *data){
    /* Write text data to LCD RAM */
    _send(dev, RA8835_MWRITE, RA8835_CMD);
    while(*data != 0){
        _send(dev, *data++, RA8835_DATA);
    }
    
}

void ra8835_clear(const ra8835_t *dev){
    uint16_t addr = (dev->rows / 8) * (dev->cols / 8);

    /* Set cursor adress to upper left corner */
    _send(dev, RA8835_CSRW, RA8835_CMD);
    _send(dev, addr & 0xFF, RA8835_DATA);
    _send(dev, (addr >> 8) & 0xFF, RA8835_DATA);
    
    /* Set cursor autoincrement to move it right */
    _send(dev, RA8835_CSRDIR_RIGHT, RA8835_CMD);
    
    /* Write zeros to LCD RAM */
    _send(dev, RA8835_MWRITE, RA8835_CMD);
    for(int y = 0; y < dev->rows; y++){
        for(int x = 0; x < dev->cols/8; x++){
            _send(dev, 0x00, RA8835_DATA);
        }
    }
}

void ra8835_write_img(const ra8835_t *dev, const char img[]){
    uint16_t addr;

    /* Set cursor adress to upper left corner */
    addr = (dev->rows / 8) * (dev->cols / 8);
    if( dev->upside_down ){
        /* Some displays are upside down, so... */
        /* Set cursor adress to down right corner */
        addr += dev->rows * (dev->cols / 8) - 1;
    }
    _send(dev, RA8835_CSRW, RA8835_CMD); 
    _send(dev, addr & 0xFF, RA8835_DATA);
    _send(dev, (addr >> 8) & 0xFF, RA8835_DATA);
    
    /* Set cursor autoincrement to move it properly */
    if( !dev->upside_down ){
        _send(dev, RA8835_CSRDIR_RIGHT, RA8835_CMD);
    } else {
        _send(dev, RA8835_CSRDIR_LEFT, RA8835_CMD);
    }

    /* Write picture data to LCD RAM */
    _send(dev, RA8835_MWRITE, RA8835_CMD);
    for(int y = 0; y < dev->rows; y++){
        for(int x = 0; x < dev->cols/8; x++){
            char p = img[y * (dev->cols/8) + x];
            if( dev->upside_down ){
                /* Reverse bits */
                p = reverse[(size_t) p];
            }
            _send(dev, p, RA8835_DATA);
        }
    }
}


void ra8835_put_pixel(const ra8835_t *dev, int x, int y) {
    uint16_t addr;

    /* Set cursor adress to upper left corner */
    addr = (dev->rows / 8) * (dev->cols / 8);

    /* Set address of the point */
    addr += y * (dev->cols/8) + (x/8);
 
    _send(dev, RA8835_CSRW, RA8835_CMD);
    _send(dev, addr & 0xFF, RA8835_DATA); //младший байт
    _send(dev, (addr >> 8) & 0xFF, RA8835_DATA); //старший байт

    /* Write picture data to LCD RAM */
    _send(dev, RA8835_MWRITE, RA8835_CMD);
    
    _send(dev, (0x01 << (7 - x%8)), RA8835_DATA);

}

void ra8835_line (const ra8835_t *dev, int x1, int y1, int x2, int y2) {
    //Координаты точек

    int dx = (x2 - x1 >= 0 ? 1 : -1);
    int dy = (y2 - y1 >= 0 ? 1 : -1);
 
    int lengthX = abs(x2 - x1);
    int lengthY = abs(y2 - y1);
 
    int length = (lengthX - lengthY >=0 ? lengthX : lengthY);
 
     if (length == 0)
     {
        ra8835_put_pixel(dev, x1, y1);
        return;
     }
 
     if (lengthY <= lengthX)
     {
            // Начальные значения
         int x = x1;
         int y = y1;
         int d = -lengthX;
 
            // Основной цикл
         length++;
         while(length >0)
            {
                  // ra8835_put_pixel(&the_display, x1, y1);
                uint16_t addr;

                /* Set cursor adress to upper left corner */
                addr = (dev->rows / 8) * (dev->cols / 8);

                /* Set address of the point */
                addr += y * (dev->cols/8) + (x/8);
                //Передаём адрес
                _send(dev, RA8835_CSRW, RA8835_CMD); 
                _send(dev, addr & 0xFF, RA8835_DATA);
                _send(dev, (addr >> 8) & 0xFF, RA8835_DATA);

                _send(dev, RA8835_MWRITE, RA8835_CMD);


                int numberByte = x/8; 
                int y_old = y; //Чтобы определять, не поменялась ли строка
                int maskInit = 0x01; //Для формирования маски
                int mask = 0; //Формируемая маска

                while (y == y_old && (length-- >0)) //пока находимся на одной и той же строке
                {   
                    mask = mask + (maskInit << (7 - x%8)); //Формируем маску

                    x += dx;

                    d += 2 * lengthY;
                    if (d > 0) {
                        d -= 2 * lengthX;
                        y += dy;
                        _send(dev, mask, RA8835_DATA);
                        break;
                  }
                  if (x/8 != numberByte) { //Если начался новый байт, то передаём сформированную маску
                    _send(dev, mask, RA8835_DATA);
                    numberByte = x/8;
                    mask = 0;
                    }
                }

            }
      }
      else
      {
            // Начальные значения
            int x = x1;
            int y = y1;
            int d = - lengthY;
 
            // Основной цикл
            length++;
            while(length-- > 0)
            {   
                ra8835_put_pixel(dev, x, y);
                y += dy;
                d += 2* lengthX;
                if (d > 0) {
                        d -= 2 * lengthY;
                        x += dx;
                  }

            }
      }
}