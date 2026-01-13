/*
    Exemplar CSMA algorithms for 0, 1 and p persistent CSMA
    N.B. all of these algorithms are blocking, they will prevent other process execution
*/

#pragma once

#include <stdint.h>

#define CSMA_P_PROBABILITY 80 // 80/100 chance of success in transmissions



void csma0Send(const void* buffer, uint8_t bufferSize);

void csma1Send(const void* buffer, uint8_t bufferSize);

void csmaPSend(const void* buffer, uint8_t bufferSize);