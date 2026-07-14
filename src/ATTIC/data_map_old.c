// old code and notes kept for reference
// =========== NO LONGER VALID, KEPT FOR HISTORICAL REASONS ===========
//
// data zblocks layout example (2D example)
//
// zblocks along i (x) : 10   (ZNI)
// zblocks along j (y) : 11   (ZNJ)
// stripe factor : 4        (SF0)
// top stripe factor        (SF1)  (may be smaller than SF0)
//
// the number (ZI) in the zblocks is the sequential position in the data map (Z index)
//
// SF1 = MODULO(ZNJ , SF0)
// if(SF1 == 0) then SF1 = SF0
// STJ = J / SF0                ( stripe number for row J )
// J0  = STJ * SF0              ( J index of lower row in stripe )
// if(J0 + SF0 > ZNJ) then SF = SF1 else SF = SF0    ( stripe factor for this row )
// ZI = (J0 * ZNI) + (J - J0) + (SF1 * I)            ( Z index of tile[I,J] )
//
// row (J)                                                           stripe
//     +-----+-----+-----+-----+-----+-----+-----+-----+-----+-----+
//     |     |     |     |     |     |     |     |     |     |     |
//  10 |  82 |  85 |  88 |  91 |  94 |  97 | 100 | 103 | 106 | 109 |
//     |     |     |     |     |     |     |     |     |     |     |
//     +-----+-----+-----+-----+-----+-----+-----+-----+-----+-----+
//     |     |     |     |     |     |     |     |     |     |     |
//   9 |  81 |  84 |  87 |  90 |  93 |  96 |  99 | 102 | 105 | 108 |
//     |     |     |     |     |     |     |     |     |     |     |
//     +-----+-----+-----+-----+-----+-----+-----+-----+-----+-----+
//     |     |     |     |     |     |     |     |     |     |     |
//   8 |  80 |  83 |  86 |  89 |  92 |  95 |  98 | 101 | 104 | 107 |
//     |     |     |     |     |     |     |     |     |     |     |  [2]
//     +=====+=====+=====+=====+=====+=====+=====+=====+=====+=====+=======
//     |     |     |     |     |     |     |     |     |     |     |
//   7 |  43 |  47 |  51 |  55 |  59 |  63 |  67 |  71 |  75 |  79 |
//     |     |     |     |     |     |     |     |     |     |     |
//     +-----+-----+-----+-----+-----+-----+-----+-----+-----+-----+
//     |     |     |     |     |     |     |     |     |     |     |
//   6 |  42 |  46 |  50 |  54 |  58 |  62 |  66 |  70 |  74 |  78 |
//     |     |     |     |     |     |     |     |     |     |     |
//     +-----+-----+-----+-----+-----+-----+-----+-----+-----+-----+
//     | ****************|     |     |     |     |     |     |     |
//   5 | *41 |  45 |  49*|  53 |  57 |  61 |  65 |  69 |  73 |  77 |
//     | *   |     |    *|     |     |     |     |     |     |     |
//     +-*---+-----+----*+-----+-----+-----+-----+-----+-----+-----+
//     | *   |     |    *|     |     |     |     |     |     |     |
//   4 | *40 |  44 |  48*|  52 |  56 |  60 |  64 |  68 |  72 |  76 |
//     | *   |     |    *|     |     |     |     |     |     |     |  [1]
//     +=*===+=====+====*+=====+=====+=====+=====+=====+=====+=====+=======
//     | *   |     |    *|     |     |     |     | ##########|     |
//   3 | * 3 |   7 |  11*|  15 |  19 |  23 |  27 | #31 |  35#|  39 |
//     | *   |     |    *|     |     |     |     | #   |    #|     |
//     +-*---+-----+----*+-----+-----+-----+-----+-#---+----#+-----+
//     | *   |     |    *| %%%%%%%%%%%%%%%%|     | #   |    #|     |
//   2 | * 2 |   6 |  10*| %14 |  18 |  22%|  26 | #30 |  34#|  38 |
//     | ****************| %   |     |    %|     | #   |    #|     |
//     +-----+-----+-----+-%---+-----+----%+-----+-#---+----#+-----+
//     |     |     |     | %   |     |    %|     | #   |    #|     |
//   1 |   1 |   5 |   9 | %13 |  17 |  21%|  25 | #29 |  33#|  37 |
//     |     |     |     | %%%%%%%%%%%%%%%%|     | #   |    #|     |
//     +-----+-----+-----+-----+-----+-----+-----+-#---+----#+-----+
//     |     |     |     |     |     |     |     | #   |    #|     |
//   0 |   0 |   4 |   8 |  12 |  16 |  20 |  24 | #28 |  32#|  36 |
//     |     |     |     |     |     |     |     | ##########|     |  [0]
//     +=====+=====+=====+=====+=====+=====+=====+=====+=====+=====+=======
//        0     1     2     3     4     5     6     7     8     9    column (I)
//
// stripe delimiter : '='
//
// * delimited region, 12 zblocks
// option 1 : ( probably slowest )
//   read zblocks 2->3, 6->7, 10->11, 40->41, 44->45, 48->49 [ 12 zblocks read, 6 IO requests ]
// option 2 :
//   read zblocks 2->11 and 40->49 [ 20 zblocks read, 2 IO requests ]
// option 3 : ( probably fastest)
//   read zblocks 2->49 [ 48 zblocks read, 1 IO request ]
//
// % delimited region, 6 zblocks
//  option 1 : ( probably slower )
//    read zblocks 13->14, 17->18, 21->22 [ 6 zblocks read, 3 IO requests ]
//  option 2 : ( probably fastest )
//    read zblocks 13->22 [ 10 zblocks read, 1 IO request ]
//
// # delimited region, 8 zblocks
//  option 1 : ( ideal case )
//    read zblocks 28->35 [ 8 zblocks read, 1 IO request ]
//
// the 3D extension is simple
// each block has dimensions lni|li0 x lnj|lj0 x lnk|lkx
// offset and size have dimensions zni x znj x znk
// there is no striping along z
//  compression may be 2D (lnk blocks lni x lnj) or 3D (1 block lni x lnj x lnk)
//
// =================================================================
//
// NOTE: Zindex_to_ij, Zindex_from_ij, Z_map_index  may become irrelevant (zigzag indexing no longer contemplated)
#if 0
// translate block Z (zigzag) index into block (i,j) coordinates
// zij    [IN] : Z (zigzag) index
// nti    [IN] : row size
// ntj    [IN] : number of rows
// sf0    [IN] : stripe width, last (top) stripe may be narrower
// the function returns the i and j coordinates in struct ij_range
index_pair Zindex_to_ij(int32_t zij, int32_t nti, int32_t ntj, int32_t sf0){
  index_pair ij ;
  int32_t sf1, i, j, st0, sz0, sti, stn, j0 ;

  ij.i = -1 ;                               // precondition for miserable failure
  ij.j = -1 ;
  // a negative value for zij would translate into a huge unsigned number
  if(zij < 0) goto end ;
  if(zij >= nti * ntj) goto end ;           // zij is out of bounds

  stn = (ntj - 1) / sf0 ;                   // stripe number for last (top) row
  j0  = stn * sf0 ;                         // j index of lowest row in last stripe
  sz0 = stn * nti * sf0 ;                   // z index of first point in last stripe
  sf1 = ntj - j0 ;                          // current width = width of last stripe
  if(zij < sz0) sf1 = sf0 ;                 // not in last stripe, current width = sf0

  st0 = zij / (sf0 * nti) ;                 // current stripe number
  sz0 = st0 * (sf0 * nti) ;                 // first z index in stripe
  sti = (zij - sz0) ;                       // z index offset in stripe
  i   = sti / sf1 ;                         // position along i
  j   = sti - (i * sf1) ;                   // modulo(sti, sf1) (j position in current stripe)
  j  += st0 * sf0 ;                         // position along j (add stripe j start position)
  ij.i = i ;                                // i coordinate of block
  ij.j = j ;                                // j coordinate of block
end:
  return ij ;                               // return pair of coordinates
}

