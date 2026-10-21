#include "oscilloscope_workspace.h"
#include "shared_buffer.h"
#include "sdram_layout.h"
#include <stddef.h>
typedef char scope_workspace_size_check[(sizeof(scope_workspace_t)<=SDRAM_SHARED_WORK_SIZE)?1:-1];
static scope_workspace_t *workspace;
#ifdef SCOPE_HOST_TEST
static scope_workspace_t host_workspace;
#endif
int scope_workspace_acquire(void) {
    void *address=NULL;
    if(shared_buffer_acquire(SHARED_BUFFER_OWNER_OSCILLOSCOPE,
        (uint32_t)sizeof(scope_workspace_t),&address)!=SHARED_BUFFER_OK)return 0;
#ifdef SCOPE_HOST_TEST
    workspace=&host_workspace;
#else
    workspace=(scope_workspace_t *)address;
#endif
    return 1;
}
void scope_workspace_release(void) {
    if(workspace){
        (void)shared_buffer_release(SHARED_BUFFER_OWNER_OSCILLOSCOPE);
        workspace=NULL;
    }
}
scope_workspace_t *scope_workspace_get(void) {
#ifdef SCOPE_HOST_TEST
    /* 独立数值测试不打开硬件；互斥生命周期另有专门测试。 */
    return &host_workspace;
#else
    return workspace;
#endif
}
