/*
 * Copyright (C) 2006 The Android Open Source Project
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

#define LOG_TAG "JniConstants"
#include "ALog-priv.h"

#include "JniConstants.h"

#include <mutex>
#include <string>

#include "nativehelper/ScopedLocalRef.h"

namespace {

std::mutex g_constants_mutex;
bool g_constants_initialized = false;

jclass FindClass(JNIEnv* env, const char* name) {
    ScopedLocalRef<jclass> klass(env, env->FindClass(name));
    ALOG_ALWAYS_FATAL_IF(klass.get() == nullptr, "failed to find class '%s'", name);
    return reinterpret_cast<jclass>(env->NewGlobalRef(klass.get()));
}

jfieldID FindField(JNIEnv* env, jclass klass, const char* name, const char* desc) {
    jfieldID result = env->GetFieldID(klass, name, desc);
    ALOG_ALWAYS_FATAL_IF(result == nullptr, "failed to find field '%s:%s'", name, desc);
    return result;
}

jmethodID FindMethod(JNIEnv* env, jclass klass, const char* name, const char* signature) {
    jmethodID result = env->GetMethodID(klass, name, signature);
    ALOG_ALWAYS_FATAL_IF(result == nullptr, "failed to find method '%s%s'", name, signature);
    return result;
}

}  // namespace

// java.io.FileDescriptor
jclass JniConstants::fileDescriptorClass;

// java.io.FileDescriptor.descriptor.
jfieldID JniConstants::fileDescriptorDescriptorField = nullptr;

// java.io.FileDescriptor.ownerId.
jfieldID JniConstants::fileDescriptorOwnerIdField = nullptr;

// void java.io.FileDescriptor.<init>().
jmethodID JniConstants::fileDescriptorInitMethod = nullptr;

// java.lang.ref.Reference
jclass JniConstants::referenceClass;

// Object java.lang.ref.Reference.get()
jmethodID JniConstants::referenceGetMethod = nullptr;

// java.lang.String
jclass JniConstants::stringClass;

void JniConstants::Initialize(JNIEnv* env) {
    std::lock_guard<std::mutex> guard(g_constants_mutex);
    if (g_constants_initialized) {
        return;
    }
    fileDescriptorClass = FindClass(env, "java/io/FileDescriptor");
    fileDescriptorDescriptorField =
        FindField(env, JniConstants::fileDescriptorClass, "descriptor", "I");
    fileDescriptorOwnerIdField =
        FindField(env, JniConstants::fileDescriptorClass, "ownerId", "J");
    fileDescriptorInitMethod =
        FindMethod(env, JniConstants::fileDescriptorClass, "<init>", "()V");
    referenceClass = FindClass(env, "java/lang/ref/Reference");
    referenceGetMethod =
        FindMethod(env, JniConstants::referenceClass, "get", "()Ljava/lang/Object;");
    stringClass = FindClass(env, "java/lang/String");
    g_constants_initialized = true;
}

void JniConstants::Uninitialize() {
    // This are invalidated when a new VM instance is created. Since only one VM
    // is supported at a time, we know these are invalid. Not clean, but a
    // stepping stone in addressing some of the internal debt here.
    std::lock_guard<std::mutex> guard(g_constants_mutex);
    fileDescriptorClass = nullptr;
    fileDescriptorDescriptorField = nullptr;
    fileDescriptorOwnerIdField = nullptr;
    fileDescriptorInitMethod = nullptr;
    referenceClass = nullptr;
    referenceGetMethod = nullptr;
    stringClass = nullptr;
    g_constants_initialized = false;
}
