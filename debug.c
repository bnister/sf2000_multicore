/* Copyright (C) 2023 Nikita Burnashev

Permission to use, copy, modify, and/or distribute this software
for any purpose with or without fee is hereby granted.

THIS SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND! */

#include <stdio.h>
#include <stdarg.h>
#include <string.h>

extern unsigned long PINMUXL;
extern unsigned long PINMUXT;
extern unsigned long GPIOLCTRL;
extern unsigned long GPIOTCTRL;

static char lcd_buf[24][53];
static unsigned lcd_y, lcd_x;

static void lcd_init(void)
{
	memset(lcd_buf, ' ', sizeof lcd_buf);
	lcd_y = lcd_x = 0;
}

static int lcd_vprintf(const char* fmt, va_list ap)
{
	int ret;
	char buf[999];

	ret = vsnprintf(buf, sizeof buf, fmt, ap);

	for (char *pc = buf; *pc; pc++) {
		if (lcd_x >= sizeof lcd_buf[0]) {
			lcd_x = 0;
			lcd_y++;
		}
		if (lcd_y >= sizeof lcd_buf / sizeof lcd_buf[0])
			continue;
		if (*pc == '\n') {
			lcd_x = 0;
			lcd_y++;
		}
		else if (*pc >= ' ') {
			lcd_buf[lcd_y][lcd_x] = *pc;
			lcd_x++;
		}
	}
	return ret;
}

static void lcd_printf(const char *fmt, ...)
{
	va_list ap;
	va_start(ap, fmt);
	lcd_vprintf(fmt, ap);
	va_end(ap);
}

static void lcd_send(unsigned short data)
{
	*((volatile unsigned *)&GPIOLCTRL + 4) = // clear L10 (CS), L07 (WR); L02-06 <- D0-4
		*((volatile unsigned *)&GPIOLCTRL + 4) & 0xfffffb03 | data << 2 & 0x7c;
	*((volatile unsigned *)&GPIOTCTRL + 4) = // T09-14 <- D5-10, D11-15 -> T02-06, tDST
		*((volatile unsigned *)&GPIOTCTRL + 4) & 0xffff8183 | data << 4 & 0x7e00 | data >> 9 & 0x7c;
	*((volatile unsigned *)&GPIOLCTRL + 4) |= 1 << 7; // set L07 (WR), tCSH
	*((volatile unsigned *)&GPIOLCTRL + 4) |= 1 << 10; // set L10 (CS)
}

static void lcd_send_cmd(unsigned char cmd)
{
	*((volatile unsigned *)&GPIOTCTRL + 4) &= ~(1 << 1); // clear T01 (RS)
	lcd_send(cmd);
}

static void lcd_send_data(unsigned short data)
{
	*((volatile unsigned *)&GPIOTCTRL + 4) |= 1 << 1; // set T01 (RS)
	lcd_send(data);
}

/* borrowed from RetroArch/gfx/drivers_font_renderer, usual license restrictions apply */
#define FONT_WIDTH 5
#define FONT_HEIGHT 10

#define FONT_WIDTH_STRIDE (FONT_WIDTH + 1)

#define FONT_OFFSET(x) ((x - 0x20) * ((FONT_HEIGHT * FONT_WIDTH + 7) / 8))

