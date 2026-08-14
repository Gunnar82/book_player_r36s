#ifndef SCANNER_H
#define SCANNER_H

#include "types.h"

/* Prueft anhand der Dateiendung, ob es sich um eine unterstuetzte
   Audiodatei handelt (.mp3, .ogg, .wav, .flac). */
int is_audio(const char *name);

/* True, wenn direkt in dirpath mindestens eine Audiodatei liegt. */
int directory_has_audio(const char *dirpath);

/* True, wenn direkt in dirpath mindestens ein Unterordner liegt. */
int directory_has_subdirectories(const char *dirpath);

/* Listet die direkten Unterordner in base alphabetisch sortiert. */
int scan_books(const char *base, char names[][256], char paths[][512]);

/* Findet rekursiv alle Ordner, die DIREKT Audiodateien enthalten.
   Damit bleiben Resume/Fortschritt auch bei Autor-/Kategorieordnern erhalten. */
int scan_books_recursive(const char *base, char names[][256], char paths[][512]);

/* Listet alle Audiodateien (=Tracks) in dirpath alphabetisch sortiert. */
int scan_tracks(const char *dirpath, Track tracks[]);

#endif /* SCANNER_H */
