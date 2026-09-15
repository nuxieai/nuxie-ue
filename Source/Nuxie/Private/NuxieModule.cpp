#include "NuxieModule.h"
#include "NuxieNativeTransport.h"

#include "Modules/ModuleManager.h"

IMPLEMENT_MODULE(FNuxieModule, Nuxie)

void FNuxieModule::StartupModule()
{
}

void FNuxieModule::ShutdownModule()
{
#if PLATFORM_ANDROID
  StopNuxieAndroidDispatcher();
#endif
}
