#include <avr/io.h>
#include <stdlib.h>
#include <util/delay.h>

#include <string.h>
#include <stdint.h>

//#include the library for the RFM69 module and the UART
#include "RFM69.h"
#include "RFM69registers.h"
#include "uart.h"
#include "millis.h"
#include "xorrand.h"

#include "net.h"
#include <util/delay.h>


void test_binary_convertor();
void test_crc2();
void test_neighbourhood_unpacking();
void setup();
void demo();
void pythondemo();


int main(void)
{
	setup();
	char testing[] = "Test string";
	
    //getchar();

	//test_neighbourhood_unpacking();
	demo();
    getchar();
	while(1){
        
	}

}
	
void demo(){
	printf("\n***Time independent execution***");
    printf("\n_______________________________________________________________________________________________\n\n");

	getchar();
	//receive false crc 
    uint8_t packet[6] = {0b00000011, 0b10000010, 0b00001000, 0b00000110, 0x5f, 0x2d};
    network_receive_up(packet, 6);
    printf("\n_______________________________________________________________________________________________\n\n");
    getchar();

	//receive neighbourhood packet
    uint8_t packet1[6] = {0b00000010, 0b10000010, 0b00001000, 0b00000110, 0x5f, 0x2d};
    network_receive_up(packet1, 6);
    printf("\n_______________________________________________________________________________________________\n\n");
    getchar();
	
	//receive neighbourhood packet
    uint8_t packet2[6] = {4, 0b10011100, 0b0110, 0b1111, 0x92, 0xf0};
    network_receive_up(packet2, 6);
    printf("\n_______________________________________________________________________________________________\n\n");
    getchar();

	//receive packet that needs to be routed
    uint8_t packet3[11] = {0b11100, 0b1110010, 0b1101, 0b110, 0b1011, 0x0f, 0xf0, 0xcc, 0x33, 0xe4, 0xea};
    network_receive_up(packet3, 11);
    printf("\n_______________________________________________________________________________________________\n\n");
    getchar();

	//receive a neighbourhood packet
    uint8_t packet4[9] = {0x10, 0xab, 0x08, 0x03, 0x09, 0x0e, 0x01, 0x39, 0xcd};
    network_receive_up(packet4, 9); 
    printf("\n_______________________________________________________________________________________________\n\n");
    getchar();
	
	
	//transmit to device 14
	uint8_t packet6[11] = {0xa1, 0xb2, 0xc3, 0xd4, 0xe5, 0xf6, 0xe7, 0xd8, 0xc9, 0x10, 0x11};
	network_send_down(packet6, 11, 14);
    printf("\n_______________________________________________________________________________________________\n\n");
    getchar();	//exp crc = a086

	//transmit to 28
	uint8_t packet7[20] = {0x3A, 0xF1, 0x7C, 0x92, 0x0B,0xD4, 0xE8, 0x56, 0x11, 0xAF, 0x33, 0xC2, 0x79, 0x04, 0xBE, 0x8D, 0x65, 0xFA, 0x20, 0x48};
	network_send_down(packet7, 20, 28);
    printf("\n_______________________________________________________________________________________________\n\n");
    getchar();	//exp crc = e349

	
	//cant route a packet. broadcast
	uint8_t packet8[20] = {0x3A, 0xF1, 0x7C, 0x92, 0x0B,0xD4, 0xE8, 0x56, 0x11, 0xAF, 0x33, 0xC2, 0x79, 0x04, 0xBE, 0x8D, 0x65, 0xFA, 0x20, 0x48};
	network_send_down(packet8, 20, 33);
    printf("\n_______________________________________________________________________________________________\n\n");
    getchar();
    
	printf("\nCleanup...");
    getchar();
	send_neighbourhood_packet();
	//print_neighbourhood();
    printf("\n_______________________________________________________________________________________________\n\n");
    getchar();

	//receive a neighbourhood packet
    uint8_t packet9[9] = {0x10, 0xab, 0x08, 0x03, 0x09, 0x0e, 0x01, 0x39, 0xcd};
    network_receive_up(packet9, 9); 
    printf("\n_______________________________________________________________________________________________\n\n");
    getchar();

	//transmit to device 14
	uint8_t packet10[11] = {0xa1, 0xb2, 0xc3, 0xd4, 0xe5, 0xf6, 0xe7, 0xd8, 0xc9, 0x10, 0x11};
	network_send_down(packet10, 11, 14);
    printf("\n_______________________________________________________________________________________________\n\n");
    getchar();	//exp crc = a086

	//transmit to 28
	uint8_t packet11[20] = {0x3A, 0xF1, 0x7C, 0x92, 0x0B,0xD4, 0xE8, 0x56, 0x11, 0xAF, 0x33, 0xC2, 0x79, 0x04, 0xBE, 0x8D, 0x65, 0xFA, 0x20, 0x48};
	network_send_down(packet11, 20, 28);
    printf("\n_______________________________________________________________________________________________\n\n");
    getchar();	//exp crc = e349

    //cleanup
}



