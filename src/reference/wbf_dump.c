/*
	Eink waveform binary dumper
*/
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

/*
Type 	Initial State 	Final State 	Waveform Period
INIT 	0-F 			F 	~4000 ms 
DU 		0-F 			0 or F 	~260 ms
GC16 	0-F 			0-F 	~760 ms
GC4 	0-F 			0, 5, A, or F 	~500 ms
A2 		0 or F 			0 or F 	~120 ms
*/

// http://wenku.baidu.com/view/187d53956bec0975f465e245.html

// header structures from linux driver
struct waveform_data_header {
	uint32_t checksum; // 4 # 0x00
	uint32_t filesize; // 4 # 0x04
	uint32_t serial; // 4 # 0x08
	uint8_t run_type; // 1 # 0x0c
	uint8_t fpl_platform; // 1 # 0x0d
	uint16_t fpl_lot; // 2 # 0x0e
	uint8_t mode_version; // 1 # 0x10
	uint8_t wv_version; // 1 # 0x11
	uint8_t wv_subversion; // 1 # 0x12
	uint8_t wv_type; // 1 # 0x13
	uint8_t fpl_size; // 1 # 0x14
	uint8_t mfg_code; // 1 # 0x15
	uint8_t wv_rev; // 1 # 0x16
	uint8_t fpl_rate; // 1 # 0x17
	uint8_t reserved0; // 1 # 0x18
	uint8_t reserved1; // 1 # 0x19
	uint8_t reserved2; // 1 # 0x1a
	uint8_t reserved3; // 1 # 0x1b
	uint8_t xwia1; // 1 # 0x1c
	uint8_t xwia2; // 1 # 0x1d
	uint8_t xwia3; // 1 # 0x1e
	uint8_t checksum1; // # 0x1f
	uint32_t reserved4; // # 0x20
	uint32_t reserved5; // # 0x24
	uint32_t reserved6; // # 0x28
	uint16_t reserved7; // # 0x2c
	uint8_t reserved8; // # 0x2e
	uint8_t checksum2; // 1

};

#define WAVEFORM_MODE_INIT 0
#define WAVEFORM_MODE_DU 1
#define WAVEFORM_MODE_GC16 2
#define WAVEFORM_MODE_GC4 3
#define WAVEFORM_MODE_A2 4

uint8_t	*waveform_buffer;
uint32_t wv_modes_temps[4][15];

#define BYTE_TO_BINARY_PATTERN "%c%c%c%c%c%c%c%c"
#define BYTE_TO_BINARY(byte)  \
  (byte & 0x80 ? '1' : '0'), \
  (byte & 0x40 ? '1' : '0'), \
  (byte & 0x20 ? '1' : '0'), \
  (byte & 0x10 ? '1' : '0'), \
  (byte & 0x08 ? '1' : '0'), \
  (byte & 0x04 ? '1' : '0'), \
  (byte & 0x02 ? '1' : '0'), \
  (byte & 0x01 ? '1' : '0')

