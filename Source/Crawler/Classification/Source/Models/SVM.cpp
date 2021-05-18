#include "Classification/Export/Models/SVM.h"
#include "CoreTypes/Export/Pair.h"

#include "../../3rdParty/Shark/Export/SharkSVM.h"

// =================================================================================================

typedef shark::AbstractKernelFunction<shark::RealVector> TSvmKernelType;
typedef shark::KernelClassifier<shark::RealVector> TSvmModelType;

// =================================================================================================

// -------------------------------------------------------------------------------------------------

static TSvmKernelType* SCreateKernel(double gamma)
{
  // return new shark::LinearKernel<shark::RealVector>();
  return new shark::GaussianRbfKernel<shark::RealVector>(gamma);
  // return new shark::MonomialKernel<shark::RealVector>(2);
  // return new shark::PolynomialKernel<shark::RealVector>(1);
}

// -------------------------------------------------------------------------------------------------

static TSvmModelType* SCreateModel(TSvmKernelType* pKernel)
{
  return new TSvmModelType(pKernel);
}

// -------------------------------------------------------------------------------------------------

static TSvmModelType* STrainModel(
  const shark::ClassificationDataset&   TrainData,
  TSvmKernelType*                       pKernel,
  double                                C)
{
  const bool trainOffset = true;
  const bool unconstrained = false;
  shark::CSvmTrainer<shark::RealVector> Trainer(
    pKernel, C, trainOffset, unconstrained);
#if 1 // works a little bit better with spectrum & features
  Trainer.setMcSvmType(shark::McSvm::OVA);
#else
  Trainer.setMcSvmType(shark::McSvm::CS);
#endif

  // Trainer.setMaxIterations(200000);
  
  TOwnerPtr< TSvmModelType > pModel(SCreateModel(pKernel));
  Trainer.train(*pModel, TrainData);

  return pModel.ReleaseOwnership();
}

// -------------------------------------------------------------------------------------------------

static float SEvaluateModel(
  TSvmModelType*                      pModel,
  TSvmKernelType*                     pKernel,
  const shark::ClassificationDataset& TestData,
  double                              gamma,
  double                              C)
{
  shark::Data<unsigned int> Prediction = (*pModel)(TestData.inputs());

  shark::ZeroOneLoss<unsigned int> Loss;
  float error_rate = (float)Loss(TestData.labels(), Prediction);

  gTraceVar("  Error: %.2f (Gamma: %g C: %g)",
    error_rate, (float)gamma, (float)C);

  return error_rate;
}

// -------------------------------------------------------------------------------------------------

/*static*/ TPair<TSvmKernelType*, TSvmModelType*> SEvaluateBestModel(
  const shark::ClassificationDataset& TrainData,
  const shark::ClassificationDataset& TestData)
{
  double bestGamma = -1.0;
  double bestC = -1.0;
  float bestErrorRate = 1.0;

  TOwnerPtr< TSvmKernelType > pBestKernel;
  TOwnerPtr< shark::KernelClassifier<shark::RealVector > > pBestModel;

  shark::JaakkolaHeuristic ja(TrainData);
  gTraceVar("  Tommi Jaakkola says: best gamma = %g", ja.gamma());

  // 1st course stage
  for (double gamma = 0.01; gamma <= 10; gamma *= 10.0)
  {
    pBestKernel = TOwnerPtr< TSvmKernelType >(
      SCreateKernel(gamma));

    for (double C = 0.01; C <= 100.0; C *= 10.0)
    {
      TOwnerPtr< TSvmKernelType > pKernel(SCreateKernel(gamma));
      TOwnerPtr< TSvmModelType > pModel(STrainModel(TrainData, pKernel, C));

      const float Error = SEvaluateModel(pModel, pKernel, TestData, gamma, C);

      if (Error < bestErrorRate)
      {
        pBestModel = pModel;
        pBestKernel = pKernel;

        bestErrorRate = Error;
        bestGamma = gamma;
        bestC = C;
      }
    }
  }

  gTraceVar("  1st stage: Gamma: %g C: %g (Error: %.2f)",
    (float)bestGamma, (float)bestC, (float)bestErrorRate);

  // 2nd finetuning stage
  const double minGamma = bestGamma / 10, maxGamma = bestGamma * 10;
  const double minC = bestC / 10, maxC = bestC * 10;

  for (double gamma = minGamma; gamma < maxGamma; gamma *= 2.0)
  {
    for (double C = minC; C <= maxC; C *= 2.0)
    {
      TOwnerPtr< TSvmKernelType > pKernel(SCreateKernel(gamma));
      TOwnerPtr< TSvmModelType > pModel(STrainModel(TrainData, pKernel, C));

      const float Error = SEvaluateModel(pModel, pKernel, TestData, gamma, C);

      if (Error < bestErrorRate)
      {
        pBestModel = pModel;
        pBestKernel = pKernel;

        bestErrorRate = Error;
        bestGamma = gamma;
        bestC = C;
      }
    }
  }

  gTraceVar("  2nd stage: Gamma: %g C: %g (Error: %.2f)",
    (float)bestGamma, (float)bestC, (float)bestErrorRate);

  return MakePair(pBestKernel.ReleaseOwnership(), pBestModel.ReleaseOwnership());
}

// =================================================================================================

static const double sBestGamma = 0.001;
static const double sBestC = 4;

// -------------------------------------------------------------------------------------------------

TSvmClassificationModel::TSvmClassificationModel()
{
  // nothing to do
}

// -------------------------------------------------------------------------------------------------

TSvmClassificationModel::~TSvmClassificationModel()
{
  // avoid getting inlined
}

// -------------------------------------------------------------------------------------------------

TString TSvmClassificationModel::OnName() const
{
  return "SVM";
}

// -------------------------------------------------------------------------------------------------

TSvmClassificationModel::TSharkModelType*
  TSvmClassificationModel::OnCreateNewModel(
    const TPoint&                       InputFeaturesSize,
    int                                 NumberOfClasses)
{
  mpKernel = TOwnerPtr< TSvmKernelType >(SCreateKernel(sBestGamma));
  return SCreateModel(mpKernel);
}

// -------------------------------------------------------------------------------------------------

TSvmClassificationModel::TSharkModelType*
  TSvmClassificationModel::OnCreateTrainedModel(
    const shark::ClassificationDataset& TrainData,
    const shark::ClassificationDataset& TestData,
    const TString&                      DataSetFileName,
    const TPoint&                       InputFeaturesSize,
    int                                 NumberOfClasses)
{
#if 0 // Evaluate best parameters
  TPair<TSvmKernelType*, TSvmModelType*> Result = SEvaluateBestModel(
    TrainData, TestData);

  mpKernel = TOwnerPtr<TSvmKernelType>(Result.First());
  return Result.Second();
#else

  mpKernel = TOwnerPtr<TSvmKernelType>(SCreateKernel(sBestGamma));
  return STrainModel(TrainData, mpKernel, sBestC);
#endif
}

