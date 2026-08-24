#ifndef BLUETOOTH_AGENT_H
#define BLUETOOTH_AGENT_H

int bluetooth_agent_start(void);
void bluetooth_agent_process(void);
void bluetooth_agent_stop(void);
int bluetooth_agent_active(void);

#endif
