/*****************************************************************************
 *  CP2K: A general program to perform molecular dynamics simulations        *
 *  Copyright (C) 2000 - 2015  CP2K developers group                         *
 *****************************************************************************/

//! **************************************************************************
//!> \author Hans Pabst (Intel Corp.)
//! **************************************************************************

#if defined(__LIBXSMM) || (defined(__ACC) && defined(__ACC_MIC) && defined(__DBCSR_ACC) && defined(__LIBXSTREAM))
#include "libxsmm_acc.hpp"

#if defined(__ACC) && defined(__ACC_MIC) && defined(__DBCSR_ACC) && defined(__LIBXSTREAM)
# include <libxstream_begin.h>
#endif
#include <iostream>
#include <cstdlib>
#if defined(__MKL) || defined(MKL_DIRECT_CALL_SEQ) || defined(MKL_DIRECT_CALL)
# include <mkl_service.h>
#endif
#if defined(__ACC) && defined(__ACC_MIC) && defined(__DBCSR_ACC) && defined(__LIBXSTREAM)
# include <libxstream_end.h>
#endif


LIBXSMM_ACC_EXTERN_C void xsmm_acc_abort(const char* filename, int line_number, const char* message)
{
  if (filename && *filename) {
    std::cerr << filename << ':' << line_number << " - " << ((message && *message) ? message : "unknown error") << std::endl/*includes flush*/;
  }
  exit(-1);
}


#if defined(__RECONFIGURE)
LIBXSMM_ACC_EXTERN_C void LIBXSMM_ACC_FSYMBOL(__real_dbcsr_config_mp_dbcsr_set_conf_mm_driver)(const int*);
LIBXSMM_ACC_EXTERN_C void LIBXSMM_ACC_FSYMBOL(dbcsr_config_mp_dbcsr_set_conf_mm_stacksize)(const int*);
extern int LIBXSMM_ACC_FSYMBOL(dbcsr_config_mp_multrec_limit);
extern int LIBXSMM_ACC_FSYMBOL(dbcsr_config_mp_comm_thread_load);

#if defined(__ACC) && defined(__ACC_MIC) && defined(__DBCSR_ACC) && defined(__LIBXSTREAM)
extern int LIBXSMM_ACC_FSYMBOL(dbcsr_config_mp_accdrv_posterior_streams);
extern int LIBXSMM_ACC_FSYMBOL(dbcsr_config_mp_accdrv_posterior_buffers);
extern int LIBXSMM_ACC_FSYMBOL(dbcsr_config_mp_accdrv_priority_streams);
extern int LIBXSMM_ACC_FSYMBOL(dbcsr_config_mp_accdrv_priority_buffers);
extern int LIBXSMM_ACC_FSYMBOL(dbcsr_config_mp_accdrv_min_flop_process);
#endif


