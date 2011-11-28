#ifndef PROTOBUF_C_VARINT_H
#define PROTOBUF_C_VARINT_H

#include <stdint.h>

struct longlong;

int varint_encode32(uint32_t number, uint8_t buffer[10]);
int varint_encode(uint64_t number, uint8_t buffer[10]);
int varint_zigzag32(int32_t number, uint8_t buffer[10]);
int varint_zigzag(int64_t number, uint8_t buffer[10]);

int varint_decode(uint8_t buffer[10], struct longlong *result);
void varint_dezigzag64(struct longlong *r);
void varint_dezigzag32(struct longlong *r);

#endif
