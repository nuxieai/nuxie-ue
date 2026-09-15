#pragma once

#include "Modules/ModuleManager.h"

class FNuxieModule final : public IModuleInterface
{
public:
#if PLATFORM_ANDROID
  // The Android looper callback has process/module lifetime.
  virtual bool SupportsDynamicReloading() override { return false; }
#endif
  virtual void StartupModule() override;
  virtual void ShutdownModule() override;
};
