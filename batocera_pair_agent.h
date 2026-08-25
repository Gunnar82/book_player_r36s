#ifndef BATOCERA_PAIR_AGENT_H
#define BATOCERA_PAIR_AGENT_H

#include <stdint.h>

typedef enum {
    BATOCERA_PAIR_IDLE=0,
    BATOCERA_PAIR_WORKING,
    BATOCERA_PAIR_CONFIRM,
    BATOCERA_PAIR_SUCCESS,
    BATOCERA_PAIR_FAILED
} BatoceraPairState;

/* Blocking pairing operation. Intended to run in the existing SDL pairing
   worker thread. The UI remains responsive and can answer confirmation
   requests through batocera_pair_agent_respond(). */
int batocera_pair_agent_pair(const char *mac);
BatoceraPairState batocera_pair_agent_state(void);
uint32_t batocera_pair_agent_passkey(void);
void batocera_pair_agent_respond(int accept);
void batocera_pair_agent_cancel(void);

#endif
