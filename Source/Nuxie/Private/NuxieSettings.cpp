#include "NuxieSettings.h"
FNuxieOptions UNuxieSettings::GetProjectOptions() {
  const auto* Settings = GetDefault<UNuxieSettings>(); FNuxieOptions Options;
  Options.IOSPublicKey = Settings->IOSPublicKey; Options.AndroidPublicKey = Settings->AndroidPublicKey;
  Options.Environment = Settings->Environment; Options.LogLevel = Settings->LogLevel;
  return Options;
}
