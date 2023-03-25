#include <sys/types.h>
#include <sys/ioctl.h>
#include <sys/socket.h>
#include <sys/sysctl.h>
#include <dev/iicbus/iic.h>
#include <stdio.h>
#include <string.h>
#include <err.h>
#include <errno.h>
#include <fcntl.h>
#include <unistd.h>
#include <stdlib.h>
#include <ifaddrs.h>
#include <netinet/in.h>
#include <net/if.h>
#include <netdb.h>
#include <time.h>
#include <locale.h>

#define IICBUS	"/dev/iic0"
#define IICADDR	0x3c

#define SSD1306_LCDWIDTH  128
#define SSD1306_LCDHEIGHT 32
#define DISPLAY_BUFF_SIZE (SSD1306_LCDWIDTH*SSD1306_LCDHEIGHT/8 + 1)

#define SSD1306_SETCONTRAST 0x81
#define SSD1306_DISPLAYALLON_RESUME 0xA4
#define SSD1306_DISPLAYALLON 0xA5
#define SSD1306_NORMALDISPLAY 0xA6
#define SSD1306_INVERTDISPLAY 0xA7
#define SSD1306_DISPLAYOFF 0xAE
#define SSD1306_DISPLAYON 0xAF

#define SSD1306_SETDISPLAYOFFSET 0xD3
#define SSD1306_SETCOMPINS 0xDA

#define SSD1306_SETVCOMDETECT 0xDB

#define SSD1306_SETDISPLAYCLOCKDIV 0xD5
#define SSD1306_SETPRECHARGE 0xD9

#define SSD1306_SETMULTIPLEX 0xA8

#define SSD1306_SETLOWCOLUMN 0x00
#define SSD1306_SETHIGHCOLUMN 0x10

#define SSD1306_SETSTARTLINE 0x40

#define SSD1306_MEMORYMODE 0x20
#define SSD1306_COLUMNADDR 0x21
#define SSD1306_PAGEADDR   0x22

#define SSD1306_COMSCANINC 0xC0
#define SSD1306_COMSCANDEC 0xC8

#define SSD1306_SEGREMAP 0xA0

#define SSD1306_CHARGEPUMP 0x8D

#define SSD1306_EXTERNALVCC 0x1
#define SSD1306_SWITCHCAPVCC 0x2

// Scrolling #defines
#define SSD1306_ACTIVATE_SCROLL 0x2F
#define SSD1306_DEACTIVATE_SCROLL 0x2E
#define SSD1306_SET_VERTICAL_SCROLL_AREA 0xA3
#define SSD1306_RIGHT_HORIZONTAL_SCROLL 0x26
#define SSD1306_LEFT_HORIZONTAL_SCROLL 0x27
#define SSD1306_VERTICAL_AND_RIGHT_HORIZONTAL_SCROLL 0x29
#define SSD1306_VERTICAL_AND_LEFT_HORIZONTAL_SCROLL 0x2A

