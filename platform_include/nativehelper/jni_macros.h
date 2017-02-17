/*
 * Copyright (C) 2017 The Android Open Source Project
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

/*
 * JNI helper macros.
 *
 * Only intended to be used in the platform.
 */

#ifndef NATIVEHELPER_JNI_MACROS_H
#define NATIVEHELPER_JNI_MACROS_H

// Intended to construct a JNINativeMethod.
//   (Assumes the C name is the ClassName_JavaMethodName).
#ifndef NATIVE_METHOD
#define NATIVE_METHOD(className, functionName, signature) \
  { #functionName, signature, reinterpret_cast<void*>(className ## _ ## functionName) }
#endif

// Intended to construct a JNINativeMethod (when the C name doesn't match the Java name).
//   (Assumes the C name is the ClassName_Identifier).
#ifndef OVERLOADED_NATIVE_METHOD
#define OVERLOADED_NATIVE_METHOD(className, functionName, signature, identifier) \
  { #functionName, signature, reinterpret_cast<void*>(className ## _ ## identifier) }
#endif

// Used for methods that are annotated with @FastNative on the managed side.
//   See NATIVE_METHOD for usage.
#ifndef FAST_NATIVE_METHOD
#define FAST_NATIVE_METHOD NATIVE_METHOD
#endif

// Used for methods that are annotated with @FastNative on the managed side,
//   and when the C-name doesn't match the Java-name.
//
//   See OVERLOADED_NATIVE_METHOD for usage.
#ifndef OVERLOADED_FAST_NATIVE_METHOD
#define OVERLOADED_FAST_NATIVE_METHOD OVERLOADED_NATIVE_METHOD
#endif

#endif
