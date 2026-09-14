#pragma once
#include "Commandlets/Commandlet.h"
#include "NuxieCreateExamplesCommandlet.generated.h"
/** Creates real map and Blueprint assets in the selected example project. */
UCLASS()
class UNuxieCreateExamplesCommandlet : public UCommandlet {
  GENERATED_BODY()
public:
  UNuxieCreateExamplesCommandlet();
  int32 Main(const FString& Params) override;
};