// translate block i, j coordinates into block Z (zigzag) index
// i      [IN] : index in row
// j      [IN] : index of row
// nti    [IN] : row size
// ntj    [IN] : number of rows
// sf0    [IN] : stripe width (last stripe may be narrower)
// the function returns the Z (zigzag) index associated with block(i,j)
int32_t Zindex_from_ij(int32_t i, int32_t j, int32_t nti, int32_t ntj, int32_t sf0){
  int32_t zi, sf1, j0, stj, stn ;

  zi = -1 ;                                 // precondition for miserable failure
  // a negative value for i or j would translate into a huge unsigned number
  if(i < 0 || j < 0) goto end ;
  if( i >= nti || j >= ntj) goto end ;      // i or j is out of bounds

  stn = (ntj - 1) / sf0 ;                   // stripe number for last row
  j0  = stn * sf0 ;                         // j index of lowest row in last stripe
  stj = j / sf0 ;                           // stripe number for this row
  sf1 = ntj - j0 ;                          // current width = width of last stripe
  if(j < j0) sf1 = sf0 ;                    // not in last stripe, current width = sf0

  j0 = stj * sf0 ;                          // j index of lowest row in current stripe
  zi = (j0 * nti) +                         // lower left corner of stripe
       (j - j0) +                           // number of rows above bottom of stripe
       (i * sf1) ;                          // i * current stripe width
end:
  return zi ;
}

