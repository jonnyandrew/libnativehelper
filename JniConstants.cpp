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

std::mutex g_class_refs_mutex;
bool g_class_refs_initialized = false;

// Global reference to java.io.FileDescriptor
jclass g_file_descriptor_class = nullptr;

// java.io.FileDescriptor.descriptor
jfieldID g_file_descriptor_descriptor_field = nullptr;

// java.io.FileDescriptor.ownerId
jfieldID g_file_descriptor_owner_id_field = nullptr;

// void java.io.FileDescriptor.<init>()
jmethodID g_file_descriptor_init_method = nullptr;

// Global reference to java.lang.ref.Reference
jclass g_reference_class = nullptr;

// Object java.lang.ref.Reference.get()
jmethodID g_reference_get_method = nullptr;

// Global reference to java.lang.String
jclass g_string_class = nullptr;

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

jclass JniConstants::GetFileDescriptorClass(JNIEnv* env) {
    if (g_file_descriptor_class == nullptr) {
        Initialize(env);
    }
    return g_file_descriptor_class;
}

jfieldID JniConstants::GetFileDescriptorDescriptorField(JNIEnv* env) {
    if (g_file_descriptor_descriptor_field == nullptr) {
        jclass klass = GetFileDescriptorClass(env);
        g_file_descriptor_descriptor_field = FindField(env, klass, "descriptor", "I");
    }
    return g_file_descriptor_descriptor_field;
}

jfieldID JniConstants::GetFileDescriptorOwnerIdField(JNIEnv* env) {
    if (g_file_descriptor_owner_id_field == nullptr) {
        jclass klass = GetFileDescriptorClass(env);
        g_file_descriptor_owner_id_field = FindField(env, klass, "ownerId", "J");
    }
    return g_file_descriptor_owner_id_field;
}

jmethodID JniConstants::GetFileDescriptorInitMethod(JNIEnv* env) {
    if (g_file_descriptor_init_method == nullptr) {
        jclass klass = GetFileDescriptorClass(env);
        g_file_descriptor_init_method = FindMethod(env, klass, "<init>", "()V");
    }
    return g_file_descriptor_init_method;
}

jclass JniConstants::GetReferenceClass(JNIEnv* env) {
    if (g_reference_class == nullptr) {
        Initialize(env);
    }
    return g_reference_class;
}

jmethodID JniConstants::GetReferenceGetMethod(JNIEnv* env) {
    if (g_reference_get_method == nullptr) {
        jclass klass = GetReferenceClass(env);
        g_reference_get_method = FindMethod(env, klass, "get", "()Ljava/lang/Object;");
    }
    return g_reference_get_method;
}

jclass JniConstants::GetStringClass(JNIEnv* env) {
    if (g_string_class == nullptr) {
        Initialize(env);
    }
    return g_string_class;
}

void JniConstants::Initialize(JNIEnv* env) {
    std::lock_guard<std::mutex> guard(g_class_refs_mutex);
    if (g_class_refs_initialized) {
        return;
    }
    // Class constants should be initialized only once because they global
    // references. Field ids and Method ids can be initialized later since they
    // are not references and races only have trivial performance
    // consequences. Note that the FileDescriptor class has a static member that
    // is a FileDescriptor instance. Getting a field id or method id initializes
    // the class and static members and this recurses into field id
    // initialization. A recursive_mutex would be required if field ids were
    // initialized here.
    g_file_descriptor_class = FindClass(env, "java/io/FileDescriptor");
    g_reference_class = FindClass(env, "java/lang/ref/Reference");
    g_string_class = FindClass(env, "java/lang/String");
    g_class_refs_initialized = true;
}

void JniConstants::Uninitialize() {
    // This are invalidated when a new VM instance is created. Since only one VM
    // is supported at a time, we know these are invalid. Not clean, but a
    // stepping stone in addressing some of the internal debt here.
    // Clean shutdown would require DeleteGlobalRef for the class references.
    std::lock_guard<std::mutex> guard(g_class_refs_mutex);
    g_file_descriptor_class = nullptr;
    g_file_descriptor_descriptor_field = nullptr;
    g_file_descriptor_owner_id_field = nullptr;
    g_file_descriptor_init_method = nullptr;
    g_reference_class = nullptr;
    g_reference_get_method = nullptr;
    g_string_class = nullptr;
    g_class_refs_initialized = false;
}
