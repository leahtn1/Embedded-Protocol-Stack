// Final Version of DLL with ARQ and CSMA
// Physical Layer given from lab materials
// DLL Layer implemented with Even Parity, Fragmentation, Reassembly, Addressing for Broadcast and Unicast
// Acknowledgements and ARQ

// Still left to implement: ARQ - no timeouts or retransmissions
// CSMA implemented frm lab materials

// https://github.com/cjlee21/ELEC3227-Coursework.git

// Standard Libraries
#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>
#include "dll.h"
#include <avr/io.h>
#include <util/delay.h>

//#include the library for the RFM69 module and the UART
#include "RFM69.h"
#include "RFM69registers.h"
#include "uart.h"
#include "millis.h"
#include "xorrand.h"
#include "csma.h"

#define BROADCAST_ADDRESS   0x00
#define MY_ADDRESS          0x44

#define HEADER_VALUE 	    0xFF
#define FOOTER_VALUE 	    0x7E
#define ACK_TYPE            1
#define DATA_TYPE           0

#define EVEN_PARITY         0
#define CRC16               1

#define FRAME_MASK		    0xFFFFFFFF 		// 32 bits
#define BYTE_MASK 		    0xFF 			// 1111_1111
#define FRAG_MASK 		    0xF0 			// 1111_0000
#define SEQ_MASK 		    0x0F 			// 0000_1111
#define MF_MASK 		    0x80			// 1000_0000
#define CHECK_MASK		    0x08			// 0000_1000
#define ACKREQ_MASK		    0x04			// 0000_0100
#define TYPE_MASK		    0x03			// 0000_0011

#define MAX_FRAGMENTS       4

typedef struct ReassemblyBuffer {
    uint8_t seq;
    uint8_t fragments[MAX_FRAGMENTS][23];  // Assume maximum fragments is 16
    int active;                            // Tracks if current buffer is being used
} ReassemblyBuffer;

// Create 16 reassembly buffers used to store incoming fragmented packets
static ReassemblyBuffer reassembly_buffers[8];      // 8 arbitrary number of buffers
static uint8_t seq = 0;                             // Global sequence number
static int ack_received = 0;



// Control 1: [FRAG_4 - SEQ_4] Control 2: [MF_1 - 000 - CHECK_1 - ACKREQ_1 - TYPE_2]
uint16_t pack_control(uint8_t seq, uint8_t frag, uint8_t mf, 
	uint8_t check, uint8_t ackreq, uint8_t type) {
	
	uint8_t control_1 = 0;
	uint8_t control_2 = 0;
	
	control_1 |= (frag << 4) & FRAG_MASK;
	control_1 |= (seq & SEQ_MASK);

	control_2 |= (mf << 7) & MF_MASK;
	control_2 |= (check << 3) & CHECK_MASK;
	control_2 |= (ackreq << 2) & ACKREQ_MASK;
	control_2 |= (type & TYPE_MASK);

	return (control_1 << 8) | control_2;

}

// Testing function to simulate sending frame to PHY layer
void send_to_phy_sim(uint8_t* frame, uint8_t length) {
	int frame_length = length + 9;
	printf("Sending frame to PHY layer:\n");
	for(int i = 0; i < frame_length; i++){
		printf("%02X ", frame[i]);
	}
	printf("\n\n");

    // Loopback for testing
    printf("Loopback: Receiving frame at PHY layer\n");
    receive_from_phy(frame, length);

}

void send_to_phy(uint8_t* frame, uint8_t length) {
	int frame_length = length + 9;
	csmaPSend(frame, frame_length);
	printf("\n\n");
}

// Calculate even parity checksum
uint8_t calculate_checksum(uint8_t* data, uint8_t length) {
    uint8_t count = 0;
    for(int i = 0; i < length + 6; i++){
        uint8_t byte = data[i];
        for(int b = 0; b < 8; b++){
            if(byte & (1 << b)) count++;
        }
    }
    return count % 2;
}

// Verify checksum
uint8_t verify_checksum(uint8_t* data, uint8_t length, uint8_t received_checksum) {
    uint8_t calculated_checksum = calculate_checksum(data, length);
    return (calculated_checksum == received_checksum);
}

