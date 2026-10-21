#include "hander.h"

void led_poc(uint8_t sit,uint8_t state) 
{
	switch(sit)
	{
		case 0: 		//LED0
			switch(state)
			{
				case 0:
					HAL_GPIO_WritePin(GPIOB,GPIO_PIN_1,GPIO_PIN_SET);//灭(高电平)
				break;
				case 1:
					HAL_GPIO_WritePin(GPIOB,GPIO_PIN_1,GPIO_PIN_RESET);//亮(低电平)
				break;
				case 2:
					HAL_GPIO_TogglePin(GPIOB,GPIO_PIN_1);
				break;
			}
		break;
		case 1: 		//LED1
			switch(state)
			{
				case 0:
					HAL_GPIO_WritePin(GPIOB,GPIO_PIN_0,GPIO_PIN_SET);//灭(高电平)
				break;
				case 1:
					HAL_GPIO_WritePin(GPIOB,GPIO_PIN_0,GPIO_PIN_RESET);//亮(低电平)
				break;
				case 2:
					HAL_GPIO_TogglePin(GPIOB,GPIO_PIN_0);
				break;
			}
		break;
	}
}

uint8_t key_read(void)
{
	uint8_t num=0;
	if(HAL_GPIO_ReadPin(GPIOH,GPIO_PIN_3) == RESET) 	num = 0;		//Key0
	if(HAL_GPIO_ReadPin(GPIOH,GPIO_PIN_2) == RESET) 	num = 1;		//Key1
	if(HAL_GPIO_ReadPin(GPIOC,GPIO_PIN_13) == RESET) 	num = 2;		//Key2
	if(HAL_GPIO_ReadPin(GPIOA,GPIO_PIN_0) == SET) 		num = 3;		//WK_UP
	return num;
}

// ====== 单位转换计算函数 ======
// 电阻内部基本单位: Ω
// 电容内部基本单位: μF
//
// 将内部值转换为指定单位的数值(仅计算，不含格式化)
// type: 0=电阻, 1=电容
// unit: 'M','k','R','m','u','n','p','F'
// 返回: 转换后的浮点数
float rc_convert(float value, int type, char unit)
{
    if(type == 0) {
        // 电阻: 以Ω为基准
        switch(unit) {
            case 'M': return value * 1e-6f;   // 10^6 Ω -> MΩ
            case 'k': return value * 1e-3f;   // 10^3 Ω -> kΩ
            case 'R': return value;           // Ω 不变
            case 'm': return value * 1e3f;    // 10^-3 Ω -> mΩ
            case 'u': return value * 1e6f;    // 10^-6 Ω -> uΩ (罕见)
            default:  return value;
        }
    } else {
        // 电容: 以μF为基准
        switch(unit) {
            case 'F': return value * 1e-6f;       // F -> μF
            case 'm': return value * 1e3f;        // 千μF -> mF (value=0.001μF, 实际极少)
            case 'u': return value;               // μF 不变
            case 'n': return value * 1e3f;        // μF -> nF (value=0.001 -> 1nF)
            case 'p': return value * 1e6f;        // μF -> pF
            default:  return value;
        }
    }
}

// 解析用户输入: 数字+单位 -> 基本单位值
float rc_parse(const char *str, int type, int *ok)
{
    *ok = 0;
    if(str == NULL || !*str) return 0.0f;
    
    float val;
    char unit = 0;
    if(sscanf(str, "%f%c", &val, &unit) < 1) return 0.0f;
    
    *ok = 1;
    if(unit == 0) return val;  // 无单位, 默认Ω或μF
    
    // 反向计算: 用户输入值 -> 基本单位
    // 用 rc_convert 的逆运算
    if(type == 0) {
        switch(unit) {
            case 'M': return val * 1e6f;   // MΩ -> Ω
            case 'k': return val * 1e3f;   // kΩ -> Ω
            case 'R': return val;          // Ω
            case 'm': return val * 1e-3f;  // mΩ -> Ω
            default:  return val;
        }
    } else {
        switch(unit) {
            case 'F': return val * 1e6f;       // F -> μF
            case 'm': return val * 1e-3f;      // mF -> μF
            case 'u': return val;              // μF
            case 'n': return val * 1e-3f;      // nF -> μF
            case 'p': return val * 1e-6f;      // pF -> μF
            default:  return val;
        }
    }
}

// 自动选单位并格式化显示 (内部值 -> "10.0k" 这样)
void rc_format(float value, int type, char *buf, int size)
{
    float v = value;
    char u;
    
    if(type == 0) {
        if(v >= 1e6f)      { v = rc_convert(value, 0, 'M'); u = 'M'; }
        else if(v >= 1e3f) { v = rc_convert(value, 0, 'k'); u = 'k'; }
        else if(v >= 1.0) { u = 'R'; }
        else if(v >= 1e-3f){ v = rc_convert(value, 0, 'm'); u = 'm'; }
        else              { v = rc_convert(value, 0, 'u'); u = 'u'; }
    } else {
        if(v >= 1000.0f)     { v = rc_convert(value, 1, 'm'); u = 'm'; }
        else if(v >= 1.0f)   { u = 'u'; }
        else if(v >= 0.001f) { v = rc_convert(value, 1, 'n'); u = 'n'; }
        else                 { v = rc_convert(value, 1, 'p'); u = 'p'; }
    }
    snprintf(buf, size, "%.2f%c", v, u);
}


/* ---- LED blink proc: 5ms ON, 2.5s OFF (call from TIM3 ISR) ---- */
void led_blink_proc(void)
{
    static uint32_t next_toggle = 0;
    static uint8_t  led_on = 0;
    uint32_t now = HAL_GetTick();

    if ((int32_t)(now - next_toggle) < 0) return;

    if (led_on) {
        led_poc(0, 0);  /* OFF */
        next_toggle = now + 2500;
        led_on = 0;
    } else {
        led_poc(0, 1);  /* ON */
        next_toggle = now + 5;
        led_on = 1;
    }
}
