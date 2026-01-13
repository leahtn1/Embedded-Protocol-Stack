#include <stdint.h>

int receive_from_phy(uint8_t* frame, int length);
int receive_from_phy(uint8_t* frame, int length);
void receive_packet(uint8_t* packet, int length, uint8_t dest);
uint16_t pack_control(uint8_t seq, uint8_t frag, uint8_t mf, 
	uint8_t check, uint8_t ackreq, uint8_t type);

void send_ack(uint8_t dest, uint8_t seq);
void send_to_phy(uint8_t* frame, uint8_t length);
uint8_t calculate_checksum(uint8_t* data, uint8_t length);
uint8_t verify_checksum(uint8_t* data, uint8_t length, uint8_t received_checksum);
uint8_t build_frame(uint8_t* frame, uint16_t control, uint16_t addressing, uint8_t length, 
    const uint8_t* packet, uint16_t* checksum);
void packet_confirm();
void receive_packet(uint8_t* packet, int length, uint8_t dest);
int receive_from_phy(uint8_t* frame, int length);