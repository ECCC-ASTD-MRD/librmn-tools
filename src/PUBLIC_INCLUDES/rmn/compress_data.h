#if ! defined(CHUNK_I) && ! defined(CHUNK_J) && ! defined(BLK_I) && ! defined(BLK_J)

#include <rmn/bi_endian_pack.h>
#include <rmn/tile_encoders.h>

// linear quantization header for style 0 (64 bits) (2 x 32 bits)
typedef struct{
  uint32_t bias:32;  // minimum absolute value (actually never more than 31 bits value)
  uint32_t resv:19,  // reserved
           shft:5 ,  // shift count (0 -> 31) for quanze/unquantize
           nbts:5 ,  // number of bits (0 -> 31) per value (nbts == 0 : same value, same sign)
           cnst:1 ,  // range is 0, constant absolute values
           allp:1 ,  // all numbers are >= 0
           allm:1 ;  // all numbers are < 0
} ieee32_0 ;

// linear quantization header for style 1 (64 bits) (2 x 32 bits)
typedef struct {
  uint32_t bias:32;  // integer offset reflecting minimum value of packed field
  uint32_t resv:16,  // reserved
           efac:8 ,  // exponent for quantization mutliplier
           nbts:5 ,  // number of bits (0 -> 31) per value (nbts == 0 : same value, same sign)
           cnst:1 ,  // range is 0, constant absolute values
           allp:1 ,  // all numbers are >= 0
           allm:1 ;  // all numbers are < 0
} ieee32_1 ;

// pseudo log quantization header (64 bits) (2 x 32 bits)
typedef struct {
  uint32_t resv:32;  // reserved
  uint32_t emax:8 ,  // exponent of largest absolute value
           emin:8 ,  // exponent of smallest absolute value
           elow:8 ,  // exponent of smallest significant absolute value
           nbts:5 ,  // number of bits (0 -> 31) per value (nbts == 0 : same value, same sign)
           clip:1 ,  // original emin < elow, clipping may occur
           allp:1 ,  // all numbers are >= 0
           allm:1 ;  // all numbers are < 0
} ieee32_p ;

// BLK_I MUST BE a multiple of 8
// BLK_J MUST BE a multiple of 8
#define BLK_I 64
#define BLK_J 64
typedef struct{        // BLK_I x BLK_J quantization block header (96 bits) (3 x 32 bits)
  // common part
  struct{
    uint32_t npti:8 ,  // first dimension (1->BLK_I)
             nptj:8 ,  // second dimension (1->BLK_J)
             qtyp:3 ,  // quantization type (q0/q1/qe/...)
             pred:2 ,  // predictor type
             resv:11;  // reserved
  } bh ;
  // quantizer dependent part
  union{
    ieee32_0 q0 ;      // linear quantization type 0
    ieee32_1 q1 ;      // linear quantization type 1
    ieee32_p qe ;      // pseudo log quantization
  } q ;
} block_base ;
typedef struct{
  block_base bh ;      // header
  uint32_t u[3] ;      // (3 x 32 bits)
}block_header ;

// CHUNK_I MUST BE a multiple of BLK_I
// CHUNK_J MUST BE a multiple of BLK_J
#define CHUNK_I 128
#define CHUNK_J 128
typedef struct{                // CHUNK_I x CHUNK_J data chunk
  uint32_t npti:16 ;           // first dimension of data chunk
  uint32_t nptj:16 ;           // first dimension of data chunk
} chunk_header ;

typedef struct{                // compression parameters
  float   max_float_error ;    // largest absolute error (float)
  int32_t max_int_error ;      // largest absolute error (integer)
  int32_t max_bits ;           // max number of bits to be used by quantizer
  int32_t dtype ;              // data type
  int32_t quantizer ;          // quantizer type (linear/log)
} compress_rules ;

typedef struct{
  uint32_t npti ;              // first dimension of full data array
  uint32_t nptj ;              // second dimension of full data array
  compress_rules r ;           // compression rules ;
} field_header ;

typedef struct{
  uint32_t size ;              // compressed chunk size (in 32 bit units)
  uint32_t offset ;            // compressed chunk offset (in 32 bit units)
} chunk_item ;

typedef struct{                // 2D nci x ncj chunk array
  uint32_t nci ;               // number of chunks along 1st dimension
  uint32_t ncj ;               // number of chunks along 2nd dimension
  chunk_item chunk[] ;         // flexible array, chunk layout
} data_map ;

typedef struct{
  data_map *map ;             // pointer to data map
  uint32_t *data ;            // pointer to data
}compressed_field ;

void compress_2d_block(void *data, int lni, int ni, int nj, compress_rules rules, bitstream *stream);
bitstream *compress_2d_chunk(void *data, int lni, int ni, int nj, compress_rules rules);
compressed_field compress_2d_data(void *data, int lni, int ni, int nj, compress_rules rules);

#endif
