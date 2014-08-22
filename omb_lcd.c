#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/ioctl.h>
#include <string.h>

#include <ft2build.h>
#include FT_FREETYPE_H

#include "omb_common.h"
#include "omb_log.h"
#include "omb_lcd.h"

#ifndef LCD_IOCTL_ASC_MODE
#define LCDSET					0x1000
#define LCD_IOCTL_ASC_MODE		(21|LCDSET)
#define	LCD_MODE_ASC			0
#define	LCD_MODE_BIN			1
#endif

#define RED(x)   (x >> 16) & 0xff;
#define GREEN(x) (x >> 8) & 0xff;
#define BLUE(x)   x & 0xff;

static int omb_lcd_fd = -1;
static int omb_lcd_width = 0;
static int omb_lcd_height = 0;
static int omb_lcd_stride = 0;
static int omb_lcd_bpp = 0;
static unsigned char *omb_lcd_buffer = NULL;


int omb_lcd_read_value(const char *filename)
{
	int value = 0;
	FILE *fd = fopen(filename, "r");
	if (fd) {
		int tmp;
		if (fscanf(fd, "%x", &tmp) == 1)
			value = tmp;
		fclose(fd);
	}
	return value;
}

int omb_lcd_open()
{
	omb_lcd_fd = open("/dev/dbox/lcd0", O_RDWR);
	if (omb_lcd_fd == -1) {
		omb_log(LOG_ERROR, "cannot open lcd device");
		return OMB_ERROR;
	}
	
	int tmp = LCD_MODE_BIN;
	if (ioctl(omb_lcd_fd, LCD_IOCTL_ASC_MODE, &tmp)) {
		omb_log(LOG_ERROR, "failed to set lcd bin mode");
		return OMB_ERROR;
	}
	
	omb_lcd_width = omb_lcd_read_value(OMB_LCD_XRES);
	if (omb_lcd_width == 0) {
		omb_log(LOG_ERROR, "cannot read lcd x resolution");
		return OMB_ERROR;
	}
	
	omb_lcd_height = omb_lcd_read_value(OMB_LCD_YRES);
	if (omb_lcd_height == 0) {
		omb_log(LOG_ERROR, "cannot read lcd y resolution");
		return OMB_ERROR;
	}
	omb_lcd_bpp = omb_lcd_read_value(OMB_LCD_BPP);
	if (omb_lcd_bpp == 0) {
		omb_log(LOG_ERROR, "cannot read lcd bpp");
		return OMB_ERROR;
	}
	
	omb_lcd_stride = omb_lcd_width * (omb_lcd_bpp / 8);
	omb_lcd_buffer = malloc(omb_lcd_height * omb_lcd_stride);
	
	omb_log(LOG_DEBUG, "current lcd is %dx%d, %dbpp, stride %d", omb_lcd_width, omb_lcd_height, omb_lcd_bpp, omb_lcd_stride);
	
	return OMB_SUCCESS;
}

int omb_lcd_get_width()
{
	return omb_lcd_width;
}

int omb_lcd_get_height()
{
	return omb_lcd_height;
}

void omb_lcd_clear()
{
	if (!omb_lcd_buffer)
		return;
	
	memset(omb_lcd_buffer, '\0', omb_lcd_height * omb_lcd_stride);
}

void omb_lcd_update()
{
	if (!omb_lcd_buffer)
		return;
	
	write(omb_lcd_fd, omb_lcd_buffer, omb_lcd_height * omb_lcd_stride);
}

void omb_lcd_close()
{
	if (omb_lcd_fd >= 0)
		close(omb_lcd_fd);
	
	if (omb_lcd_buffer)
		free(omb_lcd_buffer);
}

void omb_lcd_draw_character(FT_Bitmap* bitmap, FT_Int x, FT_Int y, int color)
{
	int i, j, z = 0;
	long int location = 0;
	unsigned char red = RED(color);
	unsigned char green = GREEN(color);
	unsigned char blue = BLUE(color);
	
	red = (red >> 3) & 0x1f;
	green = (green >> 3) & 0x1f;
	blue = (blue >> 3) & 0x1f;
	
	for (i = y; i < y + bitmap->rows; i++) {
		for (j = x; j < x + bitmap->width; j++) {
			//if (i < 0 || j < 0 || i > omb_var_screen_info.yres || j > omb_var_screen_info.xres) {
			//	z++;
			//	continue;
			//}
			
			if (bitmap->buffer[z] != 0x00) {
				location = (j * (omb_lcd_bpp / 8)) +
					(i * omb_lcd_stride);
			
				omb_lcd_buffer[location] = red << 3 | green >> 2;
				omb_lcd_buffer[location + 1] = green << 6 | blue << 1;
				//*(omb_fb_map + location) = blue;
				//*(omb_fb_map + location + 1) = green;
				//*(omb_fb_map + location + 2) = red;
			}
			z++;
		}
	}
}
