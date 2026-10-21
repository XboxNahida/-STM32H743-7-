#include "main.h"
#include <string.h>
#include <stdio.h>

void led_poc(uint8_t sit,uint8_t state);
void led_blink_proc(void);
uint8_t key_read(void);

float rc_convert(float value, int type, char unit);
float rc_parse(const char *str, int type, int *ok);
void rc_format(float value, int type, char *buf, int size);
	
