
#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <errno.h>
#include <string.h>
#include <unistd.h>
#include <ctype.h>
#include <sys/stat.h>
#include <arpa/inet.h>

// there probably aren't any displays with more waveforms than this (we hope)
// (technically the header allows for 256 * 256 waveforms but that's not realistic)
#define MAX_WAVEFORMS (4096)

// these are the actual maximums
#define MAX_MODES (256)
#define MAX_TEMP_RANGES (256)

// for unknown reasons addresses in the .wrf file 
// need to be offset by 63 bytes
#define MYSTERIOUS_OFFSET (63)

#define MODE_INIT      (0x0)
#define MODE_DU        (0x1)
#define MODE_GC16      (0x2)
#define MODE_GC4       WAVEFORM_MODE_GC16
#define MODE_GC16_FAST (0x3)
#define MODE_A2        (0x4)
#define MODE_GL16      (0x5)
#define MODE_GL16_FAST (0x6)
#define MODE_DU4       (0x7)
#define MODE_REAGL     (0x8)
#define MODE_REAGLD    (0x9)
#define MODE_GL4       (0xA)
#define MODE_GL16_INV  (0xB)


int write_table(uint32_t table_addr, uint32_t* addrs, FILE* outfile, uint32_t max);

typedef struct {
  uint32_t key;
  const char* val;
} Pair;


Pair update_modes[] = {
  {MODE_INIT, "INIT (panel initialization / clear screen to white)"},
  {MODE_DU, "DU (direct update, gray to black/white transition, 1bpp)"},
  {MODE_GC16, "GC16 (high fidelity, flashing, 4bpp)"},
  {MODE_GC16_FAST, "GC16_FAST (medium fidelity, 4bpp)"},
  {MODE_A2, "A2 (animation update, fastest and lowest fidelity)"},
  {MODE_GL16, "GL16 (high fidelity from white transition, 4bpp)"},
  {MODE_GL16_FAST, "GL16_FAST (medium fidelity from white transition, 4bpp)"},
  {MODE_DU4, "DU4 (direct update, medium fidelity, text to text, 2bpp)"},
  {MODE_REAGL, "REAGL (non-flashing, ghost-compensation)"},
  {MODE_REAGLD, "REAGLD (non-flashing, ghost-compensation with dithering)"},
  {MODE_GL4, "GL4 (2-bit from white transition, 2bpp)"},
  {MODE_GL16_INV, "GL16_INV (high fidelity for black transition, 4bpp)"},
  {0x00, NULL}
};

Pair mfg_codes[] = {
  {0x33, "ED060SCF (V220 6\" Tequila)"},
  {0x34, "ED060SCFH1 (V220 Tequila Hydis – Line 2)"},
  {0x35, "ED060SCFH1 (V220 Tequila Hydis – Line 3)"},
  {0x36, "ED060SCFC1 (V220 Tequila CMO)"},
  {0x37, "ED060SCFT1 (V220 Tequila CPT)"},
  {0x38, "ED060SCG (V220 Whitney)"},
  {0x39, "ED060SCGH1 (V220 Whitney Hydis – Line 2)"},
  {0x3A, "ED060SCGH1 (V220 Whitney Hydis – Line 3)"},
  {0x3B, "ED060SCGC1 (V220 Whitney CMO)"},
  {0x3C, "ED060SCGT1 (V220 Whitney CPT)"},
  {0xA0, "Unknown LGD panel"},
  {0xA1, "Unknown LGD panel"},
  {0xA2, "Unknown LGD panel"},
  {0xA3, "LB060S03-RD02 (LGD Tequila Line 1)"},
  {0xA4, "2nd LGD Tequila Line"},
  {0xA5, "LB060S05-RD02 (LGD Whitney Line 1)"},
  {0xA6, "2nd LGD Whitney Line"},
  {0xA7, "Unknown LGD panel"},
  {0xA8, "Unknown LGD panel"},
  {0x00, NULL}
};

Pair run_types[] = {
  {0x00, "[B]aseline"},
  {0x01, "[T]est/trial"},
  {0x02, "[P]roduction"},
  {0x03, "[Q]ualification"},
  {0x04, "V110[A]"},
  {0x05, "V220[C]"},
  {0x06, "D"},
  {0x07, "V220[E]"},
  {0x08, "F"},
  {0x09, "G"},
  {0x0A, "H"},
  {0x0B, "I"},
  {0x0C, "J"},
  {0x0D, "K"},
  {0x0E, "L"},
  {0x0F, "M"},
  {0x10, "N"},
  {0x00, NULL}
};

