#ifndef FILE_OPEN_ROUTER_H
#define FILE_OPEN_ROUTER_H

void file_open_router_init(void);
int file_open_router_request(const char *path);
void file_open_router_poll(void);

#endif