// Standard ASCII 5x7 font - Found NO possibility to get ° degree symbol to work
static const unsigned char ssd1306_font5x7[] = {
                                                0x00, 0x00, 0x00, 0x00, 0x00, //space
                                                0x3E, 0x5B, 0x4F, 0x5B, 0x3E,
                                                0x3E, 0x6B, 0x4F, 0x6B, 0x3E,
                                                0x1C, 0x3E, 0x7C, 0x3E, 0x1C,
                                                0x18, 0x3C, 0x7E, 0x3C, 0x18,
                                                0x1C, 0x57, 0x7D, 0x57, 0x1C,
                                                0x1C, 0x5E, 0x7F, 0x5E, 0x1C,
                                                0x00, 0x18, 0x3C, 0x18, 0x00,
                                                0xFF, 0xE7, 0xC3, 0xE7, 0xFF,
                                                0x00, 0x18, 0x24, 0x18, 0x00,
                                                0xFF, 0xE7, 0xDB, 0xE7, 0xFF,
                                                0x30, 0x48, 0x3A, 0x06, 0x0E,
                                                0x26, 0x29, 0x79, 0x29, 0x26,
                                                0x40, 0x7F, 0x05, 0x05, 0x07,
                                                0x40, 0x7F, 0x05, 0x25, 0x3F,
                                                0x5A, 0x3C, 0xE7, 0x3C, 0x5A,
                                                0x7F, 0x3E, 0x1C, 0x1C, 0x08,
                                                0x08, 0x1C, 0x1C, 0x3E, 0x7F,
                                                0x14, 0x22, 0x7F, 0x22, 0x14,
                                                0x5F, 0x5F, 0x00, 0x5F, 0x5F,
                                                0x06, 0x09, 0x7F, 0x01, 0x7F,
                                                0x00, 0x66, 0x89, 0x95, 0x6A,
                                                0x60, 0x60, 0x60, 0x60, 0x60,
                                                0x94, 0xA2, 0xFF, 0xA2, 0x94,
                                                0x08, 0x04, 0x7E, 0x04, 0x08,//up INDEX 24
                                                0x10, 0x20, 0x7E, 0x20, 0x10,//down INDEX 25
                                                0x08, 0x08, 0x2A, 0x1C, 0x08,
                                                0x08, 0x1C, 0x2A, 0x08, 0x08,
                                                0x1E, 0x10, 0x10, 0x10, 0x10,
                                                0x0C, 0x1E, 0x0C, 0x1E, 0x0C,
                                                0x30, 0x38, 0x3E, 0x38, 0x30,
                                                0x06, 0x0E, 0x3E, 0x0E, 0x06,
                                                0x00, 0x00, 0x00, 0x00, 0x00,
                                                0x00, 0x00, 0x5F, 0x00, 0x00,
                                                0x00, 0x07, 0x00, 0x07, 0x00,
                                                0x14, 0x7F, 0x14, 0x7F, 0x14,
                                                0x24, 0x2A, 0x7F, 0x2A, 0x12,
                                                0x23, 0x13, 0x08, 0x64, 0x62,
                                                0x36, 0x49, 0x56, 0x20, 0x50,
                                                0x00, 0x08, 0x07, 0x03, 0x00,
                                                0x00, 0x1C, 0x22, 0x41, 0x00,
                                                0x00, 0x41, 0x22, 0x1C, 0x00,
                                                0x2A, 0x1C, 0x7F, 0x1C, 0x2A,
                                                0x08, 0x08, 0x3E, 0x08, 0x08,
                                                0x00, 0x80, 0x70, 0x30, 0x00,
                                                0x08, 0x08, 0x08, 0x08, 0x08,
                                                0x00, 0x00, 0x60, 0x60, 0x00,
                                                0x20, 0x10, 0x08, 0x04, 0x02,
                                                0x3E, 0x51, 0x49, 0x45, 0x3E,
                                                0x00, 0x42, 0x7F, 0x40, 0x00,
                                                0x72, 0x49, 0x49, 0x49, 0x46,
                                                0x21, 0x41, 0x49, 0x4D, 0x33,
                                                0x18, 0x14, 0x12, 0x7F, 0x10,
                                                0x27, 0x45, 0x45, 0x45, 0x39,
                                                0x3C, 0x4A, 0x49, 0x49, 0x31,
                                                0x41, 0x21, 0x11, 0x09, 0x07,
                                                0x36, 0x49, 0x49, 0x49, 0x36,
                                                0x46, 0x49, 0x49, 0x29, 0x1E,
                                                0x00, 0x00, 0x14, 0x00, 0x00,
                                                0x00, 0x40, 0x34, 0x00, 0x00,
                                                0x00, 0x08, 0x14, 0x22, 0x41,
                                                0x14, 0x14, 0x14, 0x14, 0x14,
                                                0x00, 0x41, 0x22, 0x14, 0x08,
                                                0x02, 0x01, 0x59, 0x09, 0x06,
                                                0x3E, 0x41, 0x5D, 0x59, 0x4E,
                                                0x7C, 0x12, 0x11, 0x12, 0x7C,
                                                0x7F, 0x49, 0x49, 0x49, 0x36,
                                                0x3E, 0x41, 0x41, 0x41, 0x22,//C
                                                0x7F, 0x41, 0x41, 0x41, 0x3E,//D
                                                0x7F, 0x49, 0x49, 0x49, 0x41,//E
                                                0x7F, 0x09, 0x09, 0x09, 0x01,//F
                                                0x3E, 0x41, 0x41, 0x51, 0x73,
                                                0x7F, 0x08, 0x08, 0x08, 0x7F,
                                                0x00, 0x41, 0x7F, 0x41, 0x00,
                                                0x20, 0x40, 0x41, 0x3F, 0x01,
                                                0x7F, 0x08, 0x14, 0x22, 0x41,
                                                0x7F, 0x40, 0x40, 0x40, 0x40,
                                                0x7F, 0x02, 0x1C, 0x02, 0x7F,
                                                0x7F, 0x04, 0x08, 0x10, 0x7F,
                                                0x3E, 0x41, 0x41, 0x41, 0x3E,
                                                0x7F, 0x09, 0x09, 0x09, 0x06,
                                                0x3E, 0x41, 0x51, 0x21, 0x5E,
                                                0x7F, 0x09, 0x19, 0x29, 0x46,
                                                0x26, 0x49, 0x49, 0x49, 0x32,
                                                0x03, 0x01, 0x7F, 0x01, 0x03,
                                                0x3F, 0x40, 0x40, 0x40, 0x3F,
                                                0x1F, 0x20, 0x40, 0x20, 0x1F,
                                                0x3F, 0x40, 0x38, 0x40, 0x3F,
                                                0x63, 0x14, 0x08, 0x14, 0x63,
                                                0x03, 0x04, 0x78, 0x04, 0x03,
                                                0x61, 0x59, 0x49, 0x4D, 0x43,
                                                0x00, 0x7F, 0x41, 0x41, 0x41,
                                                0x02, 0x04, 0x08, 0x10, 0x20,
                                                0x00, 0x41, 0x41, 0x41, 0x7F,
                                                0x04, 0x02, 0x01, 0x02, 0x04,
                                                0x40, 0x40, 0x40, 0x40, 0x40,
                                                0x00, 0x03, 0x07, 0x08, 0x00,
                                                0x20, 0x54, 0x54, 0x78, 0x40,
                                                0x7F, 0x28, 0x44, 0x44, 0x38,
                                                0x38, 0x44, 0x44, 0x44, 0x28,
                                                0x38, 0x44, 0x44, 0x28, 0x7F,
                                                0x38, 0x54, 0x54, 0x54, 0x18,
                                                0x00, 0x08, 0x7E, 0x09, 0x02,
                                                0x18, 0xA4, 0xA4, 0x9C, 0x78,
                                                0x7F, 0x08, 0x04, 0x04, 0x78,
                                                0x00, 0x44, 0x7D, 0x40, 0x00,
                                                0x20, 0x40, 0x40, 0x3D, 0x00,
                                                0x7F, 0x10, 0x28, 0x44, 0x00,
                                                0x00, 0x41, 0x7F, 0x40, 0x00,
                                                0x7C, 0x04, 0x78, 0x04, 0x78,
                                                0x7C, 0x08, 0x04, 0x04, 0x78,
                                                0x38, 0x44, 0x44, 0x44, 0x38,
                                                0xFC, 0x18, 0x24, 0x24, 0x18,
                                                0x18, 0x24, 0x24, 0x18, 0xFC,
                                                0x7C, 0x08, 0x04, 0x04, 0x08,
                                                0x48, 0x54, 0x54, 0x54, 0x24,
                                                0x04, 0x04, 0x3F, 0x44, 0x24,
                                                0x3C, 0x40, 0x40, 0x20, 0x7C,
                                                0x1C, 0x20, 0x40, 0x20, 0x1C,
                                                0x3C, 0x40, 0x30, 0x40, 0x3C,
                                                0x44, 0x28, 0x10, 0x28, 0x44,
                                                0x4C, 0x90, 0x90, 0x90, 0x7C,
                                                0x44, 0x64, 0x54, 0x4C, 0x44,
                                                0x00, 0x08, 0x36, 0x41, 0x00,
                                                0x00, 0x00, 0x77, 0x00, 0x00,
                                                0x00, 0x41, 0x36, 0x08, 0x00,
                                                0x02, 0x01, 0x02, 0x04, 0x02,
                                                0x3C, 0x26, 0x23, 0x26, 0x3C,
                                                0x1E, 0xA1, 0xA1, 0x61, 0x12,
                                                0x3A, 0x40, 0x40, 0x20, 0x7A,
                                                0x38, 0x54, 0x54, 0x55, 0x59,
                                                0x21, 0x55, 0x55, 0x79, 0x41,
                                                0x22, 0x54, 0x54, 0x78, 0x42, // a-umlaut
                                                0x21, 0x55, 0x54, 0x78, 0x40,
                                                0x20, 0x54, 0x55, 0x79, 0x40,
                                                0x0C, 0x1E, 0x52, 0x72, 0x12,
                                                0x39, 0x55, 0x55, 0x55, 0x59,
                                                0x39, 0x54, 0x54, 0x54, 0x59,
                                                0x39, 0x55, 0x54, 0x54, 0x58,
                                                0x00, 0x00, 0x45, 0x7C, 0x41,
                                                0x00, 0x02, 0x45, 0x7D, 0x42,
                                                0x00, 0x01, 0x45, 0x7C, 0x40,
                                                0x7D, 0x12, 0x11, 0x12, 0x7D, // A-umlaut
                                                0xF0, 0x28, 0x25, 0x28, 0xF0,
                                                0x7C, 0x54, 0x55, 0x45, 0x00,
                                                0x20, 0x54, 0x54, 0x7C, 0x54,
                                                0x7C, 0x0A, 0x09, 0x7F, 0x49,
                                                0x32, 0x49, 0x49, 0x49, 0x32,
                                                0x3A, 0x44, 0x44, 0x44, 0x3A, // o-umlaut
                                                0x32, 0x4A, 0x48, 0x48, 0x30,
                                                0x3A, 0x41, 0x41, 0x21, 0x7A,
                                                0x3A, 0x42, 0x40, 0x20, 0x78,
                                                0x00, 0x9D, 0xA0, 0xA0, 0x7D,
                                                0x3D, 0x42, 0x42, 0x42, 0x3D, // O-umlaut
                                                0x3D, 0x40, 0x40, 0x40, 0x3D,
                                                0x3C, 0x24, 0xFF, 0x24, 0x24,
                                                0x48, 0x7E, 0x49, 0x43, 0x66,
                                                0x2B, 0x2F, 0xFC, 0x2F, 0x2B,
                                                0xFF, 0x09, 0x29, 0xF6, 0x20,
                                                0xC0, 0x88, 0x7E, 0x09, 0x03,
                                                0x20, 0x54, 0x54, 0x79, 0x41,
                                                0x00, 0x00, 0x44, 0x7D, 0x41,
                                                0x30, 0x48, 0x48, 0x4A, 0x32,
                                                0x38, 0x40, 0x40, 0x22, 0x7A,
                                                0x00, 0x7A, 0x0A, 0x0A, 0x72,
                                                0x7D, 0x0D, 0x19, 0x31, 0x7D,
                                                0x26, 0x29, 0x29, 0x2F, 0x28,
                                                0x26, 0x29, 0x29, 0x29, 0x26,
                                                0x30, 0x48, 0x4D, 0x40, 0x20,
                                                0x38, 0x08, 0x08, 0x08, 0x08,
                                                0x08, 0x08, 0x08, 0x08, 0x38,
                                                0x2F, 0x10, 0xC8, 0xAC, 0xBA,
                                                0x2F, 0x10, 0x28, 0x34, 0xFA,
                                                0x00, 0x00, 0x7B, 0x00, 0x00,
                                                0x08, 0x14, 0x2A, 0x14, 0x22,
                                                0x22, 0x14, 0x2A, 0x14, 0x08,
                                                0x55, 0x00, 0x55, 0x00, 0x55, // #176 (25% block) missing in old code
                                                0xAA, 0x55, 0xAA, 0x55, 0xAA, // 50% block
                                                0xFF, 0x55, 0xFF, 0x55, 0xFF, // 75% block
                                                0x00, 0x00, 0x00, 0xFF, 0x00,
                                                0x10, 0x10, 0x10, 0xFF, 0x00,
                                                0x14, 0x14, 0x14, 0xFF, 0x00,
                                                0x10, 0x10, 0xFF, 0x00, 0xFF,
                                                0x10, 0x10, 0xF0, 0x10, 0xF0,
                                                0x14, 0x14, 0x14, 0xFC, 0x00,
                                                0x14, 0x14, 0xF7, 0x00, 0xFF,
                                                0x00, 0x00, 0xFF, 0x00, 0xFF,
                                                0x14, 0x14, 0xF4, 0x04, 0xFC,
                                                0x14, 0x14, 0x17, 0x10, 0x1F,
                                                0x10, 0x10, 0x1F, 0x10, 0x1F,
                                                0x14, 0x14, 0x14, 0x1F, 0x00,
                                                0x10, 0x10, 0x10, 0xF0, 0x00,
                                                0x00, 0x00, 0x00, 0x1F, 0x10,
                                                0x10, 0x10, 0x10, 0x1F, 0x10,
                                                0x10, 0x10, 0x10, 0xF0, 0x10,
                                                0x00, 0x00, 0x00, 0xFF, 0x10,
                                                0x10, 0x10, 0x10, 0x10, 0x10,
                                                0x10, 0x10, 0x10, 0xFF, 0x10,
                                                0x00, 0x00, 0x00, 0xFF, 0x14,
                                                0x00, 0x00, 0xFF, 0x00, 0xFF,
                                                0x00, 0x00, 0x1F, 0x10, 0x17,
                                                0x00, 0x00, 0xFC, 0x04, 0xF4,
                                                0x14, 0x14, 0x17, 0x10, 0x17,
                                                0x14, 0x14, 0xF4, 0x04, 0xF4,
                                                0x00, 0x00, 0xFF, 0x00, 0xF7,
                                                0x14, 0x14, 0x14, 0x14, 0x14,
                                                0x14, 0x14, 0xF7, 0x00, 0xF7,
                                                0x14, 0x14, 0x14, 0x17, 0x14,
                                                0x10, 0x10, 0x1F, 0x10, 0x1F,
                                                0x14, 0x14, 0x14, 0xF4, 0x14,
                                                0x10, 0x10, 0xF0, 0x10, 0xF0,
                                                0x00, 0x00, 0x1F, 0x10, 0x1F,
                                                0x00, 0x00, 0x00, 0x1F, 0x14,
                                                0x00, 0x00, 0x00, 0xFC, 0x14,
                                                0x00, 0x00, 0xF0, 0x10, 0xF0,
                                                0x10, 0x10, 0xFF, 0x10, 0xFF,
                                                0x14, 0x14, 0x14, 0xFF, 0x14,
                                                0x10, 0x10, 0x10, 0x1F, 0x00,
                                                0x00, 0x00, 0x00, 0xF0, 0x10,
                                                0xFF, 0xFF, 0xFF, 0xFF, 0xFF,
                                                0xF0, 0xF0, 0xF0, 0xF0, 0xF0,
                                                0xFF, 0xFF, 0xFF, 0x00, 0x00,
                                                0x00, 0x00, 0x00, 0xFF, 0xFF,
                                                0x0F, 0x0F, 0x0F, 0x0F, 0x0F,
                                                0x38, 0x44, 0x44, 0x38, 0x44,
                                                0xFC, 0x4A, 0x4A, 0x4A, 0x34, // sharp-s or beta
                                                0x7E, 0x02, 0x02, 0x06, 0x06,
                                                0x02, 0x7E, 0x02, 0x7E, 0x02,
                                                0x63, 0x55, 0x49, 0x41, 0x63,
                                                0x38, 0x44, 0x44, 0x3C, 0x04,
                                                0x40, 0x7E, 0x20, 0x1E, 0x20,
                                                0x06, 0x02, 0x7E, 0x02, 0x02,
                                                0x99, 0xA5, 0xE7, 0xA5, 0x99,
                                                0x1C, 0x2A, 0x49, 0x2A, 0x1C,
                                                0x4C, 0x72, 0x01, 0x72, 0x4C,
                                                0x30, 0x4A, 0x4D, 0x4D, 0x30,
                                                0x30, 0x48, 0x78, 0x48, 0x30,
                                                0xBC, 0x62, 0x5A, 0x46, 0x3D,
                                                0x3E, 0x49, 0x49, 0x49, 0x00,
                                                0x7E, 0x01, 0x01, 0x01, 0x7E,
                                                0x2A, 0x2A, 0x2A, 0x2A, 0x2A,
                                                0x44, 0x44, 0x5F, 0x44, 0x44,
                                                0x40, 0x51, 0x4A, 0x44, 0x40,
                                                0x40, 0x44, 0x4A, 0x51, 0x40,
                                                0x00, 0x00, 0xFF, 0x01, 0x03,
                                                0xE0, 0x80, 0xFF, 0x00, 0x00,
                                                0x08, 0x08, 0x6B, 0x6B, 0x08,
                                                0x36, 0x12, 0x36, 0x24, 0x36,
                                                0x06, 0x0F, 0x09, 0x0F, 0x06,
                                                0x00, 0x00, 0x18, 0x18, 0x00,
                                                0x00, 0x00, 0x10, 0x10, 0x00,
                                                0x30, 0x40, 0xFF, 0x01, 0x01,
                                                0x00, 0x1F, 0x01, 0x01, 0x1E,
                                                0x00, 0x19, 0x1D, 0x17, 0x12,
                                                0x00, 0x3C, 0x3C, 0x3C, 0x3C,
                                                0x00, 0x00, 0x00, 0x00, 0x00  // #255 NBSP
};

