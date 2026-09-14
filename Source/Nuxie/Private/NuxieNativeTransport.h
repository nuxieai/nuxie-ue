#pragma once
#include "CoreMinimal.h"
class INuxieNativeTransport
{
public:
  virtual ~INuxieNativeTransport() = default;
  virtual int32 ContractVersion() = 0;
  virtual bool Submit(const FString& Request) = 0;
  virtual bool Poll(FString& Message) = 0;
};
TUniquePtr<INuxieNativeTransport> CreateNuxieNativeTransport();
