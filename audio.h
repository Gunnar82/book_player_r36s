#ifndef AUDIO_H
#define AUDIO_H

#include <SDL2/SDL.h>
#include <SDL2/SDL_mixer.h>

#include "types.h"

/* Gesamtlänge des Tracks in Sekunden, <=0 falls unbekannt. */
double get_duration(Mix_Music *music);

/* Aktuelle Wiedergabeposition in Sekunden aus Basiswert + verstrichener
   Echtzeit berechnet (bzw. Basiswert direkt, falls pausiert). */
double get_position(double base, Uint32 started, int paused);

/* Lädt und startet den Track an Index "index", ggf. ab "position".
   Setzt base/started/paused entsprechend. NULL bei Fehler. */
Mix_Music *play_track(const Track *tracks, int index, double position,
                       double *base, Uint32 *started, int *paused);

/* Ermittelt die Dauer aller Tracks einmalig.
   durations muss Platz fuer count Eintraege haben.
   Rueckgabe: Gesamtdauer in Sekunden, <=0 falls mindestens eine Dauer
   nicht ermittelt werden konnte. */
double get_track_durations(const Track *tracks, int count, double durations[]);

#endif /* AUDIO_H */
