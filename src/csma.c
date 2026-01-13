/*
    Exemplar CSMA algorithms for 0, 1 and p persistent CSMA
    N.B. all of these algorithms are blocking, they will prevent other process execution
*/

#include "RFM69.h"
#include "millis.h"
#include "xorrand.h"
#include "csma.h"
#include <stdio.h>

void csma0Send(const void* buffer, uint8_t bufferSize) {
    if(receiveDone()) {
        // There's data in the buffer, we shouldn't send
        //return;
    }
    while(1){
        if(canSend()) {
            printf("Channel clear, sending message\n");
            send(buffer, bufferSize);
            return;
        }
        else {
            //delay a random amount of time up to 1000ms
            uint32_t delay = get_rand() % 1000;
            printf("Channel occupied, waiting %lu ms\n", delay);
            uint32_t millis_current = millis();
            while((millis() - (millis_current+delay)) > 0);
                //printf("waiting...\n");
            receiveDone();
        }
    }
    
}

void csma1Send(const void* buffer, uint8_t bufferSize) {
    // Check CSMA timelimit and cansend
    uint32_t millis_current = millis();
    while (!canSend()) {
        receiveDone();
        printf("Network congested, waiting\n");
    }
    printf("Channel clear, sending message\n");
    send(buffer, bufferSize);
    return;
}


void csmaPSend(const void* buffer, uint8_t bufferSize) {
    if(receiveDone()) {
        // There's data in the buffer, we shouldn't send
        //return;
    }
    while(1) {
        if(canSend()) {
            uint32_t p_rand = (get_rand() % 100);
            printf("Can send, rand = %lu \n", p_rand);

            // If we can send get a random number, modulo to 100 and compare with probability
            if(p_rand < CSMA_P_PROBABILITY){
                // we pass the probability check, send the mesage
                printf("Probability check passed, sending \n");
                send(buffer, bufferSize);
                return;
            }
        }
        //delay a random amount of time up to 1000ms
        uint32_t delay = get_rand() % 1000 ;
        printf("Channel occupiedn or probability check failed, waiting %lu ms\n", delay);
        uint32_t millis_current = millis();
        while((millis() - (millis_current+delay)) > 0);
            //printf("waiting...\n");
        receiveDone();
    }
    
}