Pair fpl_platforms[] = {
  {0x00, "2.0"},
  {0x01, "2.1"},
  {0x02, "2.3"},
  {0x03, "V110"},
  {0x04, "V110A"},
  {0x06, "V220"},
  {0x07, "V250"},
  {0x08, "V220E"},
  {0x00, NULL}
};

Pair fpl_sizes[] = {
  {0x32, "5\", unknown resolution"},
  {0x3C, "6\", 800x600"},
  {0x3D, "6.1\", 1024x768"},
  {0x3F, "6\", 800x600"},
  {0x50, "8\", unknown resolution"},
  {0x61, "9.7\", 1200x825"},
  {0x63, "9.7\", 1600x1200"},
  {0x00, NULL}
};

Pair fpl_rates[] = {
  {0x50, "50Hz"},
  {0x60, "60Hz"},
  {0x85, "85Hz"},
  {0x00, NULL}
};

Pair mode_versions[] = {
  {0x00, "MU/GU/GC/PU (V100 modes)"},
  {0x01, "DU/GC16/GC4 (V110/V110A modes)"},
  {0x02, "DU/GC16/GC4 (V110/V110A modes)"},
  {0x03, "DU/GC16/GC4/AU (V220, 50Hz/85Hz modes)"},
  {0x04, "DU/GC16/AU (V220, 85Hz modes)"},
  {0x06, "? (V220, 210 dpi, 85Hz modes)"},
  {0x07, "? (V220, 210 dpi, 85Hz modes)"},
  {0x00, NULL}
};


Pair waveform_types[] = {
  {0x0B, "TE"},
  {0x0E, "WE"},
  {0x15, "WJ"},
  {0x16, "WK"},
  {0x17, "WL"},
  {0x18, "VJ"},
  {0x2B, "WR"},
  {0x3C, "AA"},
  {0x4B, "AC"},
  {0x4C, "BD"},
  {0x50, "AE"},
  {0x00, NULL}
};


Pair waveform_tuning_biases[] = {
  {0x00, "Standard"},
  {0x01, "Increased DS Blooming V110/V110E"},
  {0x02, "Increased DS Blooming V220/V220E"},
  {0x00, NULL}
};

const char* get_desc(Pair table[], unsigned int key, const char* def) {
  int i = 0;
  while(table[i].key || table[i].val) {
    if(table[i].key == key) {
      return table[i].val;
    }
    i++;
  }
  if(def) {
    return def;
  } else {
    return "Unknown";
  }
}

void print_modes(uint8_t mode_count) {
  uint8_t i;
  const char* desc;

  printf("Modes in file:\n");
  for(i=0; i < mode_count; i++) {
    desc = get_desc(update_modes, i, "Unknown mode");
    printf("  %2u: %s\n", i, desc);
  }
  printf("\n");
}

const char* get_desc_mfg_code(unsigned int mfg_code) {
  const char* desc = get_desc(mfg_codes, mfg_code, NULL);

  if(desc) return desc;

  if(mfg_code >= 0x33 && mfg_code < 0x3c) {
    return "PVI/EIH panel\0";
  } else if(mfg_code >= 0xA0 && mfg_code < 0xA8) {
    return "LGD panel\0";
  }

  return "Unknown code\0";
}


struct waveform_data_header {
  uint32_t checksum:32; // 0
  uint32_t filesize:32; // 4
  uint32_t serial:32; // 8 serial number
  uint32_t run_type:8; // 12
  uint32_t fpl_platform:8; // 13
  uint32_t fpl_lot:16; // 14
  uint32_t mode_version_or_adhesive_run_num:8; // 16
  uint32_t waveform_version:8; // 17
  uint32_t waveform_subversion:8; // 18
  uint32_t waveform_type:8; // 19
  uint32_t fpl_size:8; // 20 (aka panel_size)
  uint32_t mfg_code:8; // 21 (aka amepd_part_number)
  uint32_t waveform_tuning_bias_or_rev:8; // 22
  uint32_t fpl_rate:8; // 23 (aka frame_rate)
  uint32_t unknown0:8;
  uint32_t vcom_shifted:8;
  uint32_t unknown1:16;
  uint32_t xwia:24; // address of extra waveform information
  uint32_t cs1:8; // checksum 1
  uint32_t wmta:24;
  uint32_t fvsn:8;
  uint32_t luts:8;
  uint32_t mc:8; // mode count (length of mode table - 1)
  uint32_t trc:8; // temperature range count (length of temperature table - 1)
  uint32_t advanced_wfm_flags:8;
  uint32_t eb:8;
  uint32_t sb:8;
  uint32_t reserved0_1:8;
  uint32_t reserved0_2:8;
  uint32_t reserved0_3:8;
  uint32_t reserved0_4:8;
  uint32_t reserved0_5:8;
  uint32_t cs2:8; // checksum 2
}__attribute__((packed));