void setup(){
	millis_init(); // Required for RFM69
	rand_init(); // Initialise the RNG
	init_debug_uart0();
	rfm69_init(433);    //init the RFM69 
	setPowerLevel(24); //set transmit power
	setChannel(0);

	init_net_layer();
	printf("Setup done\n");  
}

/*
void test_neighbourhood_unpacking(){
	
    uint8_t packet[6] = {0b00000010, 0b10000010, 0b00001000, 0b00000110, 0x5f, 0x2d};
    int dummy = network_receive_up(packet, 6);
    print_neighbourhood();
    printf("\n_______________________________________________________________________________________________");
    getchar();

    uint8_t packet2[6] = {4, 0b10011100, 0b0110, 0b1111, 0x92, 0xf0};
    dummy = network_receive_up(packet2, 6);
    print_neighbourhood();
    printf("\n___________________________________________________________");
    getchar();
    
    uint8_t packet3[11] = {0b11100, 0b1110010, 0b1101, 0b110, 0b1011, 0x0f, 0xf0, 0xcc, 0x33, 0xe4, 0xea};
    dummy = network_receive_up(packet3, 11);
    print_neighbourhood();
    printf("\n___________________________________________________________");
    getchar();

    uint8_t packet4[9] = {0x10, 0xab, 0x08, 0x03, 0x09, 0x0e, 0x01, 0x39, 0xcd};
    dummy = network_receive_up(packet4, 9);
    print_neighbourhood();

    send_neighbourhood_packet();

	printf("\ndummy %d", dummy);
    //update_routing_table();
}
*/

//performance tests

/*
Comented out for code optimisation
Global counters were embedded in the net.h but also removed for storage optimisation once results were obtained
Replaced with dummy scope level counters so it runs

void functional_correctness_test(){
    //repeated send requests. count how many times dll is invoked
    int counter = 0;
    int N = 1000;           //number of tests
    int results[N][2];    //table of expected dll invocations vs actual dll invocations

    for (int i = 0; i < N; i++) {
        results[i][0] = 0;
        results[i][1] = 0;
    }

    for (int i = 0; i < N; i = i + 10){
        results[i][0] = N;      //number of tries
        for (int j = 0; j < N/5; j = j + 10){
            //transmit to device 14
	        uint8_t packet6[11] = {0xa1, 0xb2, 0xc3, 0xd4, 0xe5, 0xf6, 0xe7, 0xd8, 0xc9, 0x10, 0x11};
	        network_send_down(packet6, 11, 14);
            counter++;
        
        }
        results[i][1] = counter;
        counter = 0;
    }

    printf("\nExpected    Actual");
    for (int i = 0; i < N; i++){
        printf("\n%4d  %4d", results[N][0], results[N][1]);
    }
}



*/