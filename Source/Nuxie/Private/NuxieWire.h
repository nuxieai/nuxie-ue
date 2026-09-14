#pragma once
#include "NuxieTypes.h"
#include "Dom/JsonObject.h"
namespace NuxieWire {
using FObject = TSharedPtr<FJsonObject>;
FObject Object(const FString& Json);
FString Json(const FObject& Value);
FNuxieError Error(ENuxieErrorCode Code, const FString& Message, const FString& NativeCode = FString());
FNuxieError NativeError(const FObject& Value);
bool Access(const FObject& Value, FNuxieFeatureAccess& Out);
bool Identity(const FObject& Value, FNuxieIdentity& Out);
bool Receipt(const FObject& Value, FNuxieUsageReceipt& Out);
bool Snapshot(const FObject& Value, FNuxieFeatureSnapshot& Out);
FNuxieProperties Properties(const FObject& Value);
bool Scalars(const FObject& Value, TMap<FString, FNuxieScalar>& Out);
bool Product(const FObject& Value, FNuxieStoreProduct& Out);
}