struct pointer {
  uint32_t addr:24;
  uint8_t checksum:8;
}__attribute__((packed));

struct temp_range {
  uint8_t from;
  uint8_t to;
};

struct packed_state {
  uint8_t s0:2;
  uint8_t s1:2;
  uint8_t s2:2;
  uint8_t s3:2;
}__attribute__((packed));

struct unpacked_state {
  uint8_t s0;
  uint8_t s1;
  uint8_t s2;
  uint8_t s3;
}__attribute__((packed));


uint8_t get_bits_per_pixel(struct waveform_data_header* header) {
  return ((header->luts & 0xc) == 4) ? 5 : 4;
}


void compute_crc_table(unsigned int* crc_table) {
   unsigned c;
   int n, k;
   for (n = 0; n < 256; n++) {
      c = (unsigned) n;
      for (k = 0; k < 8; k++) {
         if (c & 1) {
            c = 0xedb88320L ^ (c >> 1);
         }
         else {
            c = c >> 1;
         }
      }
      crc_table[n] = c;
   }
}


unsigned int update_crc(unsigned int* crc_table, unsigned crc,
                           unsigned char *buf, int len) {

  char b;
  unsigned c = crc ^ 0xffffffff;
  int i;
  
  for(i=0; i < len; i++) {
    if(!buf) {
      b = 0;
    } else {
      b = buf[i];
    }
    c = crc_table[(c ^ b) & 0xff] ^ (c >> 8);
  }
  
  return c ^ 0xffffffff;
}

unsigned crc32(unsigned char *buf, int len) {
  static unsigned int crc_table[256];
  
  compute_crc_table(crc_table);
  
  return update_crc(crc_table, 0, buf, len);
}

// TODO 

int compare_checksum(unsigned char* data, struct waveform_data_header* header) {
  unsigned int crc;
  unsigned int crc_table[256];
  compute_crc_table(crc_table);
  //crc = update_crc(crc_table, 0, NULL, 4);
  crc = update_crc(crc_table, crc, data+4, header->filesize - 4);

  if(crc != header->checksum) {
    return -1;
  }
  return 0;
}
  

int bubble_sort(uint32_t* wav_addrs) {
  uint32_t i;
  uint32_t j;
  uint32_t tmp;

  if(!wav_addrs) return 0;

  for(i=0; i < MAX_WAVEFORMS; i++) {
    if(!wav_addrs[i]) break; // zero value means end of array

    for(j=0; j < MAX_WAVEFORMS; j++) {
      if(!wav_addrs[j]) break; // zero value means end of array

      if(i == j) continue;

      if((i < j && wav_addrs[i] > wav_addrs[j]) || (i > j && wav_addrs[i] < wav_addrs[j])) {
        tmp = wav_addrs[i];
        wav_addrs[i] = wav_addrs[j];
        wav_addrs[j] = tmp;
      }
    }
  }
  return i; // return length
}

int add_addr(uint32_t* addrs, uint32_t addr, uint32_t max) {
  uint32_t i;

  for(i=0; i < max; i++) {
    if(addrs[i] == addr) {
      return 0; // this address was already in the array
    }
    if(!addrs[i]) {
      addrs[i] = addr;
      return 1; // added
    }
  }
  fprintf(stderr, "Encountered more addresses than our hardcoded max\n");
  return -1;
}

