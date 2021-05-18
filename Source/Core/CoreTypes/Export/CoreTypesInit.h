#pragma once

#ifndef _CoreTypesInit_h_
#define _CoreTypesInit_h_

// =================================================================================================

//! Usually called internally in the platform gMainEntry impls.
//! Initializes and finalizes all static initializers in the CoreTypes lib.
void CoreTypesInit();
void CoreTypesExit();


#endif // _CoreTypesInit_h_

