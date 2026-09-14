#include "NuxieNativeTransport.h"
#if PLATFORM_ANDROID
#include "Android/AndroidApplication.h"
#include "Android/AndroidJNI.h"
#include "Containers/StringConv.h"
namespace {
class FNuxieAndroidTransport final : public INuxieNativeTransport {
  jclass Bridge = nullptr;
  jmethodID Version = nullptr;
  jmethodID Dispatch = nullptr;
  jmethodID Pop = nullptr;
  static bool Failed(JNIEnv* Env) {
    if (!Env->ExceptionCheck()) return false;
    Env->ExceptionClear();
    return true;
  }
public:
  FNuxieAndroidTransport() {
    JNIEnv* Env = FAndroidApplication::GetJavaEnv();
    if (!Env) return;
    jclass Local = FAndroidApplication::FindJavaClass("ai/nuxie/unreal/NuxieUnrealBridge");
    if (Failed(Env) || !Local) return;
    Bridge = static_cast<jclass>(Env->NewGlobalRef(Local));
    Env->DeleteLocalRef(Local);
    if (Failed(Env) || !Bridge) return;
    Version = Env->GetStaticMethodID(Bridge, "contractVersion", "()I");
    if (Failed(Env)) return;
    Dispatch = Env->GetStaticMethodID(Bridge, "dispatch", "(Landroid/app/Activity;Ljava/lang/String;)Z");
    if (Failed(Env)) return;
    Pop = Env->GetStaticMethodID(Bridge, "popMessage", "()Ljava/lang/String;");
    if (Failed(Env)) Pop = nullptr;
  }
  ~FNuxieAndroidTransport() override {
    if (JNIEnv* Env = FAndroidApplication::GetJavaEnv()) {
      if (Bridge) Env->DeleteGlobalRef(Bridge);
    }
  }
  int32 ContractVersion() override {
    JNIEnv* Env = FAndroidApplication::GetJavaEnv();
    if (!Env || !Bridge || !Version || !Dispatch || !Pop) return 0;
    const jint Value = Env->CallStaticIntMethod(Bridge, Version);
    return Failed(Env) ? 0 : Value;
  }
  bool Submit(const FString& Request) override {
    JNIEnv* Env = FAndroidApplication::GetJavaEnv();
    if (!Env || !Bridge || !Dispatch || !FJavaWrapper::GameActivityThis) return false;
    const auto Utf16 = StringCast<UTF16CHAR>(*Request, Request.Len());
    jstring Argument = Env->NewString(reinterpret_cast<const jchar*>(Utf16.Get()), Utf16.Length());
    if (Failed(Env) || !Argument) return false;
    const jboolean Accepted = Env->CallStaticBooleanMethod(Bridge, Dispatch, FJavaWrapper::GameActivityThis, Argument);
    Env->DeleteLocalRef(Argument);
    return !Failed(Env) && Accepted == JNI_TRUE;
  }
  bool Poll(FString& Message) override {
    JNIEnv* Env = FAndroidApplication::GetJavaEnv();
    if (!Env || !Bridge || !Pop) return false;
    jstring Value = static_cast<jstring>(Env->CallStaticObjectMethod(Bridge, Pop));
    if (Failed(Env)) { if (Value) Env->DeleteLocalRef(Value); return false; }
    if (!Value) return false;
    const jsize Length = Env->GetStringLength(Value);
    const jchar* Characters = Env->GetStringChars(Value, nullptr);
    if (Failed(Env) || !Characters) { Env->DeleteLocalRef(Value); return false; }
    const auto Converted = StringCast<TCHAR>(reinterpret_cast<const UTF16CHAR*>(Characters), Length);
    Message = FString(Converted.Length(), Converted.Get());
    Env->ReleaseStringChars(Value, Characters);
    Env->DeleteLocalRef(Value);
    return true;
  }
};
}
TUniquePtr<INuxieNativeTransport> CreateNuxieNativeTransport() { return MakeUnique<FNuxieAndroidTransport>(); }
#endif
