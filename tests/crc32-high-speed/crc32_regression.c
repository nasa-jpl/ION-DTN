/* Regression for the slice-by-16 remainder pointer, including incremental CRCs. */
#include <crc.h>
#include <stdio.h>

#ifdef ENABLE_HIGH_SPEED

typedef uint32_t (*CrcFunction)(const char *, uint32_t, uint32_t);

static uint32_t oracle(const unsigned char *data, size_t length, uint32_t poly)
{
	uint32_t crc = 0xffffffffU;
	for (size_t i = 0; i < length; i++)
	{
		crc ^= data[i];
		for (int bit = 0; bit < 8; bit++)
			crc = (crc >> 1) ^ ((crc & 1) ? poly : 0);
	}
	return ~crc;
}

int main(void)
{
	CrcFunction functions[] = {ion_CRC32_1EDC6F41_C_slice, ion_CRC32_04C11DB7_slice};
	uint32_t polynomials[] = {0x82f63b78U, 0xedb88320U};
	uint32_t known[] = {0xe3069283U, 0xcbf43926U};
	unsigned char data[1028];
	unsigned int checks = 0;
	for (size_t i = 0; i < sizeof(data); i++) data[i] = (unsigned char)(i * 37 + i / 7);
	for (int variant = 0; variant < 2; variant++)
	{
		if (functions[variant]("123456789", 9, 0) != known[variant]) return 1;
		for (unsigned int offset = 0; offset < 4; offset++)
		{
			for (unsigned int length = 0; length <= 1024; length++)
			{
				const char *input = (const char *)data + offset;
				uint32_t expected = oracle(data + offset, length, polynomials[variant]);
				uint32_t whole = functions[variant](input, length, 0);
				unsigned int split = length / 2;
				uint32_t partial = functions[variant](input, split, 0);
				partial = functions[variant](input + split, length - split, partial);
				if (whole != expected || partial != expected)
				{
					fprintf(stderr, "CRC variant %d, offset %u, length %u: expected %08x, whole %08x, incremental %08x\n",
							variant, offset, length, expected, whole, partial);
					return 1;
				}
				checks += 2;
			}
		}
	}
	printf("PASS: CRC32C and CRC32, lengths 0..1024, offsets 0..3, whole and incremental inputs (%u checks plus known vectors).\n", checks);
	return 0;
}
#else
int main(void)
{
	puts("SKIP: CRC slice regression requires --enable-high-speed.");
	return 2;
}
#endif
