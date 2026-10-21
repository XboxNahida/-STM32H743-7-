#ifndef OSCILLOSCOPE_FORMAT_H
#define OSCILLOSCOPE_FORMAT_H
#include <math.h>
#include <stddef.h>
#include <stdio.h>
#include <string.h>

typedef enum { SCOPE_UNIT_MV, SCOPE_UNIT_SECONDS, SCOPE_UNIT_HZ,
               SCOPE_UNIT_RATE, SCOPE_UNIT_PERCENT } scope_unit_t;

/* 固定小数，不使用科学计数；输入框保留原单位，去除无意义尾零。 */
static inline void scope_format_fixed(char *out,size_t size,double value,unsigned decimals)
{
    size_t n;double step=1.0;unsigned i;
    if(!out||!size)return;
    if(!isfinite(value)){(void)snprintf(out,size,"--");return;}
    if(decimals>9U)decimals=9U;
    for(i=0U;i<decimals;++i)step*=0.1;
    if(fabs(value)<step*0.5)value=0.0;
    (void)snprintf(out,size,"%.*f",(int)decimals,value);
    if(!strchr(out,'.'))return;
    n=strlen(out);
    while(n&&out[n-1U]=='0')out[--n]='\0';
    if(n&&out[n-1U]=='.')out[n-1U]='\0';
}

/* 显示用工程单位：电压参数以mV传入，其余以SI基础单位传入。 */
static inline void scope_format_quantity_precision(char *out,size_t size,double value,scope_unit_t unit,unsigned decimals)
{
    static const char *const volts[]={"nV","uV","mV","V","kV","MV"};
    static const char *const seconds[]={"ns","us","ms","s"};
    static const char *const hz[]={"mHz","Hz","kHz","MHz","GHz"};
    static const char *const rate[]={"Sa/s","kSa/s","MSa/s","GSa/s"};
    const char *suffix="%";double v=value;unsigned index=0U,last=0U;
    const char *const *names=NULL;char number[40];
    if(!out||!size)return;
    if(!isfinite(v)){(void)snprintf(out,size,"--");return;}
    switch(unit){
    case SCOPE_UNIT_MV:names=volts;index=2U;last=5U;break;
    case SCOPE_UNIT_SECONDS:names=seconds;index=3U;last=3U;break;
    case SCOPE_UNIT_HZ:names=hz;index=1U;last=4U;break;
    case SCOPE_UNIT_RATE:names=rate;last=3U;break;
    default:break;
    }
    if(names){
        while(fabs(v)>=999.995&&index<last){v/=1000.0;++index;}
        while(v!=0.0&&fabs(v)<0.999995&&index>0U){v*=1000.0;--index;}
        suffix=names[index];
    }
    scope_format_fixed(number,sizeof(number),v,decimals);
    (void)snprintf(out,size,"%s %s",number,suffix);
}
static inline void scope_format_quantity(char *out,size_t size,double value,scope_unit_t unit)
{ scope_format_quantity_precision(out,size,value,unit,2U); }
#endif
