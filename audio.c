#include <stdio.h>

#include "audio.h"
#include "state.h" /* für "volume" */

double get_duration(Mix_Music *music)
{
    return Mix_MusicDuration(music);
}

double get_position(double base, Uint32 started, int paused)
{
    if (paused)
        return base;

    return base + (SDL_GetTicks() - started) / 1000.0;
}

Mix_Music *play_track(const Track *tracks, int index, double position,
                       double *base, Uint32 *started, int *paused)
{
    Mix_Music *music = Mix_LoadMUS(tracks[index].path);

    if (!music) {
        fprintf(stderr, "Laden fehlgeschlagen: %s\n", Mix_GetError());
        return NULL;
    }

    Mix_VolumeMusic(volume);

    if (Mix_PlayMusic(music, 1) != 0) {
        fprintf(stderr, "Wiedergabe fehlgeschlagen: %s\n", Mix_GetError());
        Mix_FreeMusic(music);
        return NULL;
    }

    if (position > 0.0)
        Mix_SetMusicPosition(position);

    *base = position;
    *started = SDL_GetTicks();
    *paused = 0;

    return music;
}


double get_track_durations(const Track *tracks, int count, double durations[])
{
    if (!tracks || !durations || count <= 0)
        return 0.0;

    double total = 0.0;
    int complete = 1;

    for (int i = 0; i < count; i++) {
        durations[i] = 0.0;
        Mix_Music *m = Mix_LoadMUS(tracks[i].path);
        if (!m) {
            complete = 0;
            continue;
        }

        double d = Mix_MusicDuration(m);
        Mix_FreeMusic(m);

        if (d > 0.0) {
            durations[i] = d;
            total += d;
        } else {
            complete = 0;
        }
    }

    return complete ? total : 0.0;
}
