#ifndef TYPES_H
#define TYPES_H

typedef struct { char path[512]; char name[256]; } Track;
typedef struct { char path[512]; char name[256]; int track; double position; long long last_played; unsigned int dial_id; } BookProgress;

#endif