void print_header(struct waveform_data_header* header, int is_wbf) {
  printf("Header info:\n");
  if(is_wbf) {
    printf("  File size (according to header): %d bytes\n", header->filesize);
  }
  printf("  Serial number: %d\n", header->serial);
  printf("  Run type: 0x%x | %s\n", header->run_type, get_desc(run_types, header->run_type, NULL));
  printf("  Manufacturer code: 0x%x | %s\n", header->mfg_code, get_desc_mfg_code(header->mfg_code));

  printf("  Frontplane Laminate (FPL) platform: 0x%x | %s\n", header->fpl_platform, get_desc(fpl_platforms, header->fpl_platform, NULL));
  printf("  Frontplane Laminate (FPL) lot: %d\n", header->fpl_lot);
  printf("  Frontplane Laminate (FPL) size: 0x%x | %s\n", header->fpl_size, get_desc(fpl_sizes, header->fpl_size, NULL));
  printf("  Frontplane Laminate (FPL) rate: 0x%x | %s\n", header->fpl_rate, get_desc(fpl_rates, header->fpl_rate, NULL));

  printf("  Waveform version: %d\n", header->waveform_version);
  printf("  Waveform sub-version: %d\n", header->waveform_subversion);

  printf("  Waveform type: 0x%x | %s\n", header->waveform_type, get_desc(waveform_types, header->waveform_type, NULL));

  // if waveform_type is WJ or earlier 
  // then waveform_tuning_bias_or_rev is the tuning bias.
  // if it is WR type or later then it is the revision.
  // if it is in between then we don't know.
  if(header->waveform_type <= 0x15) { // WJ type or earlier
    printf("  Waveform tuning bias: 0x%x | %s\n", header->waveform_tuning_bias_or_rev, get_desc(waveform_tuning_biases, header->waveform_tuning_bias_or_rev, NULL));
    printf("  Waveform revision: Unknown\n");    
  } else if(header->waveform_type >= 0x2B) { // WR type or later
    printf("  Waveform tuning bias: Unknown\n");
    printf("  Waveform revision: %d\n", header->waveform_tuning_bias_or_rev);
  } else {
    printf("  Waveform tuning bias: Unknown\n");
    printf("  Waveform revision: Unknown\n");
  }

  // if fpl_platform is < 3 then 
  // mode_version_or_adhesive_run_num is the adhesive run number
  if(header->fpl_platform < 3) {
    printf("  Adhesive run number: %d\n", header->mode_version_or_adhesive_run_num);
    printf("  Mode version: Unknown\n");
  } else {
    printf("  Adhesive run number: Unknown\n");
    printf("  Mode version: 0x%x | %s\n", header->mode_version_or_adhesive_run_num, get_desc(mode_versions, header->mode_version_or_adhesive_run_num, NULL));
  }

  printf("  Number of modes in this waveform: %d\n", header->mc + 1);
  printf("  Number of temperature ranges in this waveform: %d\n", header->trc + 1);

  printf("  4 or 5-bits per pixel: %u\n", get_bits_per_pixel(header));

  printf("\n");
}


uint32_t get_waveform_length(uint32_t* wav_addrs, uint32_t wav_addr) {
  uint32_t i;

  for(i=0; i < MAX_WAVEFORMS - 1; i++) {
    if(wav_addrs[i] == wav_addr) {
      if(!wav_addrs[i]) return 0;

      return wav_addrs[i+1] - wav_addr;
    }
  }
  return 0;
}

uint16_t parse_waveform(char* data, uint32_t* wav_addrs, uint32_t wav_addr, FILE* outfile) {
  uint32_t i, j;
  struct packed_state* s;
  struct unpacked_state u;
  uint16_t count;
  int fc_active;
  int zero_pad;
  size_t written;
  uint16_t state_count = 0;
  char* waveform = data + wav_addr;

  // TODO
  // We are cutting off the last two bytes
  // since we don't know what they are.
  // See section on unsolved mysteries at the top of this file.
  uint32_t len = get_waveform_length(wav_addrs, wav_addr) - 2;
  if(!len) {
    fprintf(stderr, "Could not find waveform length\n");
    return -1;
  }

  fc_active = 0;
  zero_pad = 0;
  i = 0;
  while(i < len - 1) {
    // 0xfc is a start and end tag for a section
    // of one-byte bit-patterns with an assumed count of 1
    if((uint8_t) waveform[i] == 0xfc) {
      fc_active = (fc_active) ? 0 : 1;
      i++;
      continue;
    }

    s = (struct packed_state*) waveform + i;

    if(fc_active) { // 1-byte pattern (count is always 1)
      count = 1;
      zero_pad = 1;
      i++;
    } else { // 2-byte pattern (second byte is count)
      if(i >= len - 1) {
        count = 1;
      } else {
        count = (uint8_t) waveform[i + 1] + 1;
      }
      zero_pad = 0;
      i += 2;
    }

    state_count += count * 4;

    if(outfile) {

      u.s0 = s->s0;
      u.s1 = s->s1;
      u.s2 = s->s2;
      u.s3 = s->s3;

      for(j=0; j < count; j++) {

        written = fwrite(&u, 1, sizeof(u), outfile);
        if(written != sizeof(u)) {
          fprintf(stderr, "Error writing waveform to output file: %s\n", strerror(errno));
          return -1;
        }
      }
    }
  }

  return state_count;
}

