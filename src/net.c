//net.c
#include <stdio.h>
#include <stdlib.h>
#include "net.h"
#include "millis.h"
//#include "DLL.h"

struct Device neighbourhood[MAX_NEIGHBOURS] = {0};
uint8_t RoutingTable[MAX_NEIGHBOURS] = {0};          

//CRC Functions

int count_bits(unsigned long n){
    int bits = 0;

    while (n > 0){
        bits++;
        n>>=1;
    }
    return bits;
}

/*
void int_to_bin_str(unsigned long long num, char *str, int bits) {

    for (int i = bits - 1; i >= 0; i--) {
        //printf("\nCount: %d", i);
        if (num & ((unsigned long long)1 << i)){
            str[bits - 1 - i] = '1';
        }else{
            str[bits - 1 - i] = '0';

        }
        
        //printf("\nconverted %c", str[bits-1-i]);
    }
    
    str[bits] = '\0'; // Null-terminate
}
*/

uint16_t run_crc2(uint8_t * received, int len){
    //char stringcrc[20] = {'\0'};
    uint16_t crc = INITIAL_CRC;

    //int_to_bin_str((unsigned long long)crc, stringcrc, 16);
    //printf("\nCRC: %s", stringcrc);

    for (int i = 0; i < len; i++){      //loop through byte array
        //printf("\nOuter loop %d", i);
        for (int j = 7; j >= 0 ; j--){    //loop through bits in each byte

            //printf("\n     Inner loop %d", j);
            uint8_t bit = (received[i]>> j) & 1;      //current bit in the message
            uint8_t msb = (crc >> 15) & 1;      //msb of the crc
            crc <<= 1;                           // shift CRC left
            crc &= 0xFFFF;                       // keep it 16-bit
            crc |= bit;                         // shift in the message bit

            if (msb) {                     // if MSB =1, xor with generator
                crc ^= generator;               
            }
            
            //int_to_bin_str((unsigned long long)crc, stringcrc, 16);
            //printf("\n%3d  CRC: %s", j, stringcrc);
        }
    }
    
    //int_to_bin_str((unsigned long long)crc, stringcrc, 16);
    //printf("\nBEFORE FLUSH  CRC: %s", stringcrc);
    //flush zeros equal to crc width
    for (int i = 0; i < 16; i++){
            uint8_t msb = (crc >> 15) & 1;      //msb of the crc
            crc <<= 1;                           // shift CRC left
            crc &= 0xFFFF;                       // keep it 16-bit
            crc |= 0;                         // shift in the message bit ==0

            if (msb) {                     // if MSB =1, xor with generator
                crc ^= generator;               
            }
    }
    
    //int_to_bin_str((unsigned long long)crc, stringcrc, 16);
    //printf("\nAFTER FLUSH  CRC: %s", stringcrc);
    //printf("\nRETURNING %d", crc);
    return crc;
    
}

//NET LAYER Functions
void init_net_layer(){
    //initialise all devices to unreachable
    for (int i = 0; i < MAX_NEIGHBOURS; i++) {
        neighbourhood[i] = (struct Device){ .ID = i, .hops = 255, .last_updated = 0 };
    }

    //initialise all routes to 0 (no route)
    for (int i = 0; i < MAX_NEIGHBOURS; i++){
        RoutingTable[i] = 0;
    }
}

//evaluation and data analysis functions
int check_crc(uint8_t * received, int len){

    uint16_t crc_received = (received[len-2] << 8)|received[len-1];
    if (run_crc2(received, len-2) == crc_received){
        return 1; //success
    }else{
        return 0;   //failure
    }
}

void strip_overhead(uint8_t * x, int len){
    //strip 7 bits of net overhead
    for (int i = 0; i < len; i++){
        if(i < (len - 6)){
            x[i] = x[i + 5]; 
        }
    }
}

int lifetime_overflow(uint8_t  control2){
    uint8_t temp = control2 & 0b01111111;

    if (temp >= 0b01111111){
        return 1;       //overflow. discard
    }else{
        return 0;
    }
}

