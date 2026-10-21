#include "oscilloscope_workspace.h"
#include "shared_buffer.h"
#include "sdram_layout.h"
#include <assert.h>
#include <stdio.h>
int main(void) {
    scope_workspace_t *w; void *other;
    assert(sizeof(scope_workspace_t)<=SDRAM_SHARED_WORK_SIZE);
    assert(scope_workspace_acquire()==1);
    w=scope_workspace_get(); assert(w!=NULL);
    w->raw[8191]=12345; w->stream[16383]=54321;
    assert(shared_buffer_acquire(SHARED_BUFFER_OWNER_NOTES,16,&other)==SHARED_BUFFER_BUSY);
    scope_workspace_release();
    assert(shared_buffer_get_owner()==SHARED_BUFFER_OWNER_NONE);
    assert(shared_buffer_acquire(SHARED_BUFFER_OWNER_NOTES,16,&other)==SHARED_BUFFER_OK);
    assert(scope_workspace_acquire()==0);
    assert(shared_buffer_release(SHARED_BUFFER_OWNER_NOTES)==SHARED_BUFFER_OK);
    assert(scope_workspace_acquire()==1); scope_workspace_release();
    printf("PASS oscilloscope_workspace bytes=%lu\n",(unsigned long)sizeof(*w));
    return 0;
}