// Header[1] - Control[2] - Addressing[2] - Length [1] - PACKET - Checksum[2] - Footer[1]
uint8_t build_frame(uint8_t* frame, uint16_t control, uint16_t addressing, uint8_t length, 
	const uint8_t* packet, uint16_t* checksum){

	// Header
	frame[0] = HEADER_VALUE;

	// Control
	frame[1] = (control >> 8) & BYTE_MASK;
	frame[2] = control & BYTE_MASK;

	// Addressing
	frame[3] = (addressing >> 8) & BYTE_MASK;
	frame[4] = addressing & BYTE_MASK;

	// Length
	frame[5] = length & BYTE_MASK;

	// Packet
	for (int i = 0; i < length; i++) {
		frame[6 + i] = packet[i];
	}

	// Checksum
	*checksum = calculate_checksum(frame, length);
	frame[6 + length] = *checksum;
	frame[6 + length + 1] = 0x00;

	// Footer
	frame[6 + length + 2] = FOOTER_VALUE;


	return 0;
}

void send_ack(uint8_t dest, uint8_t seq) {
    uint16_t control = pack_control(seq, 0, 0, EVEN_PARITY, 0, ACK_TYPE); // control for ACK
    uint16_t addressing = (dest << 8) | MY_ADDRESS; // High byte DEST - Low byte SRC
    uint8_t frame[9];
    uint16_t checksum = 0;

    // No payload for ACK
    build_frame(frame, control, addressing, 0, NULL, &checksum); // No payload for ACK, is there checksum?

    printf("Sending ACK to %d for SEQ %d\n", dest, seq);
    //send_to_phy_sim(frame, 0);
	send_to_phy(frame, 0);

}

// DLL -> NET Confirm (confirm that the DLL is done w the packet from the NET)
void packet_confirm() {
	printf("DLL confirm: packet processed\n\n\n");
}

// NET -> DLL Request
// DLL receives packet from NET layer and sends to PHY layer
void receive_packet(uint8_t* packet, int length, uint8_t dest) {

	uint16_t checksum_choice = EVEN_PARITY;
    uint16_t mf = 1;
    uint16_t frag = 0;
    uint8_t ackreq;
    int packet_length = length;

    // Fragmentation logic
    int frame_number = ( length + 22 ) / 23;
    printf("Total frames to send: %d\n", frame_number);
    
    for (int i = 0; i < frame_number; i++) {
        frag = i;
        int offset = i * 23;                    
        packet_length = 23;
        if (i == (frame_number-1)) {
            mf = 0;
            packet_length = length - (i * 23);
        }

        if ( dest == BROADCAST_ADDRESS ) {
            ackreq = 0; // No ACK for broadcast
        } else {
            ackreq = 1; // Request ACK for unicast
        }

        uint8_t frame[(packet_length + 9)];
        // seq, frag, mf, check, ackreq, type
        uint16_t control = pack_control(seq, frag, mf, checksum_choice, ackreq, DATA_TYPE); // seq, frag, mf, check, ackreq, type
        uint16_t addressing = (dest << 8) | MY_ADDRESS; // High byte DEST - Low byte SRC

        build_frame(frame, control, addressing, packet_length, (packet + offset), &checksum_choice);

        // send to PHY
        //send_to_phy_sim(frame, packet_length);
		send_to_phy(frame, packet_length);
        
        // ACK logic
        if ( ackreq ) {
            ack_received = 0;
            printf("Waiting for ACK for SEQ %d...\n", seq);
            // Wait fixed period of time for ACK
            if ( !ack_received ) {
                // In real implementation, would have timeout and retransmission here
                // Retransmit if no ACK received
                printf("No ACK received for SEQ %d, would retransmit here.\n", seq);
            }
            printf("ACK received for SEQ %d\n", seq);
            // Increment SEQ only if ACK received
        }

        packet_confirm();

    }
    
    seq = (seq + 1) % 16;               // global seq, 4 bits

}