static const unsigned char lcd_font[] = {
   0x00,0x00,0x00,0x00,0x00,0x00,0x00, /* code=0x20 */
   0x80,0x10,0x42,0x08,0x20,0x00,0x00, /* code=0x21 */
   0x4A,0x29,0x00,0x00,0x00,0x00,0x00, /* code=0x22 */
   0x00,0xA8,0xAF,0xD4,0x57,0x00,0x00, /* code=0x23 */
   0x80,0xF8,0xE2,0xE8,0x23,0x00,0x00, /* code=0x24 */
   0x60,0x4E,0x44,0x44,0xCE,0x00,0x00, /* code=0x25 */
   0xC0,0xA4,0x64,0x6A,0xB2,0x00,0x00, /* code=0x26 */
   0x84,0x10,0x00,0x00,0x00,0x00,0x00, /* code=0x27 */
   0x88,0x08,0x21,0x84,0x20,0x08,0x00, /* code=0x28 */
   0x82,0x20,0x84,0x10,0x22,0x02,0x00, /* code=0x29 */
   0x00,0x90,0xEA,0x2A,0x01,0x00,0x00, /* code=0x2a */
   0x00,0x10,0xF2,0x09,0x01,0x00,0x00, /* code=0x2b */
   0x00,0x00,0x00,0x00,0x20,0x02,0x00, /* code=0x2c */
   0x00,0x00,0xF0,0x01,0x00,0x00,0x00, /* code=0x2d */
   0x00,0x00,0x00,0x00,0x20,0x00,0x00, /* code=0x2e */
   0x10,0x22,0x44,0x88,0x10,0x01,0x00, /* code=0x2f */
   0xC0,0xC5,0x58,0x63,0x74,0x00,0x00, /* code=0x30 */
   0x80,0x18,0x42,0x08,0x71,0x00,0x00, /* code=0x31 */
   0xC0,0x45,0xC8,0x44,0xF8,0x00,0x00, /* code=0x32 */
   0xC0,0x45,0xC8,0x60,0x74,0x00,0x00, /* code=0x33 */
   0x00,0x31,0x95,0x3E,0x42,0x00,0x00, /* code=0x34 */
   0xE0,0x87,0xF0,0x60,0x74,0x00,0x00, /* code=0x35 */
   0x80,0x89,0xF0,0x62,0x74,0x00,0x00, /* code=0x36 */
   0xE0,0x43,0x84,0x08,0x11,0x00,0x00, /* code=0x37 */
   0xC0,0xC5,0xE8,0x62,0x74,0x00,0x00, /* code=0x38 */
   0xC0,0xC5,0xE8,0x21,0x32,0x00,0x00, /* code=0x39 */
   0x00,0x00,0x02,0x00,0x01,0x00,0x00, /* code=0x3a */
   0x00,0x00,0x02,0x00,0x11,0x00,0x00, /* code=0x3b */
   0x00,0x40,0x36,0x18,0x04,0x00,0x00, /* code=0x3c */
   0x00,0x80,0x0F,0x3E,0x00,0x00,0x00, /* code=0x3d */
   0x00,0x04,0x83,0x4D,0x00,0x00,0x00, /* code=0x3e */
   0xC0,0x45,0x88,0x08,0x20,0x00,0x00, /* code=0x3f */
   0xC0,0xC5,0x5A,0x7B,0xF0,0x00,0x00, /* code=0x40 */
   0x80,0x10,0xA5,0x5C,0x8C,0x00,0x00, /* code=0x41 */
   0xE0,0xC5,0xF8,0x62,0x7C,0x00,0x00, /* code=0x42 */
   0xC0,0xC5,0x10,0x42,0x74,0x00,0x00, /* code=0x43 */
   0xE0,0xA4,0x18,0x63,0x3A,0x00,0x00, /* code=0x44 */
   0xE0,0x87,0xF0,0x42,0xF8,0x00,0x00, /* code=0x45 */
   0xE0,0x87,0xF0,0x42,0x08,0x00,0x00, /* code=0x46 */
   0xC0,0xC5,0x90,0x63,0xF4,0x00,0x00, /* code=0x47 */
   0x20,0xC6,0xF8,0x63,0x8C,0x00,0x00, /* code=0x48 */
   0xC0,0x11,0x42,0x08,0x71,0x00,0x00, /* code=0x49 */
   0x80,0x43,0x08,0x21,0x7C,0x00,0x00, /* code=0x4a */
   0x20,0xA6,0x32,0x4A,0x8A,0x00,0x00, /* code=0x4b */
   0x20,0x84,0x10,0x42,0xF8,0x00,0x00, /* code=0x4c */
   0x20,0xC6,0xBD,0x6B,0x8D,0x00,0x00, /* code=0x4d */
   0x60,0xCE,0x5A,0x6B,0xCE,0x00,0x00, /* code=0x4e */
   0xC0,0xC5,0x18,0x63,0x74,0x00,0x00, /* code=0x4f */
   0xE0,0xC5,0xF8,0x42,0x08,0x00,0x00, /* code=0x50 */
   0xC0,0xC5,0x18,0x63,0xF6,0x00,0x00, /* code=0x51 */
   0xE0,0xC5,0xF8,0x62,0x8C,0x00,0x00, /* code=0x52 */
   0xC0,0xC5,0xE0,0x60,0x74,0x00,0x00, /* code=0x53 */
   0xE0,0x13,0x42,0x08,0x21,0x00,0x00, /* code=0x54 */
   0x20,0xC6,0x18,0x63,0x74,0x00,0x00, /* code=0x55 */
   0x20,0xC6,0xA8,0x14,0x21,0x00,0x00, /* code=0x56 */
   0x20,0xD6,0x5A,0x95,0x52,0x00,0x00, /* code=0x57 */
   0x20,0x46,0x45,0x54,0x8C,0x00,0x00, /* code=0x58 */
   0x20,0xC6,0xE8,0x08,0x21,0x00,0x00, /* code=0x59 */
   0xE0,0x43,0x44,0x44,0xF8,0x00,0x00, /* code=0x5a */
   0x4E,0x08,0x21,0x84,0x10,0x0E,0x00, /* code=0x5b */
   0x21,0x08,0x41,0x08,0x42,0x10,0x00, /* code=0x5c */
   0x0E,0x21,0x84,0x10,0x42,0x0E,0x00, /* code=0x5d */
   0x80,0xA8,0x08,0x00,0x00,0x00,0x00, /* code=0x5e */
   0x00,0x00,0x00,0x00,0x00,0x1F,0x00, /* code=0x5f */
   0x80,0x20,0x00,0x00,0x00,0x00,0x00, /* code=0x60 */
   0x00,0x00,0x07,0x7D,0xF4,0x00,0x00, /* code=0x61 */
   0x21,0x84,0x17,0x63,0x7C,0x00,0x00, /* code=0x62 */
   0x00,0x00,0x1F,0x42,0xF0,0x00,0x00, /* code=0x63 */
   0x10,0x42,0x1F,0x63,0xF4,0x00,0x00, /* code=0x64 */
   0x00,0x00,0x17,0x7F,0xF0,0x00,0x00, /* code=0x65 */
   0x5C,0x88,0x27,0x84,0x10,0x00,0x00, /* code=0x66 */
   0x00,0x00,0x17,0x63,0xF4,0xD0,0x01, /* code=0x67 */
   0x21,0x84,0x17,0x63,0x8C,0x00,0x00, /* code=0x68 */
   0x80,0x00,0x43,0x08,0x21,0x00,0x00, /* code=0x69 */
   0x00,0x01,0x86,0x10,0x42,0xE8,0x00, /* code=0x6a */
   0x42,0x08,0xA9,0x8C,0x92,0x00,0x00, /* code=0x6b */
   0x86,0x10,0x42,0x08,0x21,0x00,0x00, /* code=0x6c */
   0x00,0x80,0x55,0x6B,0xAD,0x00,0x00, /* code=0x6d */
   0x00,0x80,0x17,0x63,0x8C,0x00,0x00, /* code=0x6e */
   0x00,0x00,0x17,0x63,0x74,0x00,0x00, /* code=0x6f */
   0x00,0x80,0x17,0x63,0x7C,0x21,0x00, /* code=0x70 */
   0x00,0x00,0x1F,0x63,0xF4,0x10,0x02, /* code=0x71 */
   0x00,0x80,0x36,0x43,0x08,0x00,0x00, /* code=0x72 */
   0x00,0x00,0x1F,0x1C,0x7C,0x00,0x00, /* code=0x73 */
   0x40,0x08,0x27,0x84,0xE0,0x00,0x00, /* code=0x74 */
   0x00,0x80,0x18,0x63,0xF4,0x00,0x00, /* code=0x75 */
   0x00,0x80,0x18,0x95,0x22,0x00,0x00, /* code=0x76 */
   0x00,0x80,0x58,0xAB,0x52,0x00,0x00, /* code=0x77 */
   0x00,0x80,0xA8,0x88,0x8A,0x00,0x00, /* code=0x78 */
   0x00,0x80,0x18,0x63,0xF4,0xD0,0x01, /* code=0x79 */
   0x00,0x80,0x8F,0x88,0xF8,0x00,0x00, /* code=0x7a */
   0x88,0x10,0x22,0x08,0x21,0x08,0x00, /* code=0x7b */
   0x84,0x10,0x42,0x08,0x21,0x04,0x00, /* code=0x7c */
   0x82,0x10,0x82,0x08,0x21,0x02,0x00, /* code=0x7d */
   0x00,0x00,0x60,0x1B,0x00,0x00,0x00, /* code=0x7e */
   0x00,0x00,0x00,0x00,0x00,0x00,0x00  /* code=0x7f */
};

