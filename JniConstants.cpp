/*
 * Copyright (C) 2010 The Android Open Source Project
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
#include <nativehelper/JniConstants.h>
#include <nativehelper/ScopedLocalRef.h>

#include <stdlib.h>

#include <atomic>
#include <mutex>

static std::atomic<bool> g_class_constants_initialized(false);
static std::atomic<bool> g_field_method_constants_initialized(false);
static std::mutex g_constants_mutex;

jclass JniConstants::booleanClass;
jclass JniConstants::byteArrayClass;
jclass JniConstants::calendarClass;
jclass JniConstants::charsetICUClass;
jclass JniConstants::doubleClass;
jclass JniConstants::errnoExceptionClass;
jclass JniConstants::fileDescriptorClass;
jclass JniConstants::gaiExceptionClass;
jclass JniConstants::inet6AddressClass;
jclass JniConstants::inet6AddressHolderClass;
jclass JniConstants::inetAddressClass;
jclass JniConstants::inetAddressHolderClass;
jclass JniConstants::inetSocketAddressClass;
jclass JniConstants::inetSocketAddressHolderClass;
jclass JniConstants::integerClass;
jclass JniConstants::localeDataClass;
jclass JniConstants::longClass;
jclass JniConstants::mutableIntClass;
jclass JniConstants::mutableLongClass;
jclass JniConstants::netlinkSocketAddressClass;
jclass JniConstants::packetSocketAddressClass;
jclass JniConstants::patternSyntaxExceptionClass;
jclass JniConstants::referenceClass;
jclass JniConstants::socketTaggerClass;
jclass JniConstants::stringClass;
jclass JniConstants::structAddrinfoClass;
jclass JniConstants::structFlockClass;
jclass JniConstants::structGroupReqClass;
jclass JniConstants::structGroupSourceReqClass;
jclass JniConstants::structIfaddrs;
jclass JniConstants::structLingerClass;
jclass JniConstants::structPasswdClass;
jclass JniConstants::structPollfdClass;
jclass JniConstants::structStatClass;
jclass JniConstants::structStatVfsClass;
jclass JniConstants::structTimevalClass;
jclass JniConstants::structTimespecClass;
jclass JniConstants::structUcredClass;
jclass JniConstants::structUtsnameClass;
jclass JniConstants::unixSocketAddressClass;
jclass JniConstants::zipEntryClass;

jfieldID JniConstants::fileDescriptorClassDescriptor;

jmethodID JniConstants::fileDescriptorClassInit;
jmethodID JniConstants::referenceClassGet;

static jclass findClass(JNIEnv* env, const char* name) {
    ScopedLocalRef<jclass> localClass(env, env->FindClass(name));
    jclass result = reinterpret_cast<jclass>(env->NewGlobalRef(localClass.get()));
    if (result == NULL) {
        ALOGE("failed to find class '%s'", name);
        abort();
    }
    return result;
}

static jfieldID findField(JNIEnv* env, jclass klass, const char* name, const char* desc) {
    jfieldID result = env->GetFieldID(klass, name, desc);
    if (result == NULL) {
        ALOGV("failed to find field '%s:%s'", name, desc);
        abort();
    }
    return result;
}

static jmethodID findMethod(JNIEnv* env, jclass klass, const char* name, const char* signature) {
    jmethodID result = env->GetMethodID(klass, name, signature);
    if (result == NULL) {
        ALOGV("failed to find method '%s%s'", name, signature);
        abort();
    }
    return result;
}

// Call once with double-checked locking pattern.  This is a clone of std::call_once().  We can't
// simply use std::call_once() because we would like to reset the once_flag.
template<typename Callable, typename... Args>
static inline void callOnce(std::atomic<bool>& once_flag, Callable f, Args&&... args) {
    if (once_flag) {
        return;
    }

    std::lock_guard<std::mutex> guard(g_constants_mutex);
    if (once_flag) {
        return;
    }

    f(std::forward<Args>(args)...);

    once_flag = true;
}

// This method re-initializes the class constants, and resets the fields and methods.
//
// init() is called early in runtime startup, and is meant to reset the state so that
// runtime can be shutdown and restarted.
void JniConstants::init(JNIEnv* env) {
    initClassConstants(env);
    g_class_constants_initialized = true;
    g_field_method_constants_initialized = false;
}

void JniConstants::ensureClassesInitialized(JNIEnv* env) {
    callOnce(g_class_constants_initialized, initClassConstants, env);
}

// TODO: make them unique.
void JniConstants::ensureFieldsInitialized(JNIEnv* env) {
    auto init_field_method = [&](JNIEnv* env_in) {
        JniConstants::initFieldConstants(env_in);
        JniConstants::initMethodConstants(env_in);
    };
    callOnce(g_field_method_constants_initialized, init_field_method, env);
}
void JniConstants::ensureMethodsInitialized(JNIEnv* env) {
    auto init_field_method = [&](JNIEnv* env_in) {
        JniConstants::initFieldConstants(env_in);
        JniConstants::initMethodConstants(env_in);
    };
    callOnce(g_field_method_constants_initialized, init_field_method, env);
}

void JniConstants::initClassConstants(JNIEnv* env) {
    booleanClass = findClass(env, "java/lang/Boolean");
    byteArrayClass = findClass(env, "[B");
    calendarClass = findClass(env, "java/util/Calendar");
    charsetICUClass = findClass(env, "java/nio/charset/CharsetICU");
    doubleClass = findClass(env, "java/lang/Double");
    errnoExceptionClass = findClass(env, "android/system/ErrnoException");
    fileDescriptorClass = findClass(env, "java/io/FileDescriptor");
    gaiExceptionClass = findClass(env, "android/system/GaiException");
    inet6AddressClass = findClass(env, "java/net/Inet6Address");
    inet6AddressHolderClass = findClass(env, "java/net/Inet6Address$Inet6AddressHolder");
    inetAddressClass = findClass(env, "java/net/InetAddress");
    inetAddressHolderClass = findClass(env, "java/net/InetAddress$InetAddressHolder");
    inetSocketAddressClass = findClass(env, "java/net/InetSocketAddress");
    inetSocketAddressHolderClass = findClass(env, "java/net/InetSocketAddress$InetSocketAddressHolder");
    integerClass = findClass(env, "java/lang/Integer");
    localeDataClass = findClass(env, "libcore/icu/LocaleData");
    longClass = findClass(env, "java/lang/Long");
    mutableIntClass = findClass(env, "android/util/MutableInt");
    mutableLongClass = findClass(env, "android/util/MutableLong");
    netlinkSocketAddressClass = findClass(env, "android/system/NetlinkSocketAddress");
    packetSocketAddressClass = findClass(env, "android/system/PacketSocketAddress");
    patternSyntaxExceptionClass = findClass(env, "java/util/regex/PatternSyntaxException");
    referenceClass = findClass(env, "java/lang/ref/Reference");
    socketTaggerClass = findClass(env, "dalvik/system/SocketTagger");
    stringClass = findClass(env, "java/lang/String");
    structAddrinfoClass = findClass(env, "android/system/StructAddrinfo");
    structFlockClass = findClass(env, "android/system/StructFlock");
    structGroupReqClass = findClass(env, "android/system/StructGroupReq");
    structGroupSourceReqClass = findClass(env, "android/system/StructGroupSourceReq");
    structIfaddrs = findClass(env, "android/system/StructIfaddrs");
    structLingerClass = findClass(env, "android/system/StructLinger");
    structPasswdClass = findClass(env, "android/system/StructPasswd");
    structPollfdClass = findClass(env, "android/system/StructPollfd");
    structStatClass = findClass(env, "android/system/StructStat");
    structStatVfsClass = findClass(env, "android/system/StructStatVfs");
    structTimevalClass = findClass(env, "android/system/StructTimeval");
    structTimespecClass = findClass(env, "android/system/StructTimespec");
    structUcredClass = findClass(env, "android/system/StructUcred");
    structUtsnameClass = findClass(env, "android/system/StructUtsname");
    unixSocketAddressClass = findClass(env, "android/system/UnixSocketAddress");
    zipEntryClass = findClass(env, "java/util/zip/ZipEntry");
}

void JniConstants::initFieldConstants(JNIEnv* env) {
    ensureClassesInitialized(env);
    fileDescriptorClassDescriptor = findField(env, fileDescriptorClass, "descriptor", "I");
}

void JniConstants::initMethodConstants(JNIEnv* env) {
    ensureClassesInitialized(env);
    fileDescriptorClassInit = findMethod(env, fileDescriptorClass, "<init>", "()V");
    referenceClassGet = findMethod(env, referenceClass, "get", "()Ljava/lang/Object;");
}
