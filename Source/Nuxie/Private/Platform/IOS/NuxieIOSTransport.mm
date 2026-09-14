#include "NuxieNativeTransport.h"
#if PLATFORM_IOS
extern "C" {
  int32 NuxieUnreal_ContractVersion();
  void NuxieUnreal_Dispatch(const char* Request);
  char* NuxieUnreal_PopMessage();
  void NuxieUnreal_FreeCString(char* Value);
}
namespace {
class FNuxieIOSTransport final : public INuxieNativeTransport {
public:
  int32 ContractVersion() override { return NuxieUnreal_ContractVersion(); }
  bool Submit(const FString& Request) override {
    NuxieUnreal_Dispatch(TCHAR_TO_UTF8(*Request));
    return true;
  }
  bool Poll(FString& Message) override {
    char* Value = NuxieUnreal_PopMessage();
    if (!Value) return false;
    Message = UTF8_TO_TCHAR(Value);
    NuxieUnreal_FreeCString(Value);
    return true;
  }
};
}
TUniquePtr<INuxieNativeTransport> CreateNuxieNativeTransport() { return MakeUnique<FNuxieIOSTransport>(); }
#endif
