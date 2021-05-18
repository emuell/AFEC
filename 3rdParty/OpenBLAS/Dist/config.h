#define OS_WINNT	1

#if defined _WIN64 
  #define ARCH_X86_64	1
  #define C_MSVC	1
  #define __64BIT__	1
#else
  #define ARCH_X86	1
  #define C_MSVC	1
  #define __32BIT__	1
#endif

#define FUNDERSCORE	_
#define BUNDERSCORE _
#define NEEDBUNDERSCORE 1
#define NEED2UNDERSCORES 0
#define GENERIC
#define L1_DATA_SIZE 32768
#define L1_DATA_LINESIZE 128
#define L2_SIZE 512488
#define L2_LINESIZE 128
#define DTB_DEFAULT_ENTRIES 128
#define DTB_SIZE 4096
#define L2_ASSOCIATIVE 8
#define CORE_generic

#define CHAR_CORENAME "generic"
#define SLOCAL_BUFFER_SIZE	4096
#define DLOCAL_BUFFER_SIZE	4096
#define CLOCAL_BUFFER_SIZE	8192
#define ZLOCAL_BUFFER_SIZE	8192
#define GEMM_MULTITHREAD_THRESHOLD	4
