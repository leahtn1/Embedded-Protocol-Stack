//net.h
//CALL init_net_layer() AT THE START OF THE PROGRAM
//millis.c is also modified to do some periodic checks so include the new millis.c

#ifndef NET_H
#define NET_H

#include <stdint.h>


#define DEVICE_ID 0b00000001
#define OVERHEAD_BYTES 7
#define MAX_NEIGHBOURS 100
#define BROADCAST_PERIOD_MS 3000
#define CUT_OFF_TIME_MS 3 * BROADCAST_PERIOD_MS

//crc definitions
#include <string.h>
#define generator 0x1021
#define INITIAL_CRC 0xffff
#define MAX_NET_PACKET_WIDTH 83

struct Device{
    uint8_t ID;
    uint8_t hops;
    unsigned long last_updated;
};

extern struct Device neighbourhood[MAX_NEIGHBOURS];

extern uint8_t RoutingTable[MAX_NEIGHBOURS];          //next node for each destination

//crc helper functions:
int count_bits(unsigned long n);                                        //returns number of bits to store an int value
//void int_to_bin_str(unsigned long long num, char *str, int bits);       //converts integer to binary string
void byte_array_to_bin_str(uint8_t *data, int len, char *str);
uint16_t run_crc2(uint8_t * received, int len);


//NET LAYER FUNCTIONS
void init_net_layer();

//send and receive functions. send called by transport, receive called by dll  
int network_send_down(uint8_t* message, int len, uint8_t dest);
int network_receive_up(uint8_t * received, int len);

//helper functions
int check_crc(uint8_t * received, int len);        //returns 1 if valid, 0 if failure
void strip_overhead(uint8_t * received, int len);   //removes net overhead for passing data up
int lifetime_overflow(uint8_t control2);            //returns 1 if lifetime counter = MAX

//routing functions
void update_routing_table();
void send_neighbourhood_packet();
void unpack_neighbourhood_info(uint8_t * received, int len);

//print functions
void print_routing_table();
void print_neighbourhood();
void print_net_packet(uint8_t * packet, int len);
void print_active_routes();
void print_neighbourhood_active();



#endif