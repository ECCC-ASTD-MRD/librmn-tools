
set(PROJECT_C_FILES 
  src/pred/average_2x2.c
  src/pack/array_nd.c
#   src/pack/bi_endian_pack.c
  src/pack/bits.c
#   src/pack/bitstream.c
#   src/pack/bitstream_pack.c
#   src/diag/c_record_io.c
  src/diag/compare_count.c
  src/pack/compress_data.c
  src/pack/compress_expand.c
  src/diag/data_info.c
  src/pack/data_map.c
  src/pack/dmap_filters.c
  src/pack/dmap_filters_000_007.c
  src/pred/dwt_i_lgt53.c
  src/diag/entropy.c
  src/diag/eval_compress.c
  src/diag/eval_diff.c
  src/pack/filter_000.c
  src/pack/filter_001.c
  src/pack/filter_100.c
  src/pack/filter_110.c
  src/pack/filter_254.c
  src/pack/filter_255.c
  src/pack/float_block_encoders.c
  src/id/identify_compiler.c
  src/id/identify_mpi_child.c
#   src/pack/ieee_quantize.c
  src/pred/lorenzo_c.c
  src/diag/misc_analyze.c
  src/pack/misc_operators.c
  src/pack/misc_pack.c
  src/pack/move_blocks.c
  src/pack/pipe_filters.c
  src/pack/pixmaps.c
  src/plugins/plugin_code.c
  src/diag/print_helpers.c
  src/pack/quantizers.c
  src/timers/rmn_timers.c
  src/pack/serialized_functions.c
  src/diag/simd_compare.c
  src/pred/smooth124.c
  src/diag/sparse_concat.c
  src/pack/stream_pack.c
  src/diag/tee_print.c
  src/diag/test_helpers.c
  src/pack/tile_encoders.c
  src/pack/tracked_malloc.c
)

set(PROJECT_PLUGIN_FILES 
#   src/pack/bi_endian_pack.c
  src/pred/lorenzo_c.c
)

set(PROJECT_F_FILES
  src/pred/analyze_data.F90
  src/diag/data_info_mod.F90
  src/plugins/fortran_plugins.F90
  src/pred/lorenzo_mod.F90
  src/pred/misc_operators_mod.F90
  src/diag/readlx_new.F90
  src/timers/rmn_timers_mod.F90
)