LIBXSMM_ACC_EXTERN_C void LIBXSMM_ACC_FSYMBOL(__wrap_dbcsr_config_mp_dbcsr_set_conf_mm_driver)(const int* driver)
{
  // make sure to reconfigure *after* the original configuration procedure ran
  LIBXSMM_ACC_FSYMBOL(__real_dbcsr_config_mp_dbcsr_set_conf_mm_driver)(driver);

  static const char *const env = getenv("LIBXSMM_ACC_RECONFIGURE");
  static const libxsmm_acc_bool_type reconfigure = (env && *env)
    ? (0 != atoi(env))
#if defined(LIBXSMM_ACC_OFFLOAD_BUILD)
    : true;
#else
    : false;
#endif

#if defined(MKL_ENABLE_AVX512_MIC)
  mkl_enable_instructions(MKL_ENABLE_AVX512_MIC);
#endif
#if defined(__LIBXSMM)
  // pre-generate dispatch tables for the static code
  libxsmm_init();
#endif

  if (reconfigure) {
#if 0 < (LIBXSMM_ACC_STACKSIZE)
    const int stacksize = LIBXSMM_ACC_STACKSIZE;
    LIBXSMM_ACC_FSYMBOL(dbcsr_config_mp_dbcsr_set_conf_mm_stacksize)(&stacksize);
#endif
#if 0 < (LIBXSMM_ACC_MULTREC_LIMIT)
    LIBXSMM_ACC_FSYMBOL(dbcsr_config_mp_multrec_limit) = LIBXSMM_ACC_MULTREC_LIMIT;
#endif
#if 0 < (LIBXSMM_ACC_COMM_THREAD_LOAD)
    LIBXSMM_ACC_FSYMBOL(dbcsr_config_mp_comm_thread_load) = LIBXSMM_ACC_COMM_THREAD_LOAD;
#endif
#if defined(__ACC) && defined(__ACC_MIC) && defined(__DBCSR_ACC) && defined(__LIBXSTREAM)
# if 0 < (LIBXSMM_ACC_ACCDRV_POSTERIOR_STREAMS)
    LIBXSMM_ACC_FSYMBOL(dbcsr_config_mp_accdrv_posterior_streams) = LIBXSMM_ACC_ACCDRV_POSTERIOR_STREAMS;
# endif
# if 0 < (LIBXSMM_ACC_ACCDRV_POSTERIOR_BUFFERS)
    LIBXSMM_ACC_FSYMBOL(dbcsr_config_mp_accdrv_posterior_buffers) = LIBXSMM_ACC_ACCDRV_POSTERIOR_BUFFERS;
# endif
# if 0 < (LIBXSMM_ACC_ACCDRV_PRIORITY_STREAMS)
    LIBXSMM_ACC_FSYMBOL(dbcsr_config_mp_accdrv_priority_streams) = LIBXSMM_ACC_ACCDRV_PRIORITY_STREAMS;
# endif
# if 0 < (LIBXSMM_ACC_ACCDRV_PRIORITY_BUFFERS)
    LIBXSMM_ACC_FSYMBOL(dbcsr_config_mp_accdrv_priority_buffers) = LIBXSMM_ACC_ACCDRV_PRIORITY_BUFFERS;
# endif
# if 0 < (LIBXSMM_ACC_ACCDRV_MIN_NFLOPS_PERMM)
    LIBXSMM_ACC_FSYMBOL(dbcsr_config_mp_accdrv_min_flop_process) = LIBXSMM_ACC_ACCDRV_MIN_NFLOPS_PERMM;
# endif
#endif
  }
}
#endif // defined(__RECONFIGURE)


#if defined(__LIBXSMM)
namespace libxsmm_acc_private {

template<typename T>
void xsmm_process_mm_stack(const libxsmm_acc_stackdesc_type* descriptor, const int* params, const int* stacksize, const T* a, const T* b, T* c)
{
  int result = LIBXSMM_ACC_ERROR_CONDITION;

  if (0 != descriptor && 0 != params && 0 != stacksize && 0 != a && 0 != b && 0 != c) {
    result = libsmm_acc_process( // TODO: fix const-correctness in libsmm_acc.h
      const_cast<int*>(params), *stacksize, LIBXSMM_ACC_NPARAMS, libxsmm_acc_elem<T,false>::type, const_cast<T*>(a), const_cast<T*>(b), c,
      descriptor->max_m, descriptor->max_n, descriptor->max_k, descriptor->defined_mnk, 0/*stream*/);
  }

  switch (result) {
    case LIBXSMM_ACC_ERROR_CONDITION: LIBXSMM_ACC_ABORT("incorrect argument(s)"); break;
    default: if (LIBXSMM_ACC_ERROR_NONE != result) LIBXSMM_ACC_ABORT("unknown error");
  }
}

} // namespace libxsmm_acc_private


LIBXSMM_ACC_EXTERN_C void LIBXSMM_ACC_FSYMBOL(xsmm_process_mm_stack_s)(const libxsmm_acc_stackdesc_type* descriptor,
  const int* params, const int* stacksize, const float* a, const float* b, float* c)
{
  libxsmm_acc_private::xsmm_process_mm_stack(descriptor, params, stacksize, a, b, c);
}


LIBXSMM_ACC_EXTERN_C void LIBXSMM_ACC_FSYMBOL(xsmm_process_mm_stack_d)(const libxsmm_acc_stackdesc_type* descriptor,
  const int* params, const int* stacksize, const double* a, const double* b, double* c)
{
  libxsmm_acc_private::xsmm_process_mm_stack(descriptor, params, stacksize, a, b, c);
}
#endif // defined(__LIBXSMM)

#endif // defined(__LIBXSMM) || (defined(__ACC) && defined(__ACC_MIC) && defined(__DBCSR_ACC) && defined(__LIBXSTREAM))