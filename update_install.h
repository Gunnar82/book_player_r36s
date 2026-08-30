#ifndef UPDATE_INSTALL_H
#define UPDATE_INSTALL_H

#include "update_check.h"

#define UPDATE_STAGE_PATH "/tmp/hoerspiel_player.new"
#define UPDATE_INSTALL_STATUS_LEN 192

int update_download_and_verify(const UpdateManifest *manifest,
                               const char *target_path,
                               char *status,
                               int status_size);

int update_install_staged(const char *staged_path,
                          char *status,
                          int status_size);

#endif
