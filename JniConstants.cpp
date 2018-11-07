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
#include "JNIHelp-priv.h"
#include <nativehelper/JniConstants.h>
#include <nativehelper/JniConstants-priv.h>
#include <nativehelper/ScopedLocalRef.h>

#include <stdlib.h>

#include <atomic>
#include <mutex>

jclass JniConstants_booleanClass;
jclass JniConstants_byteArrayClass;
jclass JniConstants_calendarClass;
jclass JniConstants_charsetICUClass;
jclass JniConstants_doubleClass;
jclass JniConstants_errnoExceptionClass;
jclass JniConstants_fileDescriptorClass;
jclass JniConstants_gaiExceptionClass;
jclass JniConstants_inet6AddressClass;
jclass JniConstants_inet6AddressHolderClass;
jclass JniConstants_inetAddressClass;
jclass JniConstants_inetAddressHolderClass;
jclass JniConstants_inetSocketAddressClass;
jclass JniConstants_inetSocketAddressHolderClass;
jclass JniConstants_integerClass;
jclass JniConstants_localeDataClass;
jclass JniConstants_longClass;
jclass JniConstants_netlinkSocketAddressClass;
jclass JniConstants_packetSocketAddressClass;
jclass JniConstants_patternSyntaxExceptionClass;
jclass JniConstants_referenceClass;
jclass JniConstants_socketTaggerClass;
jclass JniConstants_stringClass;
jclass JniConstants_structAddrinfoClass;
jclass JniConstants_structFlockClass;
jclass JniConstants_structGroupReqClass;
jclass JniConstants_structIfaddrs;
jclass JniConstants_structLingerClass;
jclass JniConstants_structPasswdClass;
jclass JniConstants_structPollfdClass;
jclass JniConstants_structStatClass;
jclass JniConstants_structStatVfsClass;
jclass JniConstants_structTimespecClass;
jclass JniConstants_structTimevalClass;
jclass JniConstants_structUcredClass;
jclass JniConstants_structUtsnameClass;
jclass JniConstants_unixSocketAddressClass;
jclass JniConstants_zipEntryClass;

static std::atomic<bool> g_constants_initialized(false);
static std::mutex g_constants_mutex;

static jclass findClass(JNIEnv* env, const char* name) {
    ScopedLocalRef<jclass> localClass(env, env->FindClass(name));
    jclass result = reinterpret_cast<jclass>(env->NewGlobalRef(localClass.get()));
    if (result == NULL) {
        ALOGE("failed to find class '%s'", name);
        abort();
    }
    return result;
}

void JniConstants_init(C_JNIEnv* abi_env) {
    // Fast check
    if (g_constants_initialized) {
      // already initialized
      return;
    }

    // Slightly slower check
    std::lock_guard<std::mutex> guard(g_constants_mutex);
    if (g_constants_initialized) {
      // already initialized
      return;
    }

    // The following cast is ordinarily unsafe across ABI boundaries. It is safe as the layout of
    // JNIEnv is an append-only POCO. We should not add to the list of constants here for safety
    // reasons.
    JNIEnv* env = reinterpret_cast<JNIEnv*>(abi_env);

    JniConstants_booleanClass = findClass(env, "java/lang/Boolean");
    JniConstants_byteArrayClass = findClass(env, "[B");
    JniConstants_calendarClass = findClass(env, "java/util/Calendar");
    JniConstants_charsetICUClass = findClass(env, "java/nio/charset/CharsetICU");
    JniConstants_doubleClass = findClass(env, "java/lang/Double");
    JniConstants_errnoExceptionClass = findClass(env, "android/system/ErrnoException");
    JniConstants_fileDescriptorClass = findClass(env, "java/io/FileDescriptor");
    JniConstants_gaiExceptionClass = findClass(env, "android/system/GaiException");
    JniConstants_inet6AddressClass = findClass(env, "java/net/Inet6Address");
    JniConstants_inet6AddressHolderClass = findClass(env, "java/net/Inet6Address$Inet6AddressHolder");
    JniConstants_inetAddressClass = findClass(env, "java/net/InetAddress");
    JniConstants_inetAddressHolderClass = findClass(env, "java/net/InetAddress$InetAddressHolder");
    JniConstants_inetSocketAddressClass = findClass(env, "java/net/InetSocketAddress");
    JniConstants_inetSocketAddressHolderClass = findClass(env, "java/net/InetSocketAddress$InetSocketAddressHolder");
    JniConstants_integerClass = findClass(env, "java/lang/Integer");
    JniConstants_localeDataClass = findClass(env, "libcore/icu/LocaleData");
    JniConstants_longClass = findClass(env, "java/lang/Long");
    JniConstants_netlinkSocketAddressClass = findClass(env, "android/system/NetlinkSocketAddress");
    JniConstants_packetSocketAddressClass = findClass(env, "android/system/PacketSocketAddress");
    JniConstants_patternSyntaxExceptionClass = findClass(env, "java/util/regex/PatternSyntaxException");
    JniConstants_referenceClass = findClass(env, "java/lang/ref/Reference");
    JniConstants_socketTaggerClass = findClass(env, "dalvik/system/SocketTagger");
    JniConstants_stringClass = findClass(env, "java/lang/String");
    JniConstants_structAddrinfoClass = findClass(env, "android/system/StructAddrinfo");
    JniConstants_structFlockClass = findClass(env, "android/system/StructFlock");
    JniConstants_structGroupReqClass = findClass(env, "android/system/StructGroupReq");
    JniConstants_structIfaddrs = findClass(env, "android/system/StructIfaddrs");
    JniConstants_structLingerClass = findClass(env, "android/system/StructLinger");
    JniConstants_structPasswdClass = findClass(env, "android/system/StructPasswd");
    JniConstants_structPollfdClass = findClass(env, "android/system/StructPollfd");
    JniConstants_structStatClass = findClass(env, "android/system/StructStat");
    JniConstants_structStatVfsClass = findClass(env, "android/system/StructStatVfs");
    JniConstants_structTimespecClass = findClass(env, "android/system/StructTimespec");
    JniConstants_structTimevalClass = findClass(env, "android/system/StructTimeval");
    JniConstants_structUcredClass = findClass(env, "android/system/StructUcred");
    JniConstants_structUtsnameClass = findClass(env, "android/system/StructUtsname");
    JniConstants_unixSocketAddressClass = findClass(env, "android/system/UnixSocketAddress");
    JniConstants_zipEntryClass = findClass(env, "java/util/zip/ZipEntry");

    g_constants_initialized = true;
}

void android_ClearJniConstantsCache() {
    g_constants_initialized = false;
    android::ClearJNIHelpLocalCache();
}