int parse_temp_ranges(struct waveform_data_header* header, char* data, char* tr_start, uint8_t tr_count, uint32_t* wav_addrs, int first_pass, FILE* outfile, int do_print) {
  struct pointer* tr;
  uint8_t checksum;
  uint8_t i;
  uint16_t state_count;
  size_t written;
  long ftable;
  long fprev;
  long fcur;
  uint32_t tr_addrs[256]; // temperature range addresses for output file
  uint32_t tr_table_addr; // temperature range table output start address

  if(!tr_count) {
    return 0;
  }

  memset(tr_addrs, 0, sizeof(tr_addrs));

  if(do_print) {
    printf("    Temperature ranges: \n");
  }


  if(outfile) {
    ftable = ftell(outfile);
    if(ftable < 0) {
      fprintf(stderr, "Error getting position in file: %s\n", strerror(errno));
      return -1;
    }
    if(fseek(outfile, (header->trc + 1) * 8, SEEK_CUR) < 0) {
      fprintf(stderr, "Error seeking in output file: %s\n", strerror(errno));
      return -1;      
    }
  }

  for(i=0; i < tr_count; i++) {
    if(do_print) {
      printf("      Checking range %2u: ", i);
    }
    tr = (struct pointer*) tr_start;
    checksum = tr_start[0] + tr_start[1] + tr_start[2];
    if(checksum != tr->checksum) {
      if(do_print) {
      printf("Failed\n");
      }
      return -1;
    }

    if(first_pass) {
      if(add_addr(wav_addrs, tr->addr, MAX_WAVEFORMS) < 0) {
        return -1;
      }
    } else {

      if(outfile) {

        fprev = ftell(outfile); // save position to use for writing phase count
        if(fprev < 0) {
          fprintf(stderr, "Error getting position in file: %s\n", strerror(errno));
          return -1;
        }

        if(add_addr(tr_addrs, fprev - MYSTERIOUS_OFFSET, MAX_TEMP_RANGES) < 0) {
          return -1;
        }

        if(fseek(outfile, 8, SEEK_CUR) < 0) {
          fprintf(stderr, "Error seeking in output file: %s\n", strerror(errno));
          return -1;
        }
      }
      state_count = parse_waveform(data, wav_addrs, tr->addr, outfile);
      if(state_count < 0) {
        return -1;
      }

      if(do_print) {
        printf("%4u phases\n", state_count / 256);
      }

      if(outfile) {
        fcur = ftell(outfile); // save current position in file
        if(fcur < 0) {
          fprintf(stderr, "Error getting position in file: %s\n", strerror(errno));
          return -1;
        }
        if(fseek(outfile, fprev, SEEK_SET) < 0) {
          fprintf(stderr, "Error seeking in output file: %s\n", strerror(errno));
          return -1;
        }

        state_count = htons(state_count);
        // write state count
        written = fwrite(&state_count, sizeof(state_count), 1, outfile);
        if(written != 1) {
          fprintf(stderr, "Error writing state count to output file: %s\n", strerror(errno));
          return -1;
        }

        // restore file position to end of previously written data
        if(fseek(outfile, fcur, SEEK_SET) < 0) {
          fprintf(stderr, "Error seeking in output file: %s\n", strerror(errno));
          return -1;
        }
      }
    }

    tr_start += 4;
  }
  if(do_print) {
    printf("\n");
  }

  if(outfile) {
    if(write_table(ftable, tr_addrs, outfile, MAX_TEMP_RANGES) < 0) {
      fprintf(stderr, "Error writing temperature range table\n");
      return -1;
    }

  }

  return 0;
}


