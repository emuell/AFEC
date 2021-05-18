#include "Classification/Export/ClassificationInit.h"

// force BLAS and Shark to be linked
#if defined(MCompiler_VisualCPP) && defined(MRelease)
  #include "../../3rdParty/Shark/Export/SharkDataSet.h"
  extern "C" void gotoblas_init(void) ;
  extern "C" void gotoblas_quit(void);
#endif

// =================================================================================================

// -------------------------------------------------------------------------------------------------

void ClassificationInit()
{
#if defined(MCompiler_VisualCPP) && defined(MRelease)
  gotoblas_init();
#endif
}

// -------------------------------------------------------------------------------------------------

void ClassificationExit()
{
  // nothing to do
}

