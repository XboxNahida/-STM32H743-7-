#include "oscilloscope_zoom.h"
#include <assert.h>
#include <math.h>
#include <stdint.h>
#include <stdio.h>

int main(void)
{
    uint32_t rate,count;float div,scale;scope_zoom_axis_t axis;
    uint32_t j,next_rate,next_count;
    for(j=512U;j<=8192U;j+=16U){
        div=scope_zoom_time_div(1000000U,j);
        assert(scope_zoom_horizontal(1000000U,div,0,&next_rate,&next_count));
        assert(next_rate==1000000U&&next_count==j);
    }
    assert(scope_zoom_auto(1.0f,&rate,&count,&div));
    assert(rate==2000U&&count==5008U&&fabsf(div-0.25f)<0.0001f);
    assert(scope_zoom_auto(200000.0f,&rate,&count,&div));
    assert(rate==1000000U&&count==512U&&fabsf(div-0.00000125f)<1e-10f);
    for(j=1U;j<=200000U;j*=10U){
        assert(scope_zoom_auto((float)j,&rate,&count,&div));
        assert(div*10.0f*(float)j>=2.49f);
        assert(count>=512U&&count<=8192U&&count%16U==0U);
    }
    assert(scope_zoom_horizontal(100000U,0.1f,1000.0f,&rate,&count));
    /* 用户扩大观察范围时允许降Fs，欠采样由界面提示，不提前锁住时基。 */
    assert(rate==5000U&&count==5008U);
    assert(scope_zoom_horizontal(1000000U,0.1f,250000.0f,&rate,&count));
    assert(rate==5000U&&count==5008U);
    div=scope_zoom_time_div(rate,count);
    assert(fabsf(div-0.10016f)<1e-6f);
    assert(scope_zoom_horizontal(100000U,0.001f,1000.0f,&rate,&count));
    assert(rate==500000U&&count==5008U);
    assert(scope_zoom_horizontal(100000U,0.00001f,1000.0f,&rate,&count));
    assert(count==512U);
    assert(scope_zoom_pinch(100,35,145,51,&axis,&scale));
    assert(axis==SCOPE_ZOOM_HORIZONTAL&&scale>1.4f&&scale<1.5f);
    assert(scope_zoom_pinch(32,100,46,150,&axis,&scale));
    assert(axis==SCOPE_ZOOM_VERTICAL&&scale>1.4f&&scale<1.5f);
    assert(!scope_zoom_pinch(0,0,4,2,&axis,&scale));
    assert(!scope_zoom_pinch(100,0,0,100,&axis,&scale));
    assert(!scope_zoom_pinch(100,35,-100,-35,&axis,&scale));
    assert(scope_zoom_pinch(100,35,-145,-51,&axis,&scale));
    assert(axis==SCOPE_ZOOM_HORIZONTAL&&scale>1.4f);
    assert(scope_zoom_horizontal(100000U,0.000000001f,0,&rate,&count));
    assert(rate==1000000U&&count==512U);
    assert(scope_zoom_horizontal(100000U,10,0,&rate,&count));
    assert(rate==100U&&count==8192U&&scope_zoom_time_div(rate,count)==8.192f);
    assert(scope_zoom_horizontal(100000U,0.1f,200000,&rate,&count));
    assert(rate==5000U&&count==5008U);
    assert(!scope_zoom_horizontal(100000U,NAN,0,&rate,&count));
    for(uint32_t limit=1000000U;limit<=2000000U;limit+=1000000U){
        scope_zoom_set_rate_limit(limit);
        for(j=0U;j<SCOPE_RATE_COUNT;j++){
            for(int edge=-1;edge<=1;edge++){
                div=(float)(8192.0/(10.0*scope_rate_at(j))*(1.0+edge*0.0001));
                if(div>10.0f)continue;
                assert(scope_zoom_horizontal(limit,div,0,&rate,&count));
                assert(rate<=limit&&count>=512U&&count<=8192U&&count%16U==0U);
                assert(scope_zoom_horizontal(rate,scope_zoom_time_div(rate,count),0,&next_rate,&next_count));
                assert(rate==next_rate&&count==next_count);
            }
        }
        assert(scope_zoom_auto(200000.0f,&rate,&count,&div));
        assert(rate==limit&&count==512U);
        assert(fabsf(div*200000.0f-0.25f)<0.001f);
    }
    scope_zoom_set_rate_limit(1000000U);
    assert(scope_zoom_pair(0.25f,0.0000512f,&rate,&count,&div));
    assert(rate==5000U&&count==8192U&&fabsf(div-0.16384f)<1e-7f);
    assert(scope_zoom_pair(0.000001f,0.25f,&rate,&count,&div));
    assert(rate==2000U&&count==5008U&&fabsf(div-0.0001f)<1e-6f);
    assert(scope_zoom_pair(0.001f,0.0002f,&rate,&count,&div));
    assert(rate==500000U&&count==5008U);
    axis=SCOPE_ZOOM_NONE;
    assert(!scope_zoom_drag_axis(7,3,&axis)&&axis==SCOPE_ZOOM_NONE);
    assert(scope_zoom_drag_axis(12,4,&axis)&&axis==SCOPE_ZOOM_HORIZONTAL);
    assert(scope_zoom_drag_axis(15,200,&axis)&&axis==SCOPE_ZOOM_HORIZONTAL);
    assert(scope_zoom_drag_axis(-80,300,&axis)&&axis==SCOPE_ZOOM_HORIZONTAL);
    axis=SCOPE_ZOOM_NONE;
    assert(scope_zoom_drag_axis(2,-8,&axis)&&axis==SCOPE_ZOOM_VERTICAL);
    assert(scope_zoom_drag_axis(400,12,&axis)&&axis==SCOPE_ZOOM_VERTICAL);
    axis=SCOPE_ZOOM_NONE;
    assert(scope_zoom_drag_axis(INT32_MIN,1,&axis)&&axis==SCOPE_ZOOM_HORIZONTAL);
    assert(!scope_zoom_drag_axis(10,0,NULL));
    puts("PASS oscilloscope_zoom");return 0;
}