uint8_t oled_buffer[DISPLAY_BUFF_SIZE] = {0x40};

void stringToByteArray(const char* str, uint8_t* buf, size_t bufSize) {
  size_t len = strlen(str);
  if (len > bufSize * 6) {
    len = bufSize * 6;
  }

  size_t row = 0, col = 0;
  memset(buf, 0, bufSize);

  for (size_t i = 0; i < len; i++) {
    uint8_t c = str[i];

    if (c == '\n') {
      row++;
      col = 0;
      continue;
    }

    for (size_t j = 0; j < 5; j++) {
      buf[row * SSD1306_LCDWIDTH + col + j] = ssd1306_font5x7[c * 5 + j];
    }
    col += 6; // Add a blank column after each character

    if (col + 5 >= SSD1306_LCDWIDTH) {
      row++;
      col = 0;
    }

    if (row >= SSD1306_LCDHEIGHT / 8) {
      break;
    }
  }
}

char* get_datetime()
{
    char *datetime_str = malloc(50);
    time_t rawtime;
    struct tm *timeinfo;

    // Set the locale to GB
    setlocale(LC_TIME, "en_GB.UTF-8");

    // Get the current time
    time(&rawtime);
    timeinfo = localtime(&rawtime);

    // Format and return the datetime string
    strftime(datetime_str, 50, "%d.%m.%Y %H:%M:%S", timeinfo);
    return datetime_str;
}

