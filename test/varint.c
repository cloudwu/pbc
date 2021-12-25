#include <stdio.h>

#include "varint.h"
#include "pbc.h"

#define varint_encode(n, b) _pbcV_encode(n, b)
#define varint_decode(n, r) _pbcV_decode(n, r)
#define varint_zigzag(n, b) _pbcV_zigzag(n, b)
#define varint_dezigzag64(r) _pbcV_dezigzag64(r)

static void 
dump(uint8_t buffer[10], int s)
{
	int i;
	for (i=0;i<s;i++) {
		printf("%02X ",buffer[i]);
	}
	printf("\n");
}

static void
encode(uint64_t n)
{
	uint8_t buffer[10];
	dump(buffer,varint_encode(n,buffer));
}

static void
decode(uint8_t buffer[10])
{
	struct longlong r;
	int s = varint_decode(buffer,&r);
	printf("[%d] %x %x\n",s, r.low, r.hi);
}

static void
zigzag(int64_t n)
{
	printf("%x%x\n", (int32_t)(n >> 32), (int32_t)(n & 0xFFFFFFFF));
	uint8_t zigzag[10];
	dump(zigzag, varint_zigzag(n,zigzag));

	struct longlong r;
	varint_decode(zigzag,&r);
	varint_dezigzag64(&r);

	printf("%x%08x\n",r.hi, r.low);
}

int 
main()
{
	encode(300);
	uint8_t buffer[10] = { 0xac, 0x2 };
	decode(buffer);
	encode(0xfffffffffLL);
	uint8_t buffer2[10] = { 0xff, 0xff, 0xff, 0xff, 0xff, 0x1 };
	decode(buffer2);

	zigzag(-0x1234567890LL);

	return 0;
}
