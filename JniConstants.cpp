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

// Recursive mutex for initializing constants (explained in JniConstants::Initialize).
std::recursive_mutex g_constants_mutex;
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

// Global reference to java.io.FileDescriptor
jclass JniConstants::fileDescriptorClass = nullptr;

// Global reference to java.lang.ref.Reference
jclass JniConstants::referenceClass = nullptr;

// Global reference to java.lang.String
jclass JniConstants::stringClass = nullptr;

// java.io.FileDescriptor.descriptor.
jfieldID JniConstants::fileDescriptorDescriptorField = nullptr;

// java.io.FileDescriptor.ownerId.
jfieldID JniConstants::fileDescriptorOwnerIdField = nullptr;

// void java.io.FileDescriptor.<init>().
jmethodID JniConstants::fileDescriptorInitMethod = nullptr;

// Object java.lang.ref.Reference.get()
jmethodID JniConstants::referenceGetMethod = nullptr;

void JniConstants::Initialize(JNIEnv* env) {
    // This method is invoked recursively, hence the recursive_mutex. The cause
    // is the FindField/FindMethod calls below. The FileDescriptor class has a
    // public static FileDescriptor field which is initialized by duping a file
    // descriptor which ends up calling JNIHelp::jniGetFDFromDescriptor which
    // calls Initialize() since the fileDescriptorField is uninitialized.
    std::lock_guard<std::recursive_mutex> guard(g_constants_mutex);
    if (g_constants_initialized) {
        return;
    }

    // Get classes taking care not to leak global references.
    if (fileDescriptorClass == nullptr) {
        fileDescriptorClass = FindClass(env, "java/io/FileDescriptor");
    }
    if (referenceClass == nullptr) {
        referenceClass = FindClass(env, "java/lang/ref/Reference");
    }
    if (stringClass == nullptr) {
        stringClass = FindClass(env, "java/lang/String");
    }

    // Get fields and methods avoiding redundant JNI calls.
    if (fileDescriptorDescriptorField == nullptr) {
        // Recursion into the current method happens in FieldField() code path here.
        fileDescriptorDescriptorField =
            FindField(env, fileDescriptorClass, "descriptor", "I");
    }
    if (fileDescriptorOwnerIdField == nullptr) {
        fileDescriptorOwnerIdField =
            FindField(env, fileDescriptorClass, "ownerId", "J");
    }
    if (fileDescriptorInitMethod == nullptr) {
        fileDescriptorInitMethod =
            FindMethod(env, fileDescriptorClass, "<init>", "()V");
    }
    if (referenceGetMethod == nullptr) {
        referenceGetMethod = FindMethod(env, referenceClass, "get", "()Ljava/lang/Object;");
    }
    g_constants_initialized = true;
}

void JniConstants::Uninitialize() {
    // This are invalidated when a new VM instance is created. Since only one VM
    // is supported at a time, we know these are invalid. Not clean, but a
    // stepping stone in addressing some of the internal debt here.
    std::lock_guard<std::recursive_mutex> guard(g_constants_mutex);
    fileDescriptorClass = nullptr;
    fileDescriptorDescriptorField = nullptr;
    fileDescriptorOwnerIdField = nullptr;
    fileDescriptorInitMethod = nullptr;
    // Clean shutdown would require DeleteGlobalRef for the class references.
    referenceClass = nullptr;
    referenceGetMethod = nullptr;
    stringClass = nullptr;
    g_constants_initialized = false;
}