void unpack_neighbourhood_info(uint8_t * received, int len){
    //printf("\nUnpacking neighbourhood packet");
    //print_routing_table();
    //print_active_routes();

    //hops to reach the device that sent this packet
    uint8_t base = neighbourhood[received[0]].hops;
    uint8_t newhops = received[3]+base;             //hops to the first device in the neighbourhood packet
    //printf("\nOld hops to %d : %d", received[2], neighbourhood[received[2]].hops);
    //printf("\nNew hops to %d : %d", received[2], newhops);

    //if the path through this node is shorter, ammend
    if (newhops < neighbourhood[received[2]].hops){
        //printf("\n%3d < %3d. To reach device %3d, pass to %3d", newhops, neighbourhood[received[2]].hops, neighbourhood[received[2]].ID, neighbourhood[received[0]].ID );
        neighbourhood[received[2]].hops = newhops;
        RoutingTable[received[2]] = received[0];
        neighbourhood[received[2]].last_updated = millis();
        
    }

    //-2 to exclude checksum, start at 5 to ignore control bits + whats already been unpacked above
    for(int i = 5; i < len-2; i+=2){     
        newhops = received[i + 1] + base;
        if( newhops < neighbourhood[received[i]].hops){
            //printf("\n%d < %d. To reach device %d, pass to %d", newhops, neighbourhood[received[i]].hops, neighbourhood[received[i]].ID, neighbourhood[received[0]].ID );
            neighbourhood[received[i]].hops = newhops;
            RoutingTable[received[i]] = received[0];
            neighbourhood[received[i]].last_updated = millis();
            
        }
    }

    //print_routing_table();
    print_active_routes();
}

//display functions
void print_routing_table() {
    printf("\nCurrent routing table:");

    int rows = 20;   // 100 entries / 5 columns = 20 rows
    int cols = 5;
    printf("\n ID  Path     ID  Path     ID  Path     ID  Path     ID  Path\n");
    printf("__________________________________________________________________\n");
    for (int r = 0; r < rows; r++) {
        for (int c = 0; c < cols; c++) {
            int i = r + c * rows;   // vertical indexing

            //printf("%3d : %-5d  ", i, RoutingTable[i]);
            //highlight non zero rows
            
            if(RoutingTable[i] !=0){
                printf("\033[30;47m%3d:%-5d \033[0m ", i, RoutingTable[i]);
            }else{
                printf("%3d:%-5d  ", i, RoutingTable[i]);
            }
            
        }
        printf("\n");
    }
}

void print_neighbourhood() {
    printf("\n  ID   Hops       t      ID   Hops       t       ID   Hops       t       ID   Hops       t       ID   Hops       t\n");
    printf("__________________________________________________________________________________________________________\n");

    int rows = 20;  // because 100 entries / 5 columns
    int cols = 5;

    for (int r = 0; r < rows; r++) {
        for (int c = 0; c < cols; c++) {

            int i = r + c * rows;   // vertical index mapping

            printf("%3d : %3d : %6lu      ",
                   i,
                   neighbourhood[i].hops,
                   neighbourhood[i].last_updated);
        }
        printf("\n");
    }
}

void print_net_packet(uint8_t * packet, int len){
    printf("\nControl 0 (hex): %5x", packet[0]);
    printf("\nControl 1 (hex): %5x", packet[1]);
    printf("\nSource address (dec): %5d", packet[2]);
    printf("\nDestination address (dec): %5d", packet[3]);
    printf("\nLength (dec): %5d", packet[4]);
    printf("\nCRC (hex): %5x", ((uint16_t)packet[len-2]<<8)|(packet[len-1]));
}

