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
#include "JniConstants.h"
#include "ScopedLocalRef.h"

#include <stdlib.h>

#include <atomic>

#define UNLIKELY(EXPR) __builtin_expect(!!(EXPR), false)

jclass JniConstants::bigDecimalClass;
jclass JniConstants::booleanClass;
jclass JniConstants::byteArrayClass;
jclass JniConstants::byteClass;
jclass JniConstants::calendarClass;
jclass JniConstants::characterClass;
jclass JniConstants::charsetICUClass;
jclass JniConstants::constructorClass;
jclass JniConstants::deflaterClass;
jclass JniConstants::doubleClass;
jclass JniConstants::errnoExceptionClass;
jclass JniConstants::fieldClass;
jclass JniConstants::fileDescriptorClass;
jclass JniConstants::floatClass;
jclass JniConstants::gaiExceptionClass;
jclass JniConstants::inet6AddressClass;
jclass JniConstants::inetAddressClass;
jclass JniConstants::inetAddressHolderClass;
jclass JniConstants::inetSocketAddressClass;
jclass JniConstants::inetSocketAddressHolderClass;
jclass JniConstants::inflaterClass;
jclass JniConstants::inputStreamClass;
jclass JniConstants::integerClass;
jclass JniConstants::localeDataClass;
jclass JniConstants::longClass;
jclass JniConstants::methodClass;
jclass JniConstants::mutableIntClass;
jclass JniConstants::mutableLongClass;
jclass JniConstants::netlinkSocketAddressClass;
jclass JniConstants::objectClass;
jclass JniConstants::objectArrayClass;
jclass JniConstants::outputStreamClass;
jclass JniConstants::packetSocketAddressClass;
jclass JniConstants::parsePositionClass;
jclass JniConstants::patternSyntaxExceptionClass;
jclass JniConstants::referenceClass;
jclass JniConstants::shortClass;
jclass JniConstants::socketClass;
jclass JniConstants::socketImplClass;
jclass JniConstants::stringClass;
jclass JniConstants::structAddrinfoClass;
jclass JniConstants::structFlockClass;
jclass JniConstants::structGroupReqClass;
jclass JniConstants::structGroupSourceReqClass;
jclass JniConstants::structLingerClass;
jclass JniConstants::structPasswdClass;
jclass JniConstants::structPollfdClass;
jclass JniConstants::structStatClass;
jclass JniConstants::structStatVfsClass;
jclass JniConstants::structTimevalClass;
jclass JniConstants::structUcredClass;
jclass JniConstants::structUtsnameClass;
jclass JniConstants::unixSocketAddressClass;
jclass JniConstants::zipEntryClass;

static std::atomic<jfieldID> gFileDescriptorClassDescriptor;
static std::atomic<jmethodID> gFileDescriptorClassInit;
static std::atomic<jmethodID> gReferenceClassGet;

static jclass findClass(JNIEnv* env, const char* name) {
    ScopedLocalRef<jclass> localClass(env, env->FindClass(name));
    jclass result = reinterpret_cast<jclass>(env->NewGlobalRef(localClass.get()));
    if (result == NULL) {
        ALOGE("failed to find class '%s'", name);
        abort();
    }
    return result;
}

static inline jfieldID findAndCacheField(std::atomic<jfieldID>& cache, JNIEnv* env, jclass klass,
                                         const char* name, const char* desc) {
    jfieldID result = cache.load(std::memory_order_acquire);
    if (UNLIKELY(result == NULL)) {
        result = env->GetFieldID(klass, name, desc);
        cache.store(result, std::memory_order_release);

        // Sanity check: abort if the field is not available. This may happen if the class couldn't
        // be initialized (e.g. still too early to resolve this field) or the programmer specified
        // an incorrect field name or an incorrect type descriptor.
        if (UNLIKELY(result == NULL)) {
            ALOGV("failed to find field '%s:%s'", name, desc);
            abort();
        }
    }
    return result;
}

static inline jmethodID findAndCacheMethod(std::atomic<jmethodID>& cache, JNIEnv* env,
                                           jclass klass, const char* name, const char* signature) {
    jmethodID result = cache.load(std::memory_order_acquire);
    if (UNLIKELY(result == NULL)) {
        result = env->GetMethodID(klass, name, signature);
        cache.store(result, std::memory_order_release);

        // Sanity check: abort if the method is not available. This may happen if the class couldn't
        // be initialized (e.g. still too early to resolve this method) or the programmer specified
        // an incorrect method name or an incorrect signature.
        if (UNLIKELY(result == NULL)) {
            ALOGV("failed to find method '%s%s'", name, signature);
            abort();
        }
    }
    return result;
}