// PHY -> DLL Indication
// DLL receives frame from PHY layer and processes it
int receive_from_phy(uint8_t* frame, int length) {

    // Parse frame
    uint8_t control1 = frame[1];
    uint8_t control2 = frame[2];
    uint8_t seq = control1 & SEQ_MASK;
    uint8_t frag = (control1 & FRAG_MASK) >> 4;
    uint8_t mf = (control2 & MF_MASK) >> 7;
    uint8_t ackreq = (control2 & ACKREQ_MASK) >> 2;
    uint8_t type = control2 & TYPE_MASK;
    uint8_t dest = frame[3];
    uint8_t src = frame[4];

    printf("Parsed Frame: SEQ: %d, FRAG: %d, MF: %d, ACKREQ: %d, TYPE: %d\n", seq, frag, mf, ackreq, type);
    printf("Destination Address: %d\n", dest);
    printf("Source Address: %d\n", frame[4]);

    // Check if frame is for this address
    if ( dest != MY_ADDRESS && dest != BROADCAST_ADDRESS ) {
        printf("Frame not for this address (Dest: %d). Discarding.\n", dest);
        return 0;
    }

    // Frame is requesting ACK
    if ( dest == MY_ADDRESS && ackreq ) {
        // Send ACK back to source
        printf("ACK requested. Sending ACK back to source %d for SEQ %d\n", frame[4], seq);
        send_ack(src, seq);
    }

    if ( type == ACK_TYPE ) {
        // Stop timer for acknowledgement
        ack_received = 1;
        printf("ACK received for SEQ %d\n", seq);
        return 0;
    }

    // Type == DATA
    if (type == DATA_TYPE) {

        // Verify checksum
        uint8_t received_checksum = frame[6 + length];
        if(verify_checksum(frame, length, received_checksum)) {
            printf("Checksum valid.\n");
        } 
        else {
            printf("Checksum invalid!\n");
            return 0;
            // Handle error (e.g., request retransmission)
        }

        // Handle reassembly if fragmented
        // Static reassembly buffer for simplicity
        ReassemblyBuffer* reassembled_packet = &reassembly_buffers[seq];
        if ( !reassembled_packet->active ) {        // if buffer isnt active
            // Initialize new buffer for this SEQ
            reassembled_packet->seq = seq;
            reassembled_packet->active = 1;
            //memset(reassembled_packet->fragments, 0, sizeof(reassembled_packet->fragments));
        }


        if ( frag == 0 ) {
            reassembled_packet->seq = seq;
            reassembled_packet->active = 1;
        }
        memcpy(reassembled_packet->fragments[frag], &frame[6], length);  // Copy fragment data, from packet start along the length of the packet
        printf("Stored fragment %d for SEQ %d\n", frag, seq);
        if ( mf == 0 ) {
            // Last fragment received, reassemble
            printf("Reassembling packet for SEQ %d\n", seq);
            uint8_t complete_packet[MAX_FRAGMENTS * 23];
            int total_length = 0;
            for (int i = 0; i < frag; i++) {
                memcpy(&complete_packet[total_length], reassembled_packet->fragments[i], 23);
                total_length += 23;
            }
            // Last fragment length
            int last_fragment_length = length;
            memcpy(&complete_packet[total_length], reassembled_packet->fragments[frag], last_fragment_length);
            total_length += last_fragment_length;
            reassembled_packet->active = 0; // Reset buffer for future use
            
            printf("Reassembled packet length: %d bytes\n", total_length);
            
            printf("Complete Packet Data: ");
            for(int i = 0; i < total_length; i++){
                printf("%02X ", complete_packet[i]);
            }
            printf("\n");
            
            // Pass to NET layer
            // network_receive_up(complete_packet, total_length);

            return 0;
        }
    }
    return 0;
}

/*
int main(void) {
	char test[] = "Hello, World!";

    // Output length of test in bytes
    printf("Length of test: %zu bytes\n", strlen(test));
    
	receive_packet((uint8_t*)test, strlen(test), MY_ADDRESS);

	return 0;
}
	*/

    /*
int main(void)
{
	millis_init(); // Required for RFM69
	rand_init(); // Initialise the RNG
	init_debug_uart0();
	rfm69_init(433);    //init the RFM69 
	setPowerLevel(24); //set transmit power
	setChannel(5);
	printf("RFM69 DLL + PHY Start\n");

	char testing[] = "Hello World";

	while(1)
   { 
		// Get a test packet from the NET layer every second
        // Create a frame and send to PHY layer NET -> DLL -> PHY
		if((millis()%3000) == 0) {
			receive_packet((uint8_t*)testing, strlen(testing), BROADCAST_ADDRESS);
		}
		// Check for received packets
        // Receive a frame, parse and process PHY -> DLL -> NET
		if(receiveDone()) {
			receive_from_phy(DATA, sizeof(DATA));
		}
	}

}
    */