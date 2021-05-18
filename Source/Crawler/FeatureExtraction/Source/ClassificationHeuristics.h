#pragma once

#ifndef _ClassificationHeuristics_h_
#define _ClassificationHeuristics_h_

class TSampleDescriptors;

// =================================================================================================

/*!
 * Heuristics to detect sample classifiers without ML models.
 *
 * NB: The heuristics only checks for a few "simple" characteristics, so they 
 * can not replace a classifier, but can only be used to override the output 
 * of some classifier in order to correct it in some cases.
 */

namespace TClassificationHeuristics
{
  // Detect if the given sample (represented via low level descriptors) is 
  // very likely a OneShot sound (and NOT a loop)
  // @param Confidence is the results confidence [0-1] 0=not, 1=very certain
  // @returns true when very certain (confidence > 0.7).
  bool IsOneShot(
    const TSampleDescriptors& LowLevelDescriptors,
    double&                   Confidence);

  // Detect if the given sample (represented via low level descriptors) is 
  // very likely a Loop sound (and NOT a loop)
  // @param Confidence is the results confidence [0-1] 0=not, 1=very certain
  // @returns true when very certain (confidence > 0.7).
  bool IsLoop(
    const TSampleDescriptors& LowLevelDescriptors,
    double&                   Confidence);
};


#endif // _ClassificationHeuristics_h_

