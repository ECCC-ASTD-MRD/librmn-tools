#if ! defined(CHUNK_I) && ! defined(BLK_I)

#include <rmn/bi_endian_pack.h>

typedef struct{                // 8 x 8 encoding tile header
} tile_header ;

// BLK_I is a multiple of 8
// BLK_J is a multiple of 8
#define BLK_I 64
#define BLK_J 64
typedef struct{                // BLK_I x BLK_J quantization block header
} block_header ;

// CHUNK_I is a multiple of BLK_I
// CHUNK_J is a multiple of BLK_J
#define CHUNK_I 128
#define CHUNK_J 128
typedef struct{                // CHUNK_I x CHUNK_J data chunk
} chunk_header ;

typedef struct{                // compression parameters
  float   max_float_error ;    // largest absolute error (float)
  int32_t max_int_error ;      // largest absolute error (integer)
  int32_t max_bits ;           // max number of bits to be used by quantizer
  int32_t dtype ;              // data type
  int32_t quantizer ;          // quantizer type (linear/log)
} compress_rules ;

typedef struct{
  uint32_t size ;              // chunk size (in 64 bit units)
  uint32_t offset ;            // offset (in 64 bit units)
} chunk_item ;

typedef struct{                // 2D nci x ncj chunk array
  uint32_t nci ;
  uint32_t ncj ;
  chunk_item chunk[] ;
} chunk_map ;

bitstream *compress_2d_chunk(void *data, int lni, int ni, int nj, compress_rules r);
bitstream **compress_2d_data(void *data, int lni, int ni, int nj, compress_rules r);

#endif