void JniConstants::init(JNIEnv* env) {
    bigDecimalClass = findClass(env, "java/math/BigDecimal");
    booleanClass = findClass(env, "java/lang/Boolean");
    byteClass = findClass(env, "java/lang/Byte");
    byteArrayClass = findClass(env, "[B");
    calendarClass = findClass(env, "java/util/Calendar");
    characterClass = findClass(env, "java/lang/Character");
    charsetICUClass = findClass(env, "java/nio/charset/CharsetICU");
    constructorClass = findClass(env, "java/lang/reflect/Constructor");
    floatClass = findClass(env, "java/lang/Float");
    deflaterClass = findClass(env, "java/util/zip/Deflater");
    doubleClass = findClass(env, "java/lang/Double");
    errnoExceptionClass = findClass(env, "android/system/ErrnoException");
    fieldClass = findClass(env, "java/lang/reflect/Field");
    fileDescriptorClass = findClass(env, "java/io/FileDescriptor");
    gaiExceptionClass = findClass(env, "android/system/GaiException");
    inet6AddressClass = findClass(env, "java/net/Inet6Address");
    inetAddressClass = findClass(env, "java/net/InetAddress");
    inetAddressHolderClass = findClass(env, "java/net/InetAddress$InetAddressHolder");
    inetSocketAddressClass = findClass(env, "java/net/InetSocketAddress");
    inetSocketAddressHolderClass = findClass(env, "java/net/InetSocketAddress$InetSocketAddressHolder");
    inflaterClass = findClass(env, "java/util/zip/Inflater");
    inputStreamClass = findClass(env, "java/io/InputStream");
    integerClass = findClass(env, "java/lang/Integer");
    localeDataClass = findClass(env, "libcore/icu/LocaleData");
    longClass = findClass(env, "java/lang/Long");
    methodClass = findClass(env, "java/lang/reflect/Method");
    mutableIntClass = findClass(env, "android/util/MutableInt");
    mutableLongClass = findClass(env, "android/util/MutableLong");
    netlinkSocketAddressClass = findClass(env, "android/system/NetlinkSocketAddress");
    objectClass = findClass(env, "java/lang/Object");
    objectArrayClass = findClass(env, "[Ljava/lang/Object;");
    outputStreamClass = findClass(env, "java/io/OutputStream");
    packetSocketAddressClass = findClass(env, "android/system/PacketSocketAddress");
    parsePositionClass = findClass(env, "java/text/ParsePosition");
    patternSyntaxExceptionClass = findClass(env, "java/util/regex/PatternSyntaxException");
    referenceClass = findClass(env, "java/lang/ref/Reference");
    shortClass = findClass(env, "java/lang/Short");
    socketClass = findClass(env, "java/net/Socket");
    socketImplClass = findClass(env, "java/net/SocketImpl");
    stringClass = findClass(env, "java/lang/String");
    structAddrinfoClass = findClass(env, "android/system/StructAddrinfo");
    structFlockClass = findClass(env, "android/system/StructFlock");
    structGroupReqClass = findClass(env, "android/system/StructGroupReq");
    structGroupSourceReqClass = findClass(env, "android/system/StructGroupSourceReq");
    structLingerClass = findClass(env, "android/system/StructLinger");
    structPasswdClass = findClass(env, "android/system/StructPasswd");
    structPollfdClass = findClass(env, "android/system/StructPollfd");
    structStatClass = findClass(env, "android/system/StructStat");
    structStatVfsClass = findClass(env, "android/system/StructStatVfs");
    structTimevalClass = findClass(env, "android/system/StructTimeval");
    structUcredClass = findClass(env, "android/system/StructUcred");
    structUtsnameClass = findClass(env, "android/system/StructUtsname");
    unixSocketAddressClass = findClass(env, "android/system/UnixSocketAddress");
    zipEntryClass = findClass(env, "java/util/zip/ZipEntry");

    // Initialize the cached jfieldIDs and jmethodIDs with NULL. We have to reset them because these
    // may be changed between different VM initializations (p.s. VM may be set up and torn down
    // several times in the unit tests.)
    gFileDescriptorClassDescriptor.store(NULL, std::memory_order_release);
    gFileDescriptorClassInit.store(NULL, std::memory_order_release);
    gReferenceClassGet.store(NULL, std::memory_order_release);
}

jfieldID JniConstants::getFileDescriptorClassDescriptor(JNIEnv* env) {
    return findAndCacheField(gFileDescriptorClassDescriptor, env, fileDescriptorClass,
                             "descriptor", "I");
}

jmethodID JniConstants::getFileDescriptorClassInit(JNIEnv* env) {
    return findAndCacheMethod(gFileDescriptorClassInit, env, fileDescriptorClass, "<init>", "()V");
}

jmethodID JniConstants::getReferenceClassGet(JNIEnv* env) {
    return findAndCacheMethod(gReferenceClassGet, env, referenceClass, "get",
                              "()Ljava/lang/Object;");
}
