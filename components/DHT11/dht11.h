#ifndef __DHT11_H_
#define __DHT11_H_


void dht11_init(void);

int dht11_read_data(float* temperature, float* humidity);

#endif