// Copyright (c) 2017 Greenheart Games Pty. Ltd. All rights reserved.
// Use of this source code is governed by the MIT license that can be
// found in the LICENSE file.

#include "nan.h"
#include "v8.h"

#include "steam/steam_api.h"

#include "greenworks_utils.h"
#include "steam_api_registry.h"
#include "steam_async_worker.h"

namespace greenworks {
namespace api {
namespace {

class StoreUserStatsWorker : public SteamCallbackAsyncWorker {
 public:
  StoreUserStatsWorker(Nan::Callback* success_callback,
                       Nan::Callback* error_callback);
  STEAM_CALLBACK(StoreUserStatsWorker,
                 OnStoreUserStatsCompleted,
                 UserStatsStored_t,
                 result);

  // Override NanAsyncWorker methods.
  void Execute() override;
  void HandleOKCallback() override;

 private:
  uint64 game_id_;
  CSteamID steam_id_user_;
};

StoreUserStatsWorker::StoreUserStatsWorker(Nan::Callback* success_callback,
                                           Nan::Callback* error_callback)
    : SteamCallbackAsyncWorker(success_callback, error_callback),
      result(this, &StoreUserStatsWorker::OnStoreUserStatsCompleted) {}

void StoreUserStatsWorker::Execute() {
  SteamUserStats()->StoreStats();
  WaitForCompleted();
}

void StoreUserStatsWorker::OnStoreUserStatsCompleted(
    UserStatsStored_t* result) {
  if (result->m_eResult != k_EResultOK) {
    SetErrorMessage("Error on storing user stats.");
  } else {
    game_id_ = result->m_nGameID;
  }
  is_completed_ = true;
}

void StoreUserStatsWorker::HandleOKCallback() {
  Nan::HandleScope scope;
  v8::Local<v8::Value> argv[] = {
      Nan::New(utils::uint64ToString(game_id_)).ToLocalChecked()};
  callback->Call(1, argv);
}

NAN_METHOD(GetStatInt) {
  Nan::HandleScope scope;
  if (info.Length() < 1 || !info[0]->IsString()) {
    THROW_BAD_ARGS("Bad arguments");
  }

  std::string name = (*(v8::String::Utf8Value(info[0])));
  int32 result = 0;
  if (SteamUserStats()->GetStat(name.c_str(), &result)) {
    info.GetReturnValue().Set(result);
    return;
  }
  info.GetReturnValue().Set(Nan::Undefined());
}

NAN_METHOD(GetStatFloat) {
  Nan::HandleScope scope;
  if (info.Length() < 1 || !info[0]->IsString()) {
    THROW_BAD_ARGS("Bad arguments");
  }

  std::string name = (*(v8::String::Utf8Value(info[0])));
  float result = 0;
  if (SteamUserStats()->GetStat(name.c_str(), &result)) {
    info.GetReturnValue().Set(result);
    return;
  }
  info.GetReturnValue().Set(Nan::Undefined());
}

NAN_METHOD(SetStat) {
  Nan::HandleScope scope;
  if (info.Length() < 2 || !info[0]->IsString() || (!info[1]->IsNumber())) {
    THROW_BAD_ARGS("Bad arguments");
  }

  std::string name = *(v8::String::Utf8Value(info[0]));
  if (info[1]->IsInt32()) {
    int32 value = info[1].As<v8::Number>()->Int32Value();
    info.GetReturnValue().Set(SteamUserStats()->SetStat(name.c_str(), value));
    return;
  }

  double value = info[1].As<v8::Number>()->NumberValue();
  info.GetReturnValue().Set(
      SteamUserStats()->SetStat(name.c_str(), static_cast<float>(value)));
}

NAN_METHOD(StoreStats) {
  Nan::HandleScope scope;
  if (info.Length() < 1 || (!info[0]->IsFunction())) {
    THROW_BAD_ARGS("Bad arguments");
  }
  Nan::Callback* success_callback =
      new Nan::Callback(info[0].As<v8::Function>());
  Nan::Callback* error_callback = nullptr;
  if (info.Length() > 1 && info[1]->IsFunction())
    error_callback = new Nan::Callback(info[1].As<v8::Function>());

  Nan::AsyncQueueWorker(
      new StoreUserStatsWorker(success_callback, error_callback));
  info.GetReturnValue().Set(Nan::Undefined());
}

void RegisterAPIs(v8::Handle<v8::Object> target) {
  Nan::Set(target, Nan::New("getStatInt").ToLocalChecked(),
           Nan::New<v8::FunctionTemplate>(GetStatInt)->GetFunction());
  Nan::Set(target, Nan::New("getStatFloat").ToLocalChecked(),
           Nan::New<v8::FunctionTemplate>(GetStatFloat)->GetFunction());
  Nan::Set(target, Nan::New("setStat").ToLocalChecked(),
           Nan::New<v8::FunctionTemplate>(SetStat)->GetFunction());
  Nan::Set(target, Nan::New("storeStats").ToLocalChecked(),
           Nan::New<v8::FunctionTemplate>(StoreStats)->GetFunction());
}

SteamAPIRegistry::Add X(RegisterAPIs);

}  // namespace
}  // namespace api
}  // namespace greenworks