char* get_cpu_info()
{
    char *info = malloc(50);
    int freq_mhz;
    float temp_f;
    size_t freq_size = sizeof(freq_mhz);

    // Get CPU frequency
    sysctlbyname("dev.cpu.0.freq", &freq_mhz, &freq_size, NULL, 0);

    // Get CPU temperature
    FILE *fp;
    char buffer[80];
    if ((fp = popen("sysctl -n dev.cpu.0.temperature", "r")) == NULL) {
        exit(1);
    }
    if (fgets(buffer, 80, fp) != NULL) {
        char* pch = strstr(buffer, "C");
        if (pch != NULL) {
            *pch = '\0';
        }
        temp_f = atof(buffer);
    }
    pclose(fp);
    // Format and return the string. No way to get deg working as single byte char
    snprintf(info, 50, "CPU:%dMHz|%.1f C", freq_mhz, temp_f);
    //snprintf(info, 50, "CPU: %dMHz %.1f%cC", freq_mhz, temp_f, '\xF7');
    return info;
}

char* get_adapter_ips(const char* interface_list) {
    struct ifaddrs *ifaddr, *ifa;
    char *ip_list = NULL;
    int family, s;
    char host[NI_MAXHOST];
    char *interface_name, *next_interface_name;
    char *interface_list_copy = strdup(interface_list);  // Make a copy of the interface list
    
    // Loop through the list of interfaces
    if (getifaddrs(&ifaddr) == -1) {
        perror("getifaddrs");
        exit(EXIT_FAILURE);
    }
    
    for (ifa = ifaddr; ifa != NULL; ifa = ifa->ifa_next) {
        if (ifa->ifa_addr == NULL) {
            continue;
        }
        
        // Check if the interface is a physical network adapter and is in the list of interfaces to include
        if (ifa->ifa_flags & IFF_UP && ifa->ifa_flags & IFF_RUNNING && !(ifa->ifa_flags & IFF_LOOPBACK) && (ifa->ifa_flags & IFF_BROADCAST || ifa->ifa_flags & IFF_POINTOPOINT)) {
            interface_name = ifa->ifa_name;
            next_interface_name = strtok((next_interface_name == NULL) ? interface_list_copy : NULL, ",");
            while (next_interface_name != NULL) {
                if (strcmp(interface_name, next_interface_name) == 0) {
                    
                    // Get the IP address of the interface
                    family = ifa->ifa_addr->sa_family;
                    if (family == AF_INET || family == AF_INET6) {
                        s = getnameinfo(ifa->ifa_addr, (family == AF_INET) ? sizeof(struct sockaddr_in) : sizeof(struct sockaddr_in6),
                                        host, NI_MAXHOST, NULL, 0, NI_NUMERICHOST);
                        if (s != 0) {
                            printf("getnameinfo() failed: %s\n", gai_strerror(s));
                            exit(EXIT_FAILURE);
                        }
                        
                        // Append the adapter name and IP address to the output string
                        if (ip_list == NULL) {
                            ip_list = malloc(strlen(interface_name) + strlen(host) + 4);
                            sprintf(ip_list, "%s:%s", interface_name, host);
                        } else {
                            ip_list = realloc(ip_list, strlen(ip_list) + strlen(interface_name) + strlen(host) + 4);
                            sprintf(ip_list + strlen(ip_list), "\n%s:%s", interface_name, host);
                        }
                        
                        break;
                    }
                }
                next_interface_name = strtok(NULL, ",");
            }
        }
    }
    
    freeifaddrs(ifaddr);
    free(interface_list_copy);
    
    return ip_list;
}

