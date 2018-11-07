/*
 * Copyright (C) 2013 The Android Open Source Project
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *      http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#ifndef JNI_INVOCATION_H_included
#define JNI_INVOCATION_H_included

#include <jni.h>

__BEGIN_DECLS

void* JniInvocationCreate();
void JniInvocationDestroy(void* instance);
int JniInvocationInit(void* instance, const char* library);
const char* JniInvocationGetLibrary(const char* library, char* buffer);

__END_DECLS

#ifdef __cplusplus

class JniInvocation final {
 public:
  JniInvocation() {
    instance_ = JniInvocationCreate();
  }

  ~JniInvocation() {
    JniInvocationDestroy(instance_);
  }

  bool Init(const char* library) {
    return JniInvocationInit(instance_, library) != 0;
  }

  static const char* GetLibrary(const char* library, char* buffer) {
    return JniInvocationGetLibrary(library, buffer);
  }

 private:
  void* instance_;
};

#endif  //  __cplusplus

#endif  // JNI_INVOCATION_H_included
