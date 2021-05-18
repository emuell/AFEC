#include "CoreTypes/Export/Debug.h"
#include "CoreTypes/Export/Directory.h"
#include "CoreTypes/Export/TestHelpers.h"

#include "Classification/Test/TestShark.h"

#include "../../3rdParty/Shark/Export/SharkSVM.h"

// =================================================================================================

// -------------------------------------------------------------------------------------------------

void TClassificationTest::Shark()
{
  M__DisableFloatingPointAssertions

  // ... single class SVM

  try
  {
    const TDirectory DataDir(gDeveloperProjectsDir().Descend("Crawler").
      Descend("Classification").Descend("Test").Descend("Data"));

    shark::ClassificationDataset TrainData, TestData;
    shark::importCSV(TrainData,
      (DataDir.Path() + "SharkClassificationData.csv").CString(TString::kFileSystemEncoding),
      shark::LAST_COLUMN, ' ');

    // Use 70% for training and 30% for testing.
    TestData = shark::splitAtElement(TrainData, 70 * TrainData.numberOfElements() / 100);

    const double gamma = 1.0;         // kernel bandwidth parameter
    const double C = 10.0;            // regularization parameter
    const bool withOffset = true;
    shark::GaussianRbfKernel<shark::RealVector> kernel(gamma);
    shark::KernelClassifier<shark::RealVector> Model(&kernel);
    shark::CSvmTrainer<shark::RealVector> Trainer(
      &kernel,
      C,
      withOffset);

    Trainer.train(Model, TrainData);

    shark::Data<unsigned int> Prediction = Model(TestData.inputs());

    shark::ZeroOneLoss<unsigned int> Loss;
    double error_rate = Loss(TestData.labels(), Prediction);

    BOOST_CHECK(error_rate < 0.3);
  }
  catch (const shark::Exception& exception)
  {
    BOOST_ERROR(exception.what());
  }


  // ... multi-class SVM

  try
  {
    const TDirectory DataDir(gDeveloperProjectsDir().Descend("Crawler").
      Descend("Classification").Descend("Test").Descend("Data"));

    shark::ClassificationDataset TrainData, TestData;
    shark::importCSV(TrainData,
      (DataDir.Path() + "SharkMultiClassificationData.csv").CString(TString::kFileSystemEncoding),
      shark::LAST_COLUMN, ',');

    // normalize data
    bool removeMean = true;
    shark::Normalizer<shark::RealVector> normalizer;
    shark::NormalizeComponentsUnitVariance<shark::RealVector> normalizingTrainer(removeMean);
    normalizingTrainer.train(normalizer, TrainData.inputs());
    TrainData = shark::transformInputs(TrainData, normalizer);

    // whiten data
    // shark::LinearModel<shark::RealVector> whitener;
    // shark::NormalizeComponentsWhitening whiteningTrainer;
    // whiteningTrainer.train(whitener, TrainData.inputs());
    // TrainData = shark::transformInputs(TrainData, whitener);

    // shuffle data
    TrainData.shuffle();

    // Use 70% for training and 30% for testing.
    TestData = shark::splitAtElement(TrainData, 70 * TrainData.numberOfElements() / 100);

    const double gamma = 0.02; // kernel bandwidth parameter
    const double C = 10.0; // regularization parameter
    const bool withOffset = true;
    shark::GaussianRbfKernel<shark::RealVector> kernel(gamma);
    shark::KernelClassifier<shark::RealVector> Model(&kernel);
    shark::CSvmTrainer<shark::RealVector> Trainer(
      &kernel,
      C,
      withOffset);
    Trainer.setMcSvmType( shark::McSvm::OVA );

    // Trainer.setMaxIterations(200000);
    Trainer.train(Model, TrainData);

    shark::Data<unsigned int> Prediction = Model(TestData.inputs());

    shark::ZeroOneLoss<unsigned int> Loss;
    double error_rate = Loss(TestData.labels(), Prediction);

    BOOST_CHECK(error_rate < 0.3);
  }
  catch (const shark::Exception& exception)
  {
    BOOST_ERROR(exception.what());
  }

  M__EnableFloatingPointAssertions
}