int parse_modes(struct waveform_data_header* header, char* data, char* mode_start, uint8_t mode_count, uint8_t temp_range_count, uint32_t* wav_addrs, int first_pass, FILE* outfile, int do_print) {
  struct pointer* mode;
  uint8_t checksum;
  uint8_t i;
  long pos;
  uint32_t mode_addrs[256]; // mode addresses for output file
  uint32_t mode_table_addr; // mode table output start address

  if(!mode_count) {
    return 0;
  }

  memset(mode_addrs, 0, sizeof(mode_addrs));

  if(do_print) {
    printf("Modes: \n");
  }

  for(i=0; i < mode_count; i++) {
    if(do_print) {
      printf("  Checking mode %2u: ", i);
    }
    mode = (struct pointer*) mode_start;
    checksum = mode_start[0] + mode_start[1] + mode_start[2];
    if(checksum != mode->checksum) {
      if(do_print) {
        printf("Failed\n");
      }
      return -1;
    }

    if(outfile) {

      pos = ftell(outfile) - MYSTERIOUS_OFFSET;
      if(pos < 0) {
        fprintf(stderr, "Error getting position in file: %s\n", strerror(errno));
        return -1;
      }

      if(add_addr(mode_addrs, pos, MAX_MODES) < 0) {
        return -1;
      }
    }

    if(do_print) {
      printf("Passed\n");
    }
    if(parse_temp_ranges(header, data, data + mode->addr, temp_range_count, wav_addrs, first_pass, outfile, do_print) < 0) {
      return -1;
    }
    
    mode_start += 4;
  }


  if(outfile) {
    // the + 2 is because there is one more temperature range than the
    // count in header->trc and then because these are ranges there is one
    // more temperature than the number of ranges
    mode_table_addr = sizeof(struct waveform_data_header) + header->trc + 2;

    if(write_table(mode_table_addr, mode_addrs, outfile, MAX_MODES) < 0) {
      fprintf(stderr, "Error writing mode table\n");
      return -1;
    }
  }

  return 0;
}

int check_xwia(char* xwia, int do_print) {
  uint8_t xwia_len;
  uint8_t i;
  uint8_t checksum;
  int non_printables = 0;

  xwia_len = *(xwia);
  xwia = xwia + 1;
  checksum = xwia_len;


  for(i=0; i < xwia_len; i++) {
    if(!isprint(xwia[i])){
      non_printables++;
    }
    checksum += xwia[i];
  }

  if(do_print) {
    
    printf("Extra Waveform Info (xwia): ");

    if(!xwia_len) {
      printf("None");
    } else if(non_printables) {
      printf("(%u bytes containing %u unprintable characters)", xwia_len, non_printables);
    } else {
      for(i=0; i < xwia_len; i++) {
        printf("%c", xwia[i]);
      }
    }
    
    printf("\n\n");
  }

  if(checksum != (uint8_t) *(xwia + xwia_len)) {
    return -1;
  }
  return 0;
}

int parse_temp_range_table(char* table, uint8_t range_count, FILE* outfile, int do_print) {
  uint8_t i;
  uint8_t checksum;
  struct temp_range range;
  size_t written;

  if(!range_count) {
    return 0;
  }

  if(do_print) {
    printf("Supported temperature ranges:\n");
  }

  checksum = 0;
  for(i=0; i < range_count; i++) {
    range.from = (uint8_t) table[i];
    range.to = (uint8_t) table[i+1];
    if(do_print) {
      printf("  %u - %u °C\n", range.from, range.to);
    }
    checksum += range.from;
  }
  checksum += range.to;
 
  if(checksum != (uint8_t) table[range_count+1]) {
    return -1;
  }

  if(outfile) {
    written = fwrite(table, 1, range_count+1, outfile);
    if(written != range_count+1) {
      fprintf(stderr, "Error writing temperature range table to output file: %s\n", strerror(errno));
      return -1;
    }
  }

  if(do_print) {
    printf("\n");
  }

  return 0;
}

