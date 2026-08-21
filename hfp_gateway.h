#ifndef HFP_GATEWAY_H
#define HFP_GATEWAY_H

#include <stddef.h>

typedef struct HfpGateway HfpGateway;

/*
 * HFP dial IPC receiver.
 * PulseAudio remains the HFP Audio Gateway and forwards ATD... commands to
 * this player through a local Unix datagram socket.
 */
int hfp_gateway_init(HfpGateway **out_gateway);
void hfp_gateway_process(HfpGateway *gateway);
int hfp_gateway_poll_dial(HfpGateway *gateway,char *number,size_t number_size);
const char *hfp_gateway_socket_path(const HfpGateway *gateway);
void hfp_gateway_close(HfpGateway *gateway);

#endif
