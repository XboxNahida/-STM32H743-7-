#ifndef NES_PLAYER_UI_H
#define NES_PLAYER_UI_H

void nes_player_ui_init(void);
void nes_player_ui_request_open(void);
int nes_player_ui_request_open_file(const char *path);
void nes_player_ui_poll(void);

#endif