// Z (zigzag) block index from block indexes, using data map
// map    [IN] : data map
// i      [IN] : i (column) position in 2D block grid
// j      [IN] : j (row) position in 2D block grid
// return [ij] Z block index
int32_t Z_map_index(zmap *map, int32_t i, int32_t j){
//   index_pair ij = block_index(map, i, j) ;
//   return Zindex_from_ij(i, j, map->fhead.zni, map->fhead.znj, map->fhead.stripe) ;
  return Zindex_from_ij(i, j, map->fhead.zni, map->fhead.znj, 1) ;
}
#endif
#if 0
// adjust offset table using the sizes table
// map [INOUT] : pointer to zmap (mmap/fmap/sizes/...) struct
// return number of entries in tables if successful, 0 otherwise
int adjust_map_offsets(zmap *map){
  if(map == NULL) return 0 ;
  uint32_t nijk = map->fhead.zijk ;
  uint64_t t ;
  uint64_t limit = RANGE_ELEMENTS(map->mhead.drng) ;    // max number of elements that can be accomodated
  map->mhead.orng.bot[0] = 0 ;
  for(uint32_t i=0 ; i<nijk ; i++){
    t = map->mhead.orng.bot[i] + map->size[i] ;
    if(t > limit) return 0 ;                            // element number exceeded
    map->mhead.orng.bot[i+1] = t ;
  }
  return nijk ;
}

// create sizes from block dimensions
int bsize_zmap(zmap *map, size_t esize){
  if(map == NULL) return 0 ;
  int i, j, lni, lnj, ij ;
  ssize_t lsize ;
  lnj = map->fhead.lj0 ;
  ij = 0 ;
  for(j=0 ; j<map->fhead.znj ; j++, lnj=map->fhead.lnj){
    lni = map->fhead.li0 ;
    for(i=0 ; i<map->fhead.zni ; i++, lni=map->fhead.lni ){
      lsize = esize ;
      lsize *= (lni * lnj) ;
      map->size[ij++] = lsize ;
    }
  }
  return ij ;
}

// TODO : redo the whole function

// (re)allocate table of pointers to packed blocks, fill it using map->size
// map  [INOUT] : pointer to data map
// data    [IN] : pointer to start of packed data (if NULL, packed data follows map in memory)
// size    [IN] : size of memory block at data in bytes
// return address of table of pointers to packed blocks, NULL if there was any error
uint64_t *mem_zmap(zmap *map, uint32_t *data, size_t size){
  int32_t zijk = map->fhead.zni * map->fhead.znj, i ;
  size_t needed = 0 ;

  if(data != NULL){    // check that enough space is available, set first/last/limit
    for(i=0 ; i<zijk ; i++){ needed += map->size[i] ; }
    if(size < needed) return NULL ;
    map->mhead.xrng.bot = map->mhead.xrng.top = data ;
//     map->mhead.first = data /*+ map->fhead.mextra*/ ;
//     map->mhead.last  = map->mhead.first ;
//     map->mhead.limit = (uint8_t *)data + size ;
//     if(DEBUG)
//       fprintf(stderr, "DEBUG mem_zmap, switching stream buffer to %16p -> %16p\n", (void *)map->mhead.first, (void *)map->mhead.limit) ;
  }
  // allocate offset table
  uint64_t *offset = (uint64_t *)malloc((zijk+1) * sizeof(uint64_t)) ;  // zijk + 1 entries needed
#if 0
  if(offset){          // allocation was successful
    if(map->mhead.offset) free(map->mhead.offset) ;  // free old table if there was one
    map->mhead.offset = offset ;
    offset[0] = map->mhead.first ;
    for(i=1 ; i<zijk+1 ; i++){
      offset[i] = offset[i-1] + map->size[i-1] ;   // recalculate offset[] using block sizes
    }
    if((uint8_t *)offset[zijk] > map->mhead.limit){   // OOPS, not enough space in data
      free(offset) ;
      offset = NULL ;
    }else{
      map->mhead.last = offset[zijk] ;                      // update last
    }
  }
#endif
  return offset ;
}
#endif
// fill map data buffer with data from address src
// data element size and dimensions will be taken from map
// void fill_zmap(zmap *map, void *src){
// }

// NOTE if first/last/limit out of map we have an external bit stream buffer

// deallocation of data map
// map  [INOUT] : pointer to data map
// full    [IN] : if zero, only deallocate pointer table to packed blocks
int free_zmap(zmap *map){
  if(map == NULL) return 1 ;
  // INVALIDATE map to prevent accidents in case of memory reuse
  map->mhead = base_mmap ;
  map->mhead.signature = 0 ;
  free(map) ;
  return 0 ; // success
  // free offset if not inside zmap struct
//   if(map->mhead.offset){
//     if(DEBUG) fprintf(stderr, "freeing map->mhead.offset at %p\n", map->mhead.offset) ;
//     free(map->mhead.offset) ;
//   }
  // free data  if not inside zmap struct
//   map->mhead.offset = NULL ;
//   if(map->mhead.options){
//     if(DEBUG) fprintf(stderr, "freeing map->mhead.options at %p\n", map->mhead.options) ;
//     free(map->mhead.options) ;
//   }
//   map->mhead.options = NULL ;
//   if(full) {
//     // NULLIFY map to prevent accidents in case of memory reuse
//     map->mhead = base_mmap ;
//     map->mhead.signature = 0 ;
//     free(map) ;
// if(DEBUG) fprintf(stderr, "FULL map free\n") ;
//   }else{
// if(DEBUG) fprintf(stderr, "PART map free\n") ;
//   }
//   return 0 ;
}
// TODO : redo the whole function
#if 0
ssize_t resize_map(zmap *map){
  int k ;
  uint32_t *current ;

  current = map->mhead.offset[0] ;          // initial position
  if(current != map->mhead.first){
    int32_t *data = (int32_t *)&(map->size[map->fhead.zni*map->fhead.znj]) ;
    fprintf(stderr, "ERROR resize_map : first map entry not pointing to start of stream\n") ;
    fprintf(stderr, "      first = %16p, start = %16p, data = %16p\n", (void *)map->mhead.first, (void *)current, (void *)data) ;
  }
  for(k=0 ; k < map->fhead.zni * map->fhead.znj ; k++){
    map->mhead.offset[k] = current ;
    current += map->size[k] ;
    if(current > map->mhead.last){
      fprintf(stderr, "ERROR: cannot resize map, not enough space occording to size table\n") ;
      return -1 ;
    }
  }
  return current - map->mhead.offset[0] ;
}
#endif
// TODO : redo the whole function
#if 0
// remove holes from data buffer, update list of memory addresses using updated sizes
ssize_t repack_map(zmap *map){
  int k ;
  uint32_t *current, *stream ;

  if(map == NULL)      return -1 ;
  if(map->mhead.offset == NULL) return -1 ;

  current = map->mhead.offset[0] ;          // initial target position
  if(current != map->mhead.first){
    int32_t *data = (int32_t *)&(map->size[map->fhead.zni*map->fhead.znj]) ;
    fprintf(stderr, "ERROR resize_map : first map entry not pointing to start of stream\n") ;
    fprintf(stderr, "current = %p, first = %16p, start = %16p, data = %16p\n", (void *)current, (void *)map->mhead.first, (void *)current, (void *)data) ;
  }
  for(k=0 ; k < map->fhead.zni * map->fhead.znj ; k++){
    stream = map->mhead.offset[k] ;         // copy from this position in memory
    map->mhead.offset[k] = current ;        // update offset pointer to new position in memory
    if(current < stream || map->size[k] != map->mhead.offset[k+1] - map->mhead.offset[k]) {  // need to copy ?
      if(DEBUG) fprintf(stderr, "copying from %6ld", current - map->mhead.offset[0]) ;
      memmove(current, stream, map->size[k] * sizeof(uint32_t)) ;    // PGI/Nvidia compile problems with DEBUG =0 and copy loop
//       int i ;
//       for(i=0 ; i < map->size[k] ; i++){ current[i] = stream[i] ; }
      if(DEBUG) fprintf(stderr, " to %6ld [%6d]\n", current + map->size[k] - map->mhead.offset[0] -1, map->size[k]) ;
    }
    current += map->size[k] ;      // update target position
  }
  map->mhead.offset[k] = current ;
  return current - map->mhead.offset[0] ;
}
#endif