static int
write_data(int fd, uint8_t data)
{
  int ret;
  uint8_t cmdbuff[1000] = {0};
  struct iic_msg write_cmd[1] = {
				 {
				  .slave = IICADDR << 1 | !IIC_M_RD,
				  .flags = !IIC_M_RD,
				  //.len = sizeof (data),
				  //.buf = &data,
				  .len = sizeof (oled_buffer),
				  .buf = oled_buffer,
				 }, 
  };

  struct iic_rdwr_data cmd = {
			      .msgs =  write_cmd,
			      .nmsgs = 1,
  };

  ret=ioctl(fd, I2CRDWR, &cmd);
  return 0;
}

static int
ssd1306_command(int fd, uint8_t data)
{
  uint8_t cmdbuff[12] = {0};
  struct iic_msg write_cmd[1] = {
				 {
				  .slave = IICADDR << 1 | !IIC_M_RD,
				  .flags = !IIC_M_RD,
				  .len = 2*sizeof (uint8_t),
				  .buf = cmdbuff,
				 }, 
  };

  struct iic_rdwr_data cmd = {
			      .msgs =  write_cmd,
			      .nmsgs = 1,
  };

  cmdbuff[0]=0;
  cmdbuff[1]=data;

  ioctl(fd, I2CRDWR, &cmd);
  return 0;
}

