#include "NuxieNativeTransport.h"
#if !PLATFORM_IOS && !PLATFORM_ANDROID
namespace {
class FNuxieUnsupportedTransport final : public INuxieNativeTransport {
public:
  int32 ContractVersion() override { return 0; }
  bool Submit(const FString&) override { return false; }
  bool Poll(FString&) override { return false; }
};
}
TUniquePtr<INuxieNativeTransport> CreateNuxieNativeTransport() { return MakeUnique<FNuxieUnsupportedTransport>(); }
#endif
