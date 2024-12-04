/*
* Copyright (C) 2019-2022 The Kraken authors. All rights reserved.
* Copyright (C) 2022-present The WebF authors. All rights reserved.
*/

#include "atomic_string.h"
#include <v8/v8.h>
#include <codecvt>
#include "bindings/v8/native_string_utils.h"
#include "built_in_string.h"
#include "event_type_names.h"
#include "gtest/gtest.h"
#include "libplatform.h"

using namespace webf;

using TestCallback = void (*)(v8::Local<v8::Context> ctx);

bool v8_platform_inited = false;
std::unique_ptr<v8::Platform> platform;

void TestAtomicString(TestCallback callback) {
  if (!v8_platform_inited) {
    // Initialize V8.
    v8::V8::InitializeICUDefaultLocation(nullptr);
    v8::V8::InitializeExternalStartupData(nullptr);
    platform = v8::platform::NewDefaultPlatform();
    v8::V8::InitializePlatform(platform.get());
    v8::V8::Initialize();
    v8_platform_inited = true;
  }

  // Create a new Isolate and make it the current one.
  v8::Isolate::CreateParams create_params;
  create_params.array_buffer_allocator = v8::ArrayBuffer::Allocator::NewDefaultAllocator();
  v8::Isolate* isolate = v8::Isolate::New(create_params);

  {
    v8::Isolate::Scope isolate_scope(isolate);
    // Create a stack-allocated handle scope.
    v8::HandleScope handle_scope(isolate);

    // Create a new context.
    v8::Local<v8::Context> context = v8::Context::New(isolate);

    // Enter the context for compiling and running the hello world script.
    v8::Context::Scope context_scope(context);

    built_in_string::Init(isolate);

    callback(context);
  }

  built_in_string::Dispose();

  // Dispose the isolate and tear down V8.
  isolate->Dispose();
}

TEST(AtomicString, Empty) {
  TestAtomicString([](v8::Local<v8::Context> ctx) {
    AtomicString atomic_string = AtomicString::Empty();
    EXPECT_STREQ(atomic_string.ToStdString(ctx->GetIsolate()).c_str(), "");
  });
}

TEST(AtomicString, FromNativeString) {
  TestAtomicString([](v8::Local<v8::Context> ctx) {
    auto nativeString = stringToNativeString("helloworld");
    AtomicString value =
        AtomicString(ctx->GetIsolate(),
                     std::unique_ptr<AutoFreeNativeString>(static_cast<AutoFreeNativeString*>(nativeString.release())));

    EXPECT_STREQ(value.ToStdString(ctx->GetIsolate()).c_str(), "helloworld");
  });
}

TEST(AtomicString, CreateFromStdString) {
  TestAtomicString([](v8::Local<v8::Context> ctx) {
    AtomicString&& value = AtomicString(ctx->GetIsolate(), "helloworld");
    EXPECT_STREQ(value.ToStdString(ctx->GetIsolate()).c_str(), "helloworld");
  });
}

TEST(AtomicString, CreateFromJSValue) {
  TestAtomicString([](v8::Local<v8::Context> ctx) {
    v8::Local<v8::String> string = v8::String::NewFromUtf8(ctx->GetIsolate(), "helloworld").ToLocalChecked();
    AtomicString&& value = AtomicString(ctx, string);
    EXPECT_STREQ(value.ToStdString(ctx->GetIsolate()).c_str(), "helloworld");
  });
}

TEST(AtomicString, ToV8) {
  TestAtomicString([](v8::Local<v8::Context> ctx) {
    AtomicString&& value = AtomicString(ctx->GetIsolate(), "helloworld");
    v8::Local<v8::String> v8_string_value = value.ToV8(ctx->GetIsolate())->ToString(ctx).ToLocalChecked();
    size_t utf_len = v8_string_value->Utf8Length(ctx->GetIsolate());
    char* str_buffer = new char[utf_len];
    v8_string_value->WriteUtf8(ctx->GetIsolate(), str_buffer);

    EXPECT_STREQ(str_buffer, "helloworld");
  });
}

TEST(AtomicString, ToNativeString) {
  TestAtomicString([](v8::Local<v8::Context> ctx) {
    AtomicString&& value = AtomicString(ctx->GetIsolate(), "helloworld");
    auto native_string = value.ToNativeString(ctx->GetIsolate());
    const uint16_t* p = native_string->string();
    EXPECT_EQ(native_string->length(), 10);

    uint16_t result[10] = {'h', 'e', 'l', 'l', 'o', 'w', 'o', 'r', 'l', 'd'};
    for (int i = 0; i < native_string->length(); i++) {
      EXPECT_EQ(result[i], p[i]);
    }
  });
}

TEST(AtomicString, CopyAssignment) {
  TestAtomicString([](v8::Local<v8::Context> ctx) {
    AtomicString str = AtomicString(ctx->GetIsolate(), "helloworld");
    struct P {
      AtomicString str;
    };
    P p{AtomicString::Empty()};
    v8::Local<v8::Value> v = str.ToV8(ctx->GetIsolate());
    p.str = str;
    EXPECT_EQ(p.str == str, true);
  });
}

TEST(AtomicString, MoveAssignment) {
  TestAtomicString([](v8::Local<v8::Context> ctx) {
    auto&& str = AtomicString(ctx->GetIsolate(), "helloworld");
    auto&& str2 = AtomicString(std::move(str));
    EXPECT_STREQ(str2.ToStdString(ctx->GetIsolate()).c_str(), "helloworld");
  });
}

TEST(AtomicString, CopyToRightReference) {
  TestAtomicString([](v8::Local<v8::Context> ctx) {
    AtomicString str = AtomicString::Empty();
    if (1 + 1 == 2) {
      str = AtomicString(ctx->GetIsolate(), "helloworld");
    }
    EXPECT_STREQ(str.ToStdString(ctx->GetIsolate()).c_str(), "helloworld");
  });
}