int main(int argc, char *argv[])
{
  int i;
  int fd;
  uint8_t cmdbuff[12] = {0};
  uint8_t buf[1]={0};
  uint8_t retbuf[1]={0};
  struct iic_msg write_cmd[2] = {
				 {
				  .slave = IICADDR << 1 | !IIC_M_RD,
				  .flags = !IIC_M_RD,
				  .len = sizeof (uint8_t),
				  .buf = cmdbuff,
				 }, 
				 {
				  .slave = IICADDR << 1 | IIC_M_RD,
				  .flags = IIC_M_RD,
				  .len = sizeof (uint8_t),
				  .buf = retbuf,
				 }, 
  };
  struct iic_rdwr_data cmd = {
			      .msgs =  write_cmd,
			      .nmsgs = 2,
  };
  if ((fd = open(IICBUS, O_RDWR)) == -1) {
    fprintf(stderr, "%d Error %s\n", __LINE__, strerror(errno));
    return 1;
  }

  /* Initialize */
  ssd1306_command(fd,SSD1306_DISPLAYOFF);
  ssd1306_command(fd,SSD1306_SETDISPLAYCLOCKDIV);
  ssd1306_command(fd,0x80);

  ssd1306_command(fd,SSD1306_SETMULTIPLEX);
  ssd1306_command(fd,SSD1306_LCDHEIGHT - 1);

  ssd1306_command(fd,SSD1306_SETDISPLAYOFFSET);
  ssd1306_command(fd,0x0);
  ssd1306_command(fd,SSD1306_SETSTARTLINE | 0x0);
  ssd1306_command(fd,SSD1306_CHARGEPUMP);

  ssd1306_command(fd,0x14);

  ssd1306_command(fd,SSD1306_MEMORYMODE);
  ssd1306_command(fd,0x00);
  ssd1306_command(fd,SSD1306_SEGREMAP | 0x1);
  ssd1306_command(fd,SSD1306_COMSCANDEC);

  ssd1306_command(fd,SSD1306_SETCOMPINS);
  // ssd1306_command(fd,0x12);
  ssd1306_command(fd,0x02);
  ssd1306_command(fd,SSD1306_SETCONTRAST);

  // ssd1306_command(fd,0xCF);
  ssd1306_command(fd,0x8F);

  ssd1306_command(fd,SSD1306_SETPRECHARGE);

  ssd1306_command(fd,0xF1);

  ssd1306_command(fd,SSD1306_SETVCOMDETECT);
  ssd1306_command(fd,0x40);
  ssd1306_command(fd,SSD1306_DISPLAYALLON_RESUME);
  ssd1306_command(fd,SSD1306_NORMALDISPLAY);

//  ssd1306_command(fd,SSD1306_DEACTIVATE_SCROLL);

  ssd1306_command(fd,SSD1306_DISPLAYON);

  sleep(1);
  /* end of init */


  /* display */
  ssd1306_command(fd,SSD1306_COLUMNADDR);

  //ssd1306_command(fd,32);
  ssd1306_command(fd,0);
  //ssd1306_command(fd,32 + SSD1306_LCDWIDTH - 1);
  ssd1306_command(fd,SSD1306_LCDWIDTH - 1);

  ssd1306_command(fd,SSD1306_PAGEADDR);
  ssd1306_command(fd,0);
  ssd1306_command(fd,(SSD1306_LCDHEIGHT / 8) - 1);
  while (1) {
    // Build the string to put onto the display
    char *ipAddress = get_adapter_ips("genet0,re0");
    char *cpu = get_cpu_info();
    char *date = get_datetime();
    char display_string[100];
    strcpy(display_string,date);
    strcat(display_string, "\n");
    strcat(display_string,cpu);
    strcat(display_string, "\n");
    strcat(display_string,ipAddress);
    stringToByteArray(display_string, &oled_buffer[1], DISPLAY_BUFF_SIZE - 1);	

    write_data(fd,oled_buffer[0]);
    sleep(1); // Pause for 1 second before next execution
  }
  close(fd);
  return 0;
}