static void lcd_flush(void)
{
	*((volatile unsigned *)&PINMUXL + 0) &= 0xffff; // L02-03 (D0-1)
	*((volatile unsigned *)&PINMUXL + 1) = 0; // L04-07 (D2-4, WR)
	*((volatile unsigned *)&PINMUXL + 2) &= 0xff00ffff; // L10 (CS)

	*((volatile unsigned *)&PINMUXT + 0) &= 0xff; // T01-03 (RS, D11-12)
	*((volatile unsigned *)&PINMUXT + 1) &= 0xff000000; // T04-06 (D13-15)
	*((volatile unsigned *)&PINMUXT + 2) &= 0xff; // T09-11 (D5-D7)
	*((volatile unsigned *)&PINMUXT + 3) &= 0xff000000; // T12-14 (D8-10)

	lcd_send_cmd(0x2a); // CASET
	lcd_send_data(0);
	lcd_send_data(0);
	lcd_send_data((320 - 1) >> 8);
	lcd_send_data((320 - 1) & 255);

	lcd_send_cmd(0x2b); // RASET
	lcd_send_data(0);
	lcd_send_data(0);
	lcd_send_data((240 - 1) >> 8);
	lcd_send_data((240 - 1) & 255);

	lcd_send_cmd(0x2c); // RAMWR
	for (unsigned y = 0; y < 240; y++) {
		for (unsigned x = 0; x < 320; x++) {
			unsigned symbol_index = lcd_buf[y / FONT_HEIGHT][x / FONT_WIDTH_STRIDE];
			unsigned i = (x % FONT_WIDTH_STRIDE), j = y % FONT_HEIGHT;
			unsigned char rem = 1 << ((i + j * FONT_WIDTH) & 7);
			unsigned offset = (i + j * FONT_WIDTH) >> 3;

			if (i < FONT_WIDTH && (lcd_font[FONT_OFFSET(symbol_index) + offset] & rem) > 0)
				lcd_send_data(0xffff); // white
			else
				lcd_send_data(0x1f); // blue
		}
	}
}

extern void os_disable_interrupt(void);

void lcd_bsod(const char *fmt, ...)
{
	os_disable_interrupt();
	lcd_init();
	lcd_printf("\n "); // guard against some screen misalignment

	va_list ap;
	va_start(ap, fmt);
	lcd_vprintf(fmt, ap);
	va_end(ap);

	lcd_flush();
	do {
	} while (1);
}