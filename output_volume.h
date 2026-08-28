#ifndef OUTPUT_VOLUME_H
#define OUTPUT_VOLUME_H

int output_volume_get(int *percent);
int output_volume_set(int percent);
int output_volume_change(int delta, int *percent);

#endif