set(PROJECT_H_FILES
  src/PUBLIC_INCLUDES/rmntools.h
  src/PUBLIC_INCLUDES/rmn/array_nd.h
  src/PUBLIC_INCLUDES/rmn/atomic_functions.h
  src/PUBLIC_INCLUDES/rmn/be_stream.h
#   src/PUBLIC_INCLUDES/rmn/bi_endian_pack.h
#   src/PUBLIC_INCLUDES/rmn/bit_pack_macros.h
#   src/PUBLIC_INCLUDES/rmn/bit_stream.h
  src/PUBLIC_INCLUDES/rmn/bits.h
  src/PUBLIC_INCLUDES/rmn/bitstream.h
  src/PUBLIC_INCLUDES/rmn/c_binding_extras.hf
#   src/PUBLIC_INCLUDES/rmn/c_record_io.h
  src/PUBLIC_INCLUDES/rmn/common_stream.h
  src/PUBLIC_INCLUDES/rmn/compare_count.h
#   src/PUBLIC_INCLUDES/rmn/compress_data.h
  src/PUBLIC_INCLUDES/rmn/compress_expand.h
#   src/PUBLIC_INCLUDES/rmn/cpp_concat.h
#   src/PUBLIC_INCLUDES/rmn/cpp_extras.h
#   src/PUBLIC_INCLUDES/rmn/cpp_loop.h
#   src/PUBLIC_INCLUDES/rmn/ct_assert.h
  src/PUBLIC_INCLUDES/rmn/data_info.h
  src/PUBLIC_INCLUDES/rmn/data_kind.h
  src/PUBLIC_INCLUDES/rmn/data_map.h
  src/PUBLIC_INCLUDES/rmn/data_properties.h
  src/PUBLIC_INCLUDES/rmn/dmap_filters.h
  src/PUBLIC_INCLUDES/rmn/dmap_filters_000_007.h
  src/PUBLIC_INCLUDES/rmn/dmap_filters_010_017.h
  src/PUBLIC_INCLUDES/rmn/dmap_filters_020_027.h
  src/PUBLIC_INCLUDES/rmn/dmap_filters_030_037.h
  src/PUBLIC_INCLUDES/rmn/dwt_i_lgt53.h
  src/PUBLIC_INCLUDES/rmn/entropy.h
  src/PUBLIC_INCLUDES/rmn/eval_compress.h
  src/PUBLIC_INCLUDES/rmn/eval_diff.h
  src/PUBLIC_INCLUDES/rmn/fastapprox.h
  src/PUBLIC_INCLUDES/rmn/filter_all.h
  src/PUBLIC_INCLUDES/rmn/filter_base.h
  src/PUBLIC_INCLUDES/rmn/filter_000.h
  src/PUBLIC_INCLUDES/rmn/filter_001.h
  src/PUBLIC_INCLUDES/rmn/filter_100.h
  src/PUBLIC_INCLUDES/rmn/filter_110.h
  src/PUBLIC_INCLUDES/rmn/filter_254.h
  src/PUBLIC_INCLUDES/rmn/filter_255.h
  src/PUBLIC_INCLUDES/rmn/float_block_encoders.h
  src/PUBLIC_INCLUDES/rmn/function_pointers.h
  src/PUBLIC_INCLUDES/rmn/identify_c_compiler.h
  src/PUBLIC_INCLUDES/rmn/identify_fortran_compiler.hf
  src/PUBLIC_INCLUDES/rmn/identify_mpi_child.h
  src/PUBLIC_INCLUDES/rmn/ieee_functions.h
  src/PUBLIC_INCLUDES/rmn/ieee_quantize.h
  src/PUBLIC_INCLUDES/rmn/is_fortran_compiler.h
  src/PUBLIC_INCLUDES/rmn/le_stream.h
  src/PUBLIC_INCLUDES/rmn/lorenzo.h
  src/PUBLIC_INCLUDES/rmn/misc_analyze.h
  src/PUBLIC_INCLUDES/rmn/misc_pack.h
  src/PUBLIC_INCLUDES/rmn/misc_pack.hf
  src/PUBLIC_INCLUDES/rmn/misc_operators.h
  src/PUBLIC_INCLUDES/rmn/move_blocks.h
  src/PUBLIC_INCLUDES/rmn/pack_macros.h
  src/PUBLIC_INCLUDES/rmn/pipe_filters.h
  src/PUBLIC_INCLUDES/rmn/pixmaps.h
#   src/PUBLIC_INCLUDES/rmn/print_bitstream.h
  src/PUBLIC_INCLUDES/rmn/quantizers.h
  src/PUBLIC_INCLUDES/rmn/rmn_tools.h
  src/PUBLIC_INCLUDES/rmn/serialized_functions.h
  src/PUBLIC_INCLUDES/rmn/simd_compare.h
#   src/PUBLIC_INCLUDES/rmn/simd_functions.h
  src/PUBLIC_INCLUDES/rmn/smooth124.h
  src/PUBLIC_INCLUDES/rmn/sparse_concat.h
  src/PUBLIC_INCLUDES/rmn/split_dimension.h
  src/PUBLIC_INCLUDES/rmn/stream_pack.h
  src/PUBLIC_INCLUDES/rmn/timers.h
  src/PUBLIC_INCLUDES/rmn/tools_types.h
  src/PUBLIC_INCLUDES/rmn/tee_print.h
#   src/PUBLIC_INCLUDES/rmn/test_helpers.h
  src/PUBLIC_INCLUDES/rmn/tile_encoders.h
  src/PUBLIC_INCLUDES/rmn/tools_plugins.h
  src/PUBLIC_INCLUDES/rmn/tools_plugins.hf
  src/PUBLIC_INCLUDES/rmn/tracked_malloc.h
#   src/PUBLIC_INCLUDES/rmn/va_args_num.h
  src/PUBLIC_INCLUDES/rmn/word_stream.h
  src/PUBLIC_INCLUDES/rmn/x86-simd.h
)
