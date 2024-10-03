
set(PROJECT_C_FILES 
  src/id/identify_compiler.c
  src/id/identify_mpi_child.c
  src/timers/rmn_timers.c
  src/pred/lorenzo_c.c
  src/pred/smooth124.c
  src/pred/average_2x2.c
  src/pack/bits.c
  src/pack/bi_endian_pack.c
  src/pack/data_map.c
  src/pack/misc_pack.c
  src/pack/stream_pack.c
#   src/pack/copy_swap.c
  src/pack/misc_operators.c
  src/pack/ieee_quantize.c
  src/pack/move_blocks.c
  src/pack/compress_data.c
  src/pack/tile_encoders.c
  src/pack/serialized_functions.c
  src/pack/pixmaps.c
  src/pack/compress_expand.c
  src/pack/pipe_filters.c
  src/pack/filter_000.c
  src/pack/filter_001.c
  src/pack/filter_100.c
  src/pack/filter_110.c
  src/pack/filter_254.c
  src/pack/filter_255.c
  src/pack/tracked_malloc.c
  src/pack/float_block_encoders.c
  src/diag/tee_print.c
  src/diag/test_helpers.c
  src/diag/print_helpers.c
  src/diag/misc_analyze.c
  src/diag/data_info.c
  src/diag/c_record_io.c
  src/diag/eval_diff.c
  src/diag/entropy.c
  src/diag/sparse_concat.c
  src/diag/simd_compare.c
  src/plugins/plugin_code.c
)

set(PROJECT_PLUGIN_FILES 
  src/pred/lorenzo_c.c
  src/pack/bi_endian_pack.c
)

set(PROJECT_F_FILES
  src/timers/rmn_timers_mod.F90
  src/pred/lorenzo_mod.F90
  src/pred/analyze_data.F90
  src/plugins/fortran_plugins.F90
  src/pred/misc_operators_mod.F90
  src/diag/data_info_mod.F90
  src/diag/readlx_new.F90
)

set(PROJECT_H_FILES
  src/PUBLIC_INCLUDES/rmntools.h
  src/PUBLIC_INCLUDES/rmn/atomic_functions.h
  src/PUBLIC_INCLUDES/rmn/ct_assert.h
  src/PUBLIC_INCLUDES/rmn/function_pointers.h
  src/PUBLIC_INCLUDES/rmn/lorenzo.h
  src/PUBLIC_INCLUDES/rmn/smooth124.h
  src/PUBLIC_INCLUDES/rmn/rmn_tools.h
  src/PUBLIC_INCLUDES/rmn/identify_compiler_f.hf
  src/PUBLIC_INCLUDES/rmn/timers.h
  src/PUBLIC_INCLUDES/rmn/tools_types.h
  src/PUBLIC_INCLUDES/rmn/pack_macros.h
  src/PUBLIC_INCLUDES/rmn/bi_endian_pack.h
  src/PUBLIC_INCLUDES/rmn/data_map.h
  src/PUBLIC_INCLUDES/rmn/word_stream.h
  src/PUBLIC_INCLUDES/rmn/bit_stream.h
  src/PUBLIC_INCLUDES/rmn/stream_pack.h
  src/PUBLIC_INCLUDES/rmn/bit_pack_macros.h
  src/PUBLIC_INCLUDES/rmn/print_bitstream.h
  src/PUBLIC_INCLUDES/rmn/bits.h
  src/PUBLIC_INCLUDES/rmn/pixmaps.h
  src/PUBLIC_INCLUDES/rmn/compress_expand.h
  src/PUBLIC_INCLUDES/rmn/tee_print.h
  src/PUBLIC_INCLUDES/rmn/test_helpers.h
  src/PUBLIC_INCLUDES/rmn/identify_compiler_c.h
  src/PUBLIC_INCLUDES/rmn/identify_mpi_child.h
  src/PUBLIC_INCLUDES/rmn/is_fortran_compiler.h
  src/PUBLIC_INCLUDES/rmn/tools_plugins.h
  src/PUBLIC_INCLUDES/rmn/tools_plugins.hf
  src/PUBLIC_INCLUDES/rmn/sparse_concat.h
  src/PUBLIC_INCLUDES/rmn/simd_compare.h
  src/PUBLIC_INCLUDES/rmn/move_blocks.h
#   src/PUBLIC_INCLUDES/rmn/copy_swap.h
  src/PUBLIC_INCLUDES/rmn/misc_pack.h
  src/PUBLIC_INCLUDES/rmn/misc_pack.hf
  src/PUBLIC_INCLUDES/rmn/compress_data.h
  src/PUBLIC_INCLUDES/rmn/tile_encoders.h
  src/PUBLIC_INCLUDES/rmn/misc_analyze.h
  src/PUBLIC_INCLUDES/rmn/ieee_quantize.h
  src/PUBLIC_INCLUDES/rmn/c_record_ioe.h
  src/PUBLIC_INCLUDES/rmn/serialized_functions.h
  src/PUBLIC_INCLUDES/rmn/c_binding_extras.hf
  src/PUBLIC_INCLUDES/rmn/misc_operators.h
  src/PUBLIC_INCLUDES/rmn/data_info.h
  src/PUBLIC_INCLUDES/rmn/eval_diff.h
  src/PUBLIC_INCLUDES/rmn/entropy.h
  src/PUBLIC_INCLUDES/rmn/x86-simd.h
  src/PUBLIC_INCLUDES/rmn/fastapprox.h
  src/PUBLIC_INCLUDES/rmn/pipe_filters.h
  src/PUBLIC_INCLUDES/rmn/filter_base.h
  src/PUBLIC_INCLUDES/rmn/filter_all.h
  src/PUBLIC_INCLUDES/rmn/filter_000.h
  src/PUBLIC_INCLUDES/rmn/filter_001.h
  src/PUBLIC_INCLUDES/rmn/filter_100.h
  src/PUBLIC_INCLUDES/rmn/filter_110.h
  src/PUBLIC_INCLUDES/rmn/filter_254.h
  src/PUBLIC_INCLUDES/rmn/filter_255.h
  src/PUBLIC_INCLUDES/rmn/tracked_malloc.h
  src/PUBLIC_INCLUDES/rmn/simd_functions.h
)