int write_table(uint32_t table_addr, uint32_t* addrs, FILE* outfile, uint32_t max) {
  int i;
  size_t written;
  uint32_t addr;
  long prev;

  prev = ftell(outfile);
  if(prev < 0) {
    return -1;
  }

  if(fseek(outfile, table_addr, SEEK_SET) < 0) {
    fprintf(stderr, "Error seeking in output file: %s\n", strerror(errno));
    return -1;
  }

  for(i=0; i < max; i++) {
    if(!addrs[i]) break;

    addr = addrs[i];

    written = fwrite(&addr, 1, sizeof(uint32_t), outfile);
    if(written != sizeof(uint32_t)) {
      fprintf(stderr, "Error writing address table to output file: %s\n", strerror(errno));
      return -1;
    }

    if(fseek(outfile, 4, SEEK_CUR) < 0) {
      fprintf(stderr, "Error seeking in output file: %s\n", strerror(errno));
      return -1;
    }
  }

  if(fseek(outfile, prev, SEEK_SET) < 0) {
    fprintf(stderr, "Error seeking in output file: %s\n", strerror(errno));
    return -1;
  }  

  return 0;
}

int write_header(FILE* outfile, struct waveform_data_header* header) {
  size_t written;

  written = fwrite(header, 1, sizeof(struct waveform_data_header), outfile);
  if(written < sizeof(struct waveform_data_header)) {
    return -1;
  }

  return 0;
}

