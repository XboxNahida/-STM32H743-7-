#ifndef WAV_PLAYBACK_SERVICE_H
#define WAV_PLAYBACK_SERVICE_H

int wav_playback_start(const char *path);
void wav_playback_poll(void);
void wav_playback_stop(void);

#endif
