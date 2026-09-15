#include "NuxieNativeTransport.h"
#if PLATFORM_ANDROID
#include "Android/AndroidApplication.h"
#include "Android/AndroidJNI.h"
#include "Containers/StringConv.h"
#include "Android/AndroidEventManager.h"
#include "NuxieSession.h"
#include "Misc/ScopeLock.h"
#include <android_native_app_glue.h>
#include <android/looper.h>
#include <sys/eventfd.h>
#include <unistd.h>
#include <cerrno>
extern struct android_app* GNativeAndroidApp;
namespace {
// Process-lifetime callback storage: the engine event thread may already have
// exited at module shutdown. Never destroy its remaining lease on another thread.
struct FWakeChannel {
  FCriticalSection Lock;
  bool Pending = false;
  bool Stopping = false;
  int Counter = -1;
  ALooper* Looper = nullptr;
  FAppEventManager::GameThreadTicker* Lease = nullptr;
};
FWakeChannel& WakeChannel() { static FWakeChannel* Channel = new FWakeChannel(); return *Channel; }
int DispatchWakeOnEventThread(int Fd, int Events, void*) {
  check(IsInAndroidEventThread());
  auto& Channel = WakeChannel();
  FScopeLock Guard(&Channel.Lock);
  if (Channel.Counter != Fd) return 0;
  uint64 Signals;
  // eventfd coalesces all notifications in one bounded read.
  while (read(Fd, &Signals, sizeof(Signals)) < 0 && errno == EINTR) {}
  delete Channel.Lease; Channel.Lease = nullptr;
  if (Channel.Stopping || (Events & (ALOOPER_EVENT_ERROR | ALOOPER_EVENT_HANGUP))) {
    Channel.Stopping = true; Channel.Pending = false;
    ALooper_removeFd(Channel.Looper, Fd);
    close(Fd); Channel.Counter = -1;
    ALooper_release(Channel.Looper); Channel.Looper = nullptr;
    return 0;
  }
  // Read current desired state, never stale acquire/release commands. Every
  // lease operation is serialized with Unreal's activation and suspension here.
  if (Channel.Pending) Channel.Lease = new FAppEventManager::GameThreadTicker();
  return 1;
}
bool EnsureWakeChannelLocked(FWakeChannel& Channel) {
  if (Channel.Stopping) return false;
  if (Channel.Counter >= 0) return true;
  if (!GNativeAndroidApp || !GNativeAndroidApp->looper) return false;
  const int Fd = eventfd(0, EFD_NONBLOCK | EFD_CLOEXEC);
  if (Fd < 0) return false;
  Channel.Looper = GNativeAndroidApp->looper;
  ALooper_acquire(Channel.Looper);
  if (ALooper_addFd(Channel.Looper, Fd, ALOOPER_POLL_CALLBACK, ALOOPER_EVENT_INPUT,
      &DispatchWakeOnEventThread, nullptr) != 1) {
    close(Fd); ALooper_release(Channel.Looper); Channel.Looper = nullptr;
    return false;
  }
  Channel.Counter = Fd;
  return true;
}
bool SignalWakeLocked(FWakeChannel& Channel) {
  if (Channel.Counter < 0) return false;
  const uint64 Signal = 1;
  ssize_t Written;
  do { Written = write(Channel.Counter, &Signal, sizeof(Signal)); } while (Written < 0 && errno == EINTR);
  return Written == sizeof(Signal) || (Written < 0 && errno == EAGAIN);
}
jboolean JNICALL RenewDispatchWake(JNIEnv*, jclass) {
  auto& Channel = WakeChannel();
  FScopeLock Guard(&Channel.Lock);
  if (!Channel.Pending || Channel.Stopping) return JNI_FALSE;
  return SignalWakeLocked(Channel) ? JNI_TRUE : JNI_FALSE;
}
bool HasDispatchWork(JNIEnv* Env, jclass Bridge) {
  if (FNuxieSession::HasDeferredCallbacks()) return true;
  if (!FNuxieSession::HasNativeOwner()) return false;
  jmethodID HasMessages = Env->GetStaticMethodID(Bridge, "hasMessages", "()Z");
  bool Pending = false;
  if (!Env->ExceptionCheck() && HasMessages) {
    Pending = Env->CallStaticBooleanMethod(Bridge, HasMessages) == JNI_TRUE;
  }
  if (Env->ExceptionCheck()) { Env->ExceptionClear(); return false; }
  return Pending;
}
void JNICALL NotifyMessages(JNIEnv* Env, jclass Bridge) {
  {
    auto& Channel = WakeChannel();
    FScopeLock Guard(&Channel.Lock);
    if (Channel.Pending || !EnsureWakeChannelLocked(Channel)) return;
    Channel.Pending = true;
    SignalWakeLocked(Channel);
  }
  jclass RetainedBridge = static_cast<jclass>(Env->NewGlobalRef(Bridge));
  if (Env->ExceptionCheck() || !RetainedBridge) {
    Env->ExceptionClear();
    if (RetainedBridge) Env->DeleteGlobalRef(RetainedBridge);
    { auto& Channel = WakeChannel(); FScopeLock Guard(&Channel.Lock); Channel.Pending = false; SignalWakeLocked(Channel); }
    return;
  }
  jmethodID Schedule = Env->GetStaticMethodID(Bridge, "scheduleDispatchWake", "()V");
  if (!Env->ExceptionCheck() && Schedule) Env->CallStaticVoidMethod(Bridge, Schedule);
  if (Env->ExceptionCheck()) Env->ExceptionClear();
  // Keep the engine awake across ordinary core-ticker frames. The session and
  // deferred queues retain their per-frame budgets; no event-loop drain recursion.
  FTSTicker::GetCoreTicker().AddTicker(FTickerDelegate::CreateLambda([RetainedBridge](float) {
    JNIEnv* CallbackEnv = FAndroidApplication::GetJavaEnv();
    if (CallbackEnv && HasDispatchWork(CallbackEnv, RetainedBridge)) return true;
    { auto& Channel = WakeChannel(); FScopeLock Guard(&Channel.Lock); Channel.Pending = false; SignalWakeLocked(Channel); }
    if (CallbackEnv) {
      // An arrival before the pending flag was released still needs its wake.
      if (HasDispatchWork(CallbackEnv, RetainedBridge)) NotifyMessages(CallbackEnv, RetainedBridge);
      CallbackEnv->DeleteGlobalRef(RetainedBridge);
    }
    return false;
  }));
}
bool RegisterDispatchCallbacks(JNIEnv* Env, jclass Bridge) {
  { auto& Channel = WakeChannel(); FScopeLock Guard(&Channel.Lock); if (!EnsureWakeChannelLocked(Channel)) return false; }
  JNINativeMethod Methods[] = {
    {const_cast<char*>("notifyMessages"), const_cast<char*>("()V"), reinterpret_cast<void*>(&NotifyMessages)},
    {const_cast<char*>("renewDispatchWake"), const_cast<char*>("()Z"), reinterpret_cast<void*>(&RenewDispatchWake)},
  };
  const jint Result = Env->RegisterNatives(Bridge, Methods, 2);
  if (Env->ExceptionCheck()) { Env->ExceptionClear(); return false; }
  return Result == JNI_OK;
}
class FNuxieAndroidTransport final : public INuxieNativeTransport {
  jclass Bridge = nullptr;
  jmethodID Version = nullptr;
  jmethodID Dispatch = nullptr;
  jmethodID Pop = nullptr;
  jmethodID HasMessages = nullptr;
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
    if (Failed(Env)) { Pop = nullptr; return; }
    HasMessages = Env->GetStaticMethodID(Bridge, "hasMessages", "()Z");
    if (Failed(Env)) { HasMessages = nullptr; return; }
    if (!RegisterDispatchCallbacks(Env, Bridge)) HasMessages = nullptr;
  }
  ~FNuxieAndroidTransport() override {
    if (JNIEnv* Env = FAndroidApplication::GetJavaEnv()) {
      if (Bridge) Env->DeleteGlobalRef(Bridge);
    }
  }
  int32 ContractVersion() override {
    JNIEnv* Env = FAndroidApplication::GetJavaEnv();
    if (!Env || !Bridge || !Version || !Dispatch || !Pop || !HasMessages) return 0;
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
void StopNuxieAndroidDispatcher() {
  auto& Channel = WakeChannel();
  FScopeLock Guard(&Channel.Lock);
  Channel.Stopping = true; Channel.Pending = false;
  // Do not wait: the detached engine event thread may already have exited.
  // If it is alive it self-removes the FD; otherwise process exit reclaims it.
  SignalWakeLocked(Channel);
}
void WakeNuxieAndroidDispatcher() {
  JNIEnv* Env = FAndroidApplication::GetJavaEnv();
  if (!Env) return;
  jclass Bridge = FAndroidApplication::FindJavaClass("ai/nuxie/unreal/NuxieUnrealBridge");
  if (Env->ExceptionCheck()) { Env->ExceptionClear(); if (Bridge) Env->DeleteLocalRef(Bridge); return; }
  if (Bridge) { if (RegisterDispatchCallbacks(Env, Bridge)) NotifyMessages(Env, Bridge); Env->DeleteLocalRef(Bridge); }
}
TUniquePtr<INuxieNativeTransport> CreateNuxieNativeTransport() { return MakeUnique<FNuxieAndroidTransport>(); }
#endif
