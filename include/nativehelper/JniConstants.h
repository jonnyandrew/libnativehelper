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

#ifndef JNI_CONSTANTS_H_included
#define JNI_CONSTANTS_H_included

#include "JNIHelp.h"

/**
 * A cache to avoid calling FindClass at runtime.
 *
 * Class lookup is relatively expensive, so we do these lookups at startup. This means that code
 * that never uses, say, java.util.zip.Deflater still has to pay for the lookup, but it means that
 * on device the cost is paid during boot and amortized. A central cache also removes the temptation
 * to dynamically call FindClass rather than add a small cache to each file that needs one. Another
 * cost is that each class cached here requires a global reference, though in practice we save
 * enough by not having a global reference for each file that uses a class such as java.lang.String
 * which is used in several files.
 *
 * FindClass is still called in a couple of situations: when throwing exceptions, and in some of
 * the serialization code. The former is clearly not a performance case, and we're currently
 * assuming that neither is the latter.
 *
 * TODO: similar arguments hold for field and method IDs; we should cache them centrally too.
 */

__BEGIN_DECLS

void JniConstants_init(C_JNIEnv* env);

extern jclass JniConstants_booleanClass;
extern jclass JniConstants_byteArrayClass;
extern jclass JniConstants_calendarClass;
extern jclass JniConstants_charsetICUClass;
extern jclass JniConstants_doubleClass;
extern jclass JniConstants_errnoExceptionClass;
extern jclass JniConstants_fileDescriptorClass;
extern jclass JniConstants_gaiExceptionClass;
extern jclass JniConstants_inet6AddressClass;
extern jclass JniConstants_inet6AddressHolderClass;
extern jclass JniConstants_inetAddressClass;
extern jclass JniConstants_inetAddressHolderClass;
extern jclass JniConstants_inetSocketAddressClass;
extern jclass JniConstants_inetSocketAddressHolderClass;
extern jclass JniConstants_integerClass;
extern jclass JniConstants_localeDataClass;
extern jclass JniConstants_longClass;
extern jclass JniConstants_netlinkSocketAddressClass;
extern jclass JniConstants_packetSocketAddressClass;
extern jclass JniConstants_patternSyntaxExceptionClass;
extern jclass JniConstants_referenceClass;
extern jclass JniConstants_socketTaggerClass;
extern jclass JniConstants_stringClass;
extern jclass JniConstants_structAddrinfoClass;
extern jclass JniConstants_structFlockClass;
extern jclass JniConstants_structGroupReqClass;
extern jclass JniConstants_structIfaddrs;
extern jclass JniConstants_structLingerClass;
extern jclass JniConstants_structPasswdClass;
extern jclass JniConstants_structPollfdClass;
extern jclass JniConstants_structStatClass;
extern jclass JniConstants_structStatVfsClass;
extern jclass JniConstants_structTimespecClass;
extern jclass JniConstants_structTimevalClass;
extern jclass JniConstants_structUcredClass;
extern jclass JniConstants_structUtsnameClass;
extern jclass JniConstants_unixSocketAddressClass;
extern jclass JniConstants_zipEntryClass;

__END_DECLS

#ifdef __cplusplus

struct JniConstants {
    static void init(JNIEnv* env) {
      // This is potentially unsafe, requires POD for JNIEnv and fixed unchanging positions for
      // classes cached here.
      C_JNIEnv* abi_env = reinterpret_cast<C_JNIEnv*>(env);
      JniConstants_init(abi_env);
    }

    static constexpr jclass& booleanClass = JniConstants_booleanClass;
    static constexpr jclass& byteArrayClass = JniConstants_byteArrayClass;
    static constexpr jclass& calendarClass = JniConstants_calendarClass;
    static constexpr jclass& charsetICUClass = JniConstants_charsetICUClass;
    static constexpr jclass& doubleClass = JniConstants_doubleClass;
    static constexpr jclass& errnoExceptionClass = JniConstants_errnoExceptionClass;
    static constexpr jclass& fileDescriptorClass = JniConstants_fileDescriptorClass;
    static constexpr jclass& gaiExceptionClass = JniConstants_gaiExceptionClass;
    static constexpr jclass& inet6AddressClass = JniConstants_inet6AddressClass;
    static constexpr jclass& inet6AddressHolderClass = JniConstants_inet6AddressHolderClass;
    static constexpr jclass& inetAddressClass = JniConstants_inetAddressClass;
    static constexpr jclass& inetAddressHolderClass = JniConstants_inetAddressHolderClass;
    static constexpr jclass& inetSocketAddressClass = JniConstants_inetSocketAddressClass;
    static constexpr jclass& inetSocketAddressHolderClass = JniConstants_inetSocketAddressHolderClass;
    static constexpr jclass& integerClass = JniConstants_integerClass;
    static constexpr jclass& localeDataClass = JniConstants_localeDataClass;
    static constexpr jclass& longClass = JniConstants_longClass;
    static constexpr jclass& netlinkSocketAddressClass = JniConstants_netlinkSocketAddressClass;
    static constexpr jclass& packetSocketAddressClass = JniConstants_packetSocketAddressClass;
    static constexpr jclass& patternSyntaxExceptionClass = JniConstants_patternSyntaxExceptionClass;
    static constexpr jclass& referenceClass = JniConstants_referenceClass;
    static constexpr jclass& socketTaggerClass = JniConstants_socketTaggerClass;
    static constexpr jclass& stringClass = JniConstants_stringClass;
    static constexpr jclass& structAddrinfoClass = JniConstants_structAddrinfoClass;
    static constexpr jclass& structFlockClass = JniConstants_structFlockClass;
    static constexpr jclass& structGroupReqClass = JniConstants_structGroupReqClass;
    static constexpr jclass& structIfaddrs = JniConstants_structIfaddrs;
    static constexpr jclass& structLingerClass = JniConstants_structLingerClass;
    static constexpr jclass& structPasswdClass = JniConstants_structPasswdClass;
    static constexpr jclass& structPollfdClass = JniConstants_structPollfdClass;
    static constexpr jclass& structStatClass = JniConstants_structStatClass;
    static constexpr jclass& structStatVfsClass = JniConstants_structStatVfsClass;
    static constexpr jclass& structTimespecClass = JniConstants_structTimespecClass;
    static constexpr jclass& structTimevalClass = JniConstants_structTimevalClass;
    static constexpr jclass& structUcredClass = JniConstants_structUcredClass;
    static constexpr jclass& structUtsnameClass = JniConstants_structUtsnameClass;
    static constexpr jclass& unixSocketAddressClass = JniConstants_unixSocketAddressClass;
    static constexpr jclass& zipEntryClass = JniConstants_zipEntryClass;
};

#endif  // __cplusplus

#define NATIVE_METHOD(className, functionName, signature) \
    { #functionName, signature, reinterpret_cast<void*>(className ## _ ## functionName) }

#endif  // JNI_CONSTANTS_H_included