void usage(FILE* fd) {
  fprintf(fd, "\n");
  fprintf(fd, "Usage: inkwave file.wbf/file.wrf [-o output.wrf]\n");
  fprintf(fd, "\n");
  fprintf(fd, "  Convert a .wbf file to a .wrf file\n");
  fprintf(fd, "  or if no output file is specified display human\n");
  fprintf(fd, "  readable info about the specified .wbf or .wrf file.\n");
  fprintf(fd, "\n");
  fprintf(fd, "Options:\n");
  fprintf(fd, "\n");
  fprintf(fd, "  -o: Specify output file.\n");
  fprintf(fd, "\n");
  fprintf(fd, "  -f wrf/wbf: Force inkwave to interpret input file\n");
  fprintf(fd, "              as either .wrf or .wbf format\n");
  fprintf(fd, "              regardless of file extension.\n");
  fprintf(fd, "\n");
  fprintf(fd, "  -h: Display this help message.\n");
  fprintf(fd, "\n");
}
/*
int main(int argc, char **argv) {

  char* data;
  char* infile_path;
  FILE* infile = NULL;
  size_t len;
  struct waveform_data_header* header; // points to `data` at beginning of header
  struct stat st;
  char* modes; // points to `data` where the modes table begins
  char* temp_range_table; // points to `data` where the temp range table begins
  uint32_t xwia_len;
  uint8_t mode_count;
  uint8_t temp_range_count;
  char* outfile_path = NULL;
  FILE* outfile = NULL;
  char* force_input = NULL;
  int do_print = 0;
  int force = 0;
  int c;
  uint32_t unique_waveform_count;
  uint32_t wav_addrs[MAX_WAVEFORMS]; // waveform addresses in input file
  uint32_t is_wbf;
  size_t to_alloc;

  memset(wav_addrs, 0, sizeof(wav_addrs));

  while((c = getopt(argc, argv, "o:f:h")) != -1) {
    switch (c) {
    case 'o':
      outfile_path = optarg;
      break;
    case 'f':
      force_input = optarg;
      break;
    case 'h':
      usage(stdout);
      return 0;
    }
  }

  // expecting exactly one non-option argument
  if(argc != optind + 1) {
    usage(stderr);
    return 1;
  }

  infile_path = argv[optind];

  if(force_input) {
    if(strncmp(force_input, "wbf", 3) == 0) {
      is_wbf = 1;
    } else if(strncmp(force_input, "wrf", 3) == 0) {
      is_wbf = 0;
    } else {
      fprintf(stderr, "Only wbf and wrf format is supported\n");
      goto fail;
    }
  } else {
    if(strlen(infile_path) < 4) {
      fprintf(stderr, "File has neither .wbf or .wrf extension\n");
      fprintf(stderr, "Consider using `-f` to bypass file format detection\n");
      goto fail;
    }
    if(strncmp(infile_path + strlen(infile_path) - 4, ".wbf", 4) == 0) {
      is_wbf = 1;
    } else if(strncmp(infile_path + strlen(infile_path) - 4, ".wrf", 4) == 0) {
      is_wbf = 0;
    } else {
      fprintf(stderr, "File has neither .wbf or .wrf extension\n");
      fprintf(stderr, "Consider using `-f` to bypass file format detection\n");
      goto fail;
    }  
  }

  if(!is_wbf && outfile_path) {
    fprintf(stderr, "Conversion from .wrf format not supported\n");
    goto fail;
  }

  infile = fopen(infile_path, "r");
  if(!infile) {
    fprintf(stderr, "Opening file %s failed: %s\n", infile_path, strerror(errno));
    goto fail;
  }

  if(stat(infile_path, &st) < 0) {
    fprintf(stderr, "Error getting file size for: %s\n", strerror(errno));
    goto fail;
  }

  if(is_wbf) {
    to_alloc = st.st_size;
  } else {
    to_alloc = sizeof(struct waveform_data_header);
  }

  data = malloc(to_alloc);
  if(!data) {
    fprintf(stderr, "Failed to allocate %d bytes of memory: %s\n", (int) st.st_size, strerror(errno));
    goto fail;
  }

  if(outfile_path) {
    outfile = fopen(outfile_path, "w");
    if(!outfile) {
      fprintf(stderr, "Opening file %s for writing failed: %s\n", outfile_path, strerror(errno));
      goto fail;
    }
  }

  if(!outfile) {
    do_print = 1;
  }

  if(do_print) {
    printf("\n");
    printf("File size: %d bytes\n", (int) st.st_size);
    printf("\n");
  }

  len = fread(data, 1, to_alloc, infile);
  if(len <= 0) {
    fprintf(stderr, "Reading file %s failed: %s\n", infile_path, strerror(errno));
    goto fail;
  }

  // start of header
  header = (struct waveform_data_header*) data;

  if(is_wbf) {
    if(header->filesize != st.st_size) {
      fprintf(stderr, "Actual file size does not match file size reported by waveform header\n");
      goto fail;
    }
  }

  if(outfile) {
    if(get_bits_per_pixel(header) != 4) {
      fprintf(stderr, "This waveform uses 5 bits per pixel which is not yet support\n");
      goto fail;
    }
  }

  if(is_wbf) {
    if(compare_checksum(data, header) < 0)  {
      fprintf(stderr, "Checksum error\n");
      goto fail;
    }
  }

  if(do_print) {
    print_header(header, is_wbf);
    
    if(header->fpl_platform < 3) {
      printf("Modes: Unknown (no mode version specified)\n");
    } else {
      print_modes(header->mc + 1);
    }
  }

  if(!is_wbf) {
    return 0;
  }

  if(outfile) {
    if(write_header(outfile, header) < 0) {
      fprintf(stderr, "Writing header to output failed\n");
      goto fail;
    }
  }

  // start of temperature range table
  temp_range_table = data + sizeof(struct waveform_data_header);

  if(parse_temp_range_table(temp_range_table, header->trc + 1, outfile, do_print)) {
    fprintf(stderr, "Temperature range checksum error\n");
    goto fail;   
  }

  if(outfile) {
    if(fseek(outfile, 8 * (header->mc + 1), SEEK_CUR) < 0) {
      fprintf(stderr, "Error seeking in output file: %s\n", strerror(errno));
      return -1;      
    }
  }

  if(header->xwia) { // if xwia is 0 then there is no xwia info
    xwia_len = data[header->xwia];

    if(check_xwia(data + header->xwia, do_print) < 0) {
      fprintf(stderr, "xwia checksum error\n");
      goto fail;
    }
  } else {
    xwia_len = 0;
  }

  // first byte of xwia contains the length
  // last byte after xwia is a checksum
  modes = data + header->xwia + 1 + xwia_len + 1;

  if(parse_modes(header, data, modes, header->mc + 1, header->trc + 1, wav_addrs, 1, NULL, 0) < 0) {
    fprintf(stderr, "Parse error during first pass\n");
    goto fail; 
  }

  unique_waveform_count = bubble_sort(wav_addrs);
  if(do_print) {
    printf("Number of unique waveforms: %u\n\n", unique_waveform_count);
  }
  
  // add file endpoint to waveform address table
  // since we use this to determine end address of each waveform
  if(add_addr(wav_addrs, st.st_size, MAX_WAVEFORMS) < 0) {
    fprintf(stderr, "Failed to add file end address to waveform table.\n");
    goto fail;
  }

  // parse modes again since we now have all the sorted waveform addresses
  if(parse_modes(header, data, modes, header->mc + 1, header->trc + 1, wav_addrs, 0, outfile, do_print) < 0) {
    fprintf(stderr, "Parse error during second pass\n");
    goto fail; 
  }

  return 0;

 fail:
  if(infile) {
    fclose(infile);
  }
  if(outfile) {
    fclose(outfile);
  }
  return 1;
}
*/