//send and receive
int network_receive_up(uint8_t * received, int len){
    //printf("\nReceived packet");
    //check crc
    if (!check_crc(received, len)){
        printf("\nCRC: Failed. Packet discarded");
        return 1;       //failure
    }else{
        printf("\nCRC check passed.");
    }

    //add device to neighbourhood
    //number of hops for device that sent this packet = 1
    neighbourhood[received[0]] = (struct Device){ received[0], 1, millis() };
    RoutingTable[received[0]] = received[0];
    //printf("\nAdding device %4d to neighbourhood.", received[0]);
    
    //check if data is valid
    if (lifetime_overflow(received[1])){
        //printf("\nout of date packet. Discarded.");
        return 1;       //out of date packet. discard
    }else{
        //printf("\nLifetime counter still valid ");
    }

    //create local copy
    uint8_t buffer[len];   
    memcpy(buffer, received, len);
    
    //check neighbourhood flag in control byte 2
    if ( (received[1] >> 7) & 1){
        //printf("\nType: Neighbourhood");
        unpack_neighbourhood_info(received, len);
        return 0;       //success
    }else{
        //printf("\nType: DATA");
        
        //if destination == me, pass to tran, else reroute
        if(buffer[3] == DEVICE_ID){
            printf("\nThis is the final destination. Stripping overhead and passing to TRAN");
            strip_overhead(buffer, len);
            //PASS BUFFER UP TO TRANSPORT
            //Transport_receive(buffer, len);
            return 0;

        }else{
            //printf("\nThis packet needs to be routed to device %d", buffer[3]);
            //modify packet for rerouting
            buffer[0] = DEVICE_ID;

            buffer[1] = (buffer[1] & 0x80) | ((buffer[1] + 1) & 0x7f);  //lifetime+1 while preserving msb
            
            //calculate next hop
            uint8_t next_hop = RoutingTable[buffer[3]]; 

            if (next_hop == 0){
                printf("\nNo valid route. Packet will be dropped");

            }else{
                printf("\nPacket forwarded to device %d", next_hop);
                //reset crc
                buffer[len -1] = 0;
                buffer[len -2] = 0;
                uint16_t crc = run_crc2(buffer, len-2);     //calc crc excluding last 2 bytes
                buffer[len-2] = (crc >> 8) & 0xFF;   // high byte
                buffer[len-1] = crc & 0xFF;          // low byte

                //print_net_packet(buffer, len);
                //Pass to DLL
                //receive_packet(buffer, len, next_hop);
            }
 

            return 0;
        }
    }
}

int network_send_down(uint8_t * message, int len, uint8_t dest){
    printf("\nTransport request transmission to node %d", dest);

    //create local copy
    int packet_length = len + 2 + 5;            //= tran_seg + crc + header
    uint8_t buffer[packet_length];   
    memcpy(buffer + 5, message, len);        
    buffer[0] = DEVICE_ID;
    buffer[1] = 0b00000001;     //data flag = 0, lifetime = 1;     
    buffer[2] = DEVICE_ID;      //src address
    buffer[3] = dest;           //dest address
    buffer[4] = packet_length;    //packet length

    uint16_t crc = run_crc2(buffer, packet_length -2);      //-2 to exclude check bits
    buffer[packet_length - 2] = (crc >> 8) & 0xFF;   // high byte
    buffer[packet_length - 1] = crc & 0xFF;          // low byte

    //determine next hop
    uint8_t next_hop = RoutingTable[buffer[3]]; 
    if (next_hop == 0){
        printf("\nNo valid route. Packet will be broadcasted");
    }else{
        printf("\nPacket will be forwarded to device %d", next_hop);
    }
    
    //pass buffer to DLL LAYER
    //print_net_packet(buffer, packet_length);
    //receive_packet(buffer, len, next_hop);

    return 0;
}

