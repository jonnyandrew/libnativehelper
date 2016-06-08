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

static std::atomic<bool> g_classes_initialized(false);
static std::mutex g_classes_mutex;

static std::atomic<bool> g_fields_and_methods_initialized(false);
static std::mutex g_fields_and_methods_mutex;


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

jfieldID JniConstants::fileDescriptorDescriptorField;

jmethodID JniConstants::fileDescriptorInitMethod;
jmethodID JniConstants::referenceGetMethod;

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

void JniConstants::clear() {
    g_classes_initialized = false;
    g_fields_and_methods_initialized = false;
}

void JniConstants::init(JNIEnv* env) {
    // Fast check
    if (g_classes_initialized) {
      // already initialized
      return;
    }

    // Slightly slower check
    std::lock_guard<std::mutex> guard(g_classes_mutex);
    if (g_classes_initialized) {
      // already initialized
      return;
    }

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

    g_classes_initialized = true;
}

void JniConstants::initFieldsAndMethods(JNIEnv* env) {
    // Fast check
    if (g_fields_and_methods_initialized) {
      // already initialized
      return;
    }

    // Slightly slower check
    std::lock_guard<std::mutex> guard(g_fields_and_methods_mutex);
    if (g_fields_and_methods_initialized) {
      // already initialized
      return;
    }

    fileDescriptorDescriptorField = findField(env, fileDescriptorClass, "descriptor", "I");
    fileDescriptorInitMethod = findMethod(env, fileDescriptorClass, "<init>", "()V");
    referenceGetMethod = findMethod(env, referenceClass, "get", "()Ljava/lang/Object;");

    g_fields_and_methods_initialized = true;
}