int main(int argc, char **argv) {
	fprintf(stderr, "WBF waveform dumper\n");

	if (argc < 5) {
		fprintf(stderr, "usage: wbf_dump filename mode temperature size\n");
	} else
	{
		char *fw = argv[1];
		int mode = atoi(argv[2]);
		int temp = atoi(argv[3]);
		int size = atoi(argv[4]);

		FILE * pFile;
		long lSize;
		uint8_t * buffer;
		size_t result;

		pFile = fopen ( fw , "rb" );
		if (pFile == NULL) {fputs ("File error", stderr); exit (1);}

		// obtain file size:
		fseek (pFile , 0 , SEEK_END);
		lSize = ftell (pFile);
		rewind (pFile);

		// allocate memory to contain the whole file:
		buffer = (uint8_t*) malloc (sizeof(uint8_t) * lSize);
		if (buffer == NULL) {fputs ("Memory error", stderr); exit (2);}

		// copy the file into the buffer:
		result = fread (buffer, 1, lSize, pFile);
		if (result != lSize) {fputs ("Reading error", stderr); exit (3);}

		/* the whole file is now loaded in the memory buffer. */

		int start_offset = 0;
		int i, j, k, l, m, n;
		j = 0;

		struct waveform_data_header hd;

		memcpy(&hd, (uint8_t *)(buffer) + start_offset, 48);

		fprintf(stderr, "filesize: %d\n", hd.filesize);
		fprintf(stderr, "platform: %x\n", hd.fpl_platform);
		fprintf(stderr, "size: %x\n", hd.fpl_size);
		fprintf(stderr, "checksum1 (0x1f): %x\n", hd.checksum1);
		fprintf(stderr, "reserved0 : %x\n", hd.reserved0);
		fprintf(stderr, "reserved1 : %x\n", hd.reserved1);
		fprintf(stderr, "reserved2 : %x\n", hd.reserved2);
		fprintf(stderr, "reserved3 : %x\n", hd.reserved3);
		fprintf(stderr, "reserved4 : %08x\n", hd.reserved4);
		fprintf(stderr, "reserved5 : %08x\n", hd.reserved5);
		fprintf(stderr, "reserved6 : %08x\n", hd.reserved6);
		fprintf(stderr, "reserved7 : %04x\n", hd.reserved7);
		fprintf(stderr, "reserved8 : %x\n", hd.reserved8);
		fprintf(stderr, "checksum2 (0x2f): %x\n", hd.checksum2);
		uint32_t xwia = (hd.xwia3 << 16) + (hd.xwia2 << 8) + hd.xwia1;
		int xwia_len = buffer[start_offset + xwia];
		fprintf(stderr, "xwia: %x : %x\n", xwia, xwia_len);

		fprintf(stderr, "filename (%d): ", xwia_len);
		for (i = 0; i < xwia_len; i++) {
			fprintf(stderr, "%c", buffer[start_offset + xwia + 1 + i]);
		}
		fprintf(stderr, "\n");
		k = 0;
		int hdr_size = start_offset + xwia + xwia_len;

		int trc = (hd.reserved5 >> 16) & 0xFF;
		uint32_t wv_modes[4];

		for (i = 0; i < 4; i++) {
			wv_modes[i] = buffer[hdr_size + 1 + 1 + i * 4] + (buffer[hdr_size + 1 + 1 + i * 4 + 1] << 8) + (buffer[hdr_size + 1 + 1 + i * 4 + 2] << 16);
			fprintf(stderr, "MODE %d : %04x\n", i, wv_modes[i]);
			for (j = 0; j < trc + 1; j++) {
				wv_modes_temps[i][j] = buffer[wv_modes[i] + j * 4] + (buffer[wv_modes[i] + j * 4 + 1] << 8) + (buffer[wv_modes[i] + j * 4 + 2] << 16);
				fprintf(stderr, "MODE %d temp %d : %04x\n", i, j, wv_modes_temps[i][j]);

			}
		}

		uint8_t luts[256][16][16];

		i = 0;
		int phase = 0;
		for (i = 0; i < 4; i++)
		{
			j = 0;
			int start_tag = 0;
			int tag = 0;
			int end_tag = 0;


			int x = 0;
			int y = 0;
			for (j = 0; j < trc + 1; j++)
			{

				uint16_t addr = wv_modes_temps[i][j];
				uint16_t len = 0;
				if (i == 4 && j == (trc + 1))
					len = (hd.filesize - addr - 2);
				else if (j == (trc + 1))
					len = (wv_modes_temps[i + 1][0] - addr - 2);
				else
					len = (wv_modes_temps[i][j + 1] - addr - 2);

//				fprintf(stderr, "\nMODE %d temp %d : %04x/%04x (LEN %d)\n", i, j, wv_modes_temps[i][j], wv_modes_temps[i][j + 1], len);
				m = 0;
				n = 0;
				for (k = 0; k < len; k += 2) {

					uint32_t p1, p2;
					uint8_t v1, v2;
					uint16_t c1, c2;

					/*
					0xFC is a special tag maybe to reduce waveform size
					0xFC 0xMM 0xNN ... 0xFC = 0xMM 0x00 0xNN 0x00
					*/
					if (tag == 0 && buffer[wv_modes_temps[i][j] + k] == 0xFC)
					{
						start_tag = 1;
					}

					if ( tag && (buffer[wv_modes_temps[i][j] + k] == 0xFC || buffer[wv_modes_temps[i][j] + k + 1] == 0xFC))
					{
						end_tag = 1;
						tag = 0;
					}

					c1 = 0;
					v1 = 0;
					p1 = 0;

					c2 = 0;
					v2 = 0;
					p2 = 0;

					if (tag) {
						p1 = wv_modes_temps[i][j] + k;
						v1 = buffer[p1];
						c1 = 1;
						p2 = wv_modes_temps[i][j] + k + 1;
						v2 = buffer[p2];
						c2 = 1;

					} else if (start_tag) {
						p1 = wv_modes_temps[i][j] + k + 1;
						v1 = buffer[p1];
						c1 = 1;
					} else if (end_tag) {
						if (buffer[wv_modes_temps[i][j] + k + 1] == 0xFC) {
							p1 = wv_modes_temps[i][j] + k;
							v1 = buffer[p1];
							c1 = 1;
						} else {
							k--;
						}
					}
					else
					{
						p1 = wv_modes_temps[i][j] + k;
						v1 = buffer[p1];
						c1 = buffer[p1 + 1] + 1;
					}

					if (start_tag) {
						tag = 1;
						start_tag = 0;
					}

					end_tag = 0;

					uint8_t b11, b12, b13, b14, b21, b22, b23, b24;
					b11 = (v1 >> 6) & 3;
					b12 = (v1 >> 4) & 3;
					b13 = (v1 >> 2) & 3;
					b14 = (v1) & 3;

					b21 = (v2 >> 6) & 3;
					b22 = (v2 >> 4) & 3;
					b23 = (v2 >> 2) & 3;
					b24 = (v2) & 3;

					if (mode == i && temp == j) {
						for (l = 0; l < c1; l++) {
							luts[phase][x + 3][y] = b14;
							luts[phase][x + 2][y] = b13;
							luts[phase][x + 1][y] = b12;
							luts[phase][x][y] = b11;
							fprintf(stderr, "[1 %x->%x %04x:%02x] %02d %02d %02d %02d ", 15 - x, 15 - y, p1, v1, b14, b13, b12, b11);
							if ((m + 1) % 4 == 0) {
								fprintf(stderr, "\n");
							}

							x += 5;
							if (x > 15) {
								y++;
								x = 0;
							}
							if (y > 15) {
								phase++;
							}

							if ((m + 1) % 64 == 0) {
								fprintf(stderr, " (%d)\n", n + 1);
								n++;
								x = 0;
								y = 0;
							}
							m++;
						}

						for (l = 0; l < c2; l++) {
							luts[phase][x + 3][y] = b24;
							luts[phase][x + 2][y] = b23;
							luts[phase][x + 1][y] = b22;
							luts[phase][x][y] = b21;
							fprintf(stderr, "[2 %x->%x %04x:%02x] %02d %02d %02d %02d ", 15 - x, 15 - y, p2, v2, b24, b23, b22, b21);
							if ((m + 1) % 4 == 0) {
								fprintf(stderr, "\n");
							}
							x += 5;
							if (x > 15) {
								y++;
								x = 0;
							}
							if (y > 15) {
								phase++;

							}
							if ((m + 1) % 64 == 0) {
								fprintf(stderr, " (%d)\n", n + 1);
								n++;
								x = 0;
								y = 0;
							}
							m++;
						}
					}
				}
			}
		}
		fprintf(stderr, "\n");

		fprintf(stderr, "%d phases\n", phase + 1);
		for (i = 0; i < size; i++) {
			if (i < phase + 1) {
				int p = i;
				uint8_t hex1, hex2, hex3, hex4;
				hex1 = (luts[p][0xF][0xF] << 6) + (luts[p][0xF][0xA] << 4) + (luts[p][0xF][0x5] << 2) + luts[p][0xF][0x0];
				hex2 = (luts[p][0xA][0xF] << 6) + (luts[p][0xA][0xA] << 4) + (luts[p][0xA][0x5] << 2) + luts[p][0xA][0x0];
				hex3 = (luts[p][0x5][0xF] << 6) + (luts[p][0x5][0xA] << 4) + (luts[p][0x5][0x5] << 2) + luts[p][0x5][0x0];
				hex4 = (luts[p][0x0][0xF] << 6) + (luts[p][0x0][0xA] << 4) + (luts[p][0x0][0x5] << 2) + luts[p][0x0][0x0];
				printf("%02x%02x%02x%02x\n", hex1, hex2, hex3, hex4);


				fprintf(stderr, "%02x%02x%02x%02x %c%c%c%c%c%c%c%c %c%c%c%c%c%c%c%c %c%c%c%c%c%c%c%c %c%c%c%c%c%c%c%c\n", hex1, hex2, hex3, hex4, BYTE_TO_BINARY(hex1), BYTE_TO_BINARY(hex2), BYTE_TO_BINARY(hex3), BYTE_TO_BINARY(hex4) );
			} else
			{
				printf("00000000\n");
			}
		}

		// terminate
		fclose (pFile);
		free (buffer);
	}
	return 0;
}