//
// Copyright (C) 2024  Environnement Canada
//
// This is free software; you can redistribute it and/or
// modify it under the terms of the GNU Lesser General Public
// License as published by the Free Software Foundation,
// version 2.1 of the License.
//
// This software is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
// Lesser General Public License for more details .
//
// Author:
//     M. Valin,   Recherche en Prevision Numerique, 2024
//
#include <string.h>

#include <rmn/tracked_malloc.h>

#define VALID_BLOCK_SIGNATURE 0xCAD0BEBE1AD0FADA

typedef struct{
  uint8_t *first ;
  uint8_t *limit ;
  size_t   size ;
  uint64_t sign ;   // normally VALID_BLOCK_SIGNATURE for a valid block
} tracked_header ;

typedef struct{
  tracked_header th ;
  uint8_t data[] ;
} tracked_block ;

// check that a tracked memory block is valid
// p   [IN] : pointer returned by tracked_block_alloc
// return 0 if block is valid, error code otherwise
int tracked_block_check(void *p){
  tracked_header *head, *tail ;
  tracked_block *tb ;
  if(p == NULL) return 1 ;
  head = (tracked_header *) p ;
  head-- ;                                                         // header below block
  if(head->sign != VALID_BLOCK_SIGNATURE) return 2 ;               // bad signature

  tb = (tracked_block *) head ;                                    // point block to start of head
  tail = (tracked_header *) head->limit ;                          // header above block
  if(memcmp(head, tail, sizeof(tracked_header)) != 0) return 3 ;   // non identical header blocks
  if(head->first != tb->data) return 4 ;                           // bad first
  if(head->limit != head->first + head->size)return 5 ;            // bad limit
  return 0 ;
}

// create a new tracked memory block
// size [IN] : size of block in bytes (same as malloc)
// return pointer to user data (not pointer to block)
void *tracked_block_alloc(size_t size){
  tracked_header *tail ;
  tracked_block *tb = (tracked_block *) malloc(size + 2 * sizeof(tracked_header)) ;
  if(tb == NULL) return NULL ;

  tb->th.first = tb->data ;                  // set header below block
  tb->th.limit = tb->th.first + size ;
  tb->th.size  = size ;
  tb->th.sign  = VALID_BLOCK_SIGNATURE ;

  tail = (tracked_header *) tb->th.limit ;   // set header above block, identical to header below block
  tail->first = tb->th.first ;
  tail->limit = tb->th.limit ;
  tail->size  = tb->th.size ;
  tail->sign  = tb->th.sign ;
  return tb->th.first ;
}

// resize a tracked memory block
// p       [IN] : pointer returned by tracked_block_alloc
// size    [IN] : size of block in bytes (same as malloc)
// options [IN] : TRACKED_BLOCK_KEEP_DATA : keep existing data
// return pointer to user data (not pointer to block) (NULL if error)
// if block is already large enough, return old pointer
void *tracked_block_resize(void *p, size_t size, int options){
  tracked_header *head, *tail ;
  tracked_block *tb ;
  if(tracked_block_check(p) != 0) return NULL ;   // invalid block
  head = (tracked_header *) p ;
  head-- ;
  if(head->size >= size) return p ;               // block is large enough

  if(options == 0){                               // new fresh block
    free(head) ;                                  // free the old block
    return tracked_block_alloc(size) ;

  }else if(options & TRACKED_BLOCK_KEEP_DATA){    // enlarge but keep data in memory block
    tb = (tracked_block *) head ;
    tb = (tracked_block *) realloc(tb, size + 2 * sizeof(tracked_header)) ;
    tail = (tracked_header *) tb->th.limit ;      // set header above block
    tail->first = tb->th.first ;
    tail->limit = tb->th.limit ;
    tail->size  = tb->th.size ;
    tail->sign  = tb->th.sign ;
    return tb->th.first ;
  }

  return NULL ;                                   // should never get here
}

// free a tracked memory block allocated with tracked_block_alloc
// p [IN] : pointer returned by tracked_block_alloc
// return 0 if O.K., error code if an error occurred
int tracked_block_free(void *p){
  tracked_header *head ;

  if(tracked_block_check(p)) return 1 ;       // invalid tracked block

  head = (tracked_header *) p ;
  head-- ;                                    // header below block == start of tracked_block
  free(head) ;                                // free the block

  return 0 ;
}