void update_routing_table(){
    //printf("\nCleaning up the routing table:");

    unsigned long time_now = millis();          
    uint8_t route = 0;
    //loop through the saved node and check if each route was last updated within the TIMEOUT
    for (int i = 0; i < MAX_NEIGHBOURS; i++){
        if(RoutingTable[i] != 0){
            route = RoutingTable[i];
            if ( (time_now - neighbourhood[route].last_updated) > CUT_OFF_TIME_MS){
                //printf("\n offline node: %3d", route);
                //printf("     Affected nodes: %3d,", i);
                
                //node is now unreachable. no route
                RoutingTable[i] = 0;

                neighbourhood[route] = (struct Device){route, 255, time_now};

                neighbourhood[i] = (struct Device){i, 255, time_now};
                /*
                printf("\nReset device %d. Hops: %d", i, neighbourhood[i].hops);
                printf("\nRoute to %d update from %d to %d", i, RoutingTable[i], 0);
                printf("\nReset device %d. Hops: %d", route, neighbourhood[route].hops);
                */

                
                //apply for all routes that use this node
                for(int j = 0; j <MAX_NEIGHBOURS; j++){
                    
                   // printf("\n*****************comparing route: %d and routingtable[%d]= %d", route, j, RoutingTable[j]);
                    if(RoutingTable[j] ==  route){
                        RoutingTable[j] = 0;
                        neighbourhood[j] = (struct Device){j, 255, time_now};
                        
                        //printf(" %d, ", j);
                        
                    }

                }
                
            }
        }
    }
}

void send_neighbourhood_packet(){
    update_routing_table();
    //printf("\nAfter cleanup:");
    print_active_routes();
    //print_routing_table();
    int counter = 0;
    uint8_t tosend[MAX_NEIGHBOURS][2] = {0};

    //count how many routes I have
    for(int i = 0; i < MAX_NEIGHBOURS; i++){
        if(RoutingTable[i] != 0){
            tosend[counter][0] = i;   //address
            tosend[counter][1] = neighbourhood[i].hops;
            
            counter++;
            //printf("\nAddress: %d    Hops: %d", tosend[counter][0], tosend[counter][1]);
        }
    }

    //printf("\nNeighbour count: %d", counter);
    int packet_length = OVERHEAD_BYTES; 

    int extra = counter*2 - 2;      //tran bytes needed to store neighbour + hops
    if(extra > 0){
        packet_length = OVERHEAD_BYTES + extra;  //overhead + tran bytes
    }

    
    //printf("\n\nNeighbourhood info to broadcast:");

    for (int q = 0; q < MAX_NEIGHBOURS; q++){
        if (tosend[q][0] !=0){
            printf("\n%3d     %3d", tosend[q][0], tosend[q][1]);
        }
    }
    
    uint8_t buffer[packet_length];     
    memset(buffer, 0, packet_length);      

    //construct neighbourhood packet
    buffer[0] = DEVICE_ID;
    buffer[1] = 0b10000001;             //lifetime counter + control flag
    buffer[4] = packet_length;          //length

    //fill in src and dest address w the first neighbour
    if(counter > 0){
        buffer[2] = tosend[0][0];
        buffer[3] = tosend[0][1];
    }

    //fill in tran bytes with remaining neighbours
    for(int i = 5; i < extra + 5; i+=2){
        buffer[i] = tosend[i-4][0];
        buffer[i+1] = tosend[i-4][1];

    }

    //calculate crc
    uint16_t crc = run_crc2(buffer, packet_length - 2);
    buffer[packet_length - 2] = (crc >> 8) & 0xFF;   // high byte
    buffer[packet_length - 1] = crc & 0xFF;          // low byte
    
    //PASS TO DLL
    //printf("\nSending neighbourhood packet to DLL");
    //print_net_packet(buffer, packet_length);
    
    //Dll_SEND(buffer, packet_length, 0);       dest = 0 for broadcast

}

void print_active_routes(){
    //printf("\nCurrent Available Routes: ");
    printf("\n\nTarget  Path   \n");
    printf("____________\n");

    for(int i = 0; i < MAX_NEIGHBOURS; i++){
        if(RoutingTable[i] != 0){
            printf("%3d    : %3d\n", i, RoutingTable[i] );
        }
    }

}

void print_neighbourhood_active(){

}

