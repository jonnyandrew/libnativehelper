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

#ifndef SCOPED_PRIMITIVE_ARRAY_H_
#define SCOPED_PRIMITIVE_ARRAY_H_

#include "jni.h"
#include "nativehelper_utils.h"

#define ARRAY_TRAITS(ARRAY_TYPE, POINTER_TYPE, NAME)                               \
class NAME ## ArrayTraits {                                                        \
public:                                                                            \
    static inline void getArrayRegion(JNIEnv* env, ARRAY_TYPE array, size_t start, \
                                         size_t len, POINTER_TYPE out) {           \
        env->Get ## NAME ## ArrayRegion(array, start, len, out);                   \
    }                                                                              \
                                                                                   \
    static inline POINTER_TYPE getArrayElements(JNIEnv* env, ARRAY_TYPE array) {   \
        return env->Get ## NAME ## ArrayElements(array, nullptr);                  \
    }                                                                              \
                                                                                   \
    static inline void releaseArrayElements(JNIEnv* env, ARRAY_TYPE array,         \
                                               POINTER_TYPE buffer, jint mode) {   \
        env->Release ## NAME ## ArrayElements(array, buffer, mode);                \
    }                                                                              \
};                                                                                 \

ARRAY_TRAITS(jbooleanArray, jboolean*, Boolean)
ARRAY_TRAITS(jbyteArray, jbyte*, Byte)
ARRAY_TRAITS(jcharArray, jchar*, Char)
ARRAY_TRAITS(jdoubleArray, jdouble*, Double)
ARRAY_TRAITS(jfloatArray, jfloat*, Float)
ARRAY_TRAITS(jintArray, jint*, Int)
ARRAY_TRAITS(jlongArray, jlong*, Long)
ARRAY_TRAITS(jshortArray, jshort*, Short)

#undef ARRAY_TRAITS

template<typename JavaArrayType, typename PrimitiveType, class Traits>
class ScopedNonNullArrayRO {
public:
    // TODO: Remove single argument constructor.
    explicit ScopedNonNullArrayRO(JNIEnv* env)
        : mEnv(env), mJavaArray(nullptr), mRawArray(nullptr), mSize(0) {}
    ScopedNonNullArrayRO(JNIEnv* env, JavaArrayType javaArray) : mEnv(env), mJavaArray(javaArray) {
        if (mJavaArray == nullptr) {
            mSize = 0;
            mRawArray = nullptr;
            jniThrowNullPointerException(mEnv, nullptr);
        } else {
            reset(mJavaArray);
        }
    }

    ~ScopedNonNullArrayRO() {
        if (mRawArray != nullptr && mRawArray != mBuffer) {
            Traits::releaseArrayElements(mEnv, mJavaArray, mRawArray, JNI_ABORT);
        }
    }

    // TODO: Remove reset method.
    void reset(JavaArrayType javaArray) {
        mJavaArray = javaArray;
        mSize = mEnv->GetArrayLength(mJavaArray);
        if (mSize <= BUFFER_SIZE) {
            Traits::getArrayRegion(mEnv, mJavaArray, 0, mSize, mBuffer);
            mRawArray = mBuffer;
        } else {
            mRawArray = Traits::getArrayElements(mEnv, mJavaArray);
        }
    }

    const PrimitiveType* get() const { return mRawArray; }
    JavaArrayType getJavaArray() const { return mJavaArray; }
    const PrimitiveType& operator[](size_t n) const { return mRawArray[n]; }
    size_t size() const { return mSize; }

private:
    constexpr static jsize BUFFER_SIZE = 1024;

    JNIEnv* const mEnv;
    JavaArrayType mJavaArray;
    PrimitiveType* mRawArray;
    jsize mSize;
    PrimitiveType mBuffer[BUFFER_SIZE];
    DISALLOW_COPY_AND_ASSIGN(ScopedNonNullArrayRO);
};

// Scoped***ArrayRO provide convenient read-only access to Java array from JNI code.
// This is cheaper than read-write access and should be used by default.
// These throw NPE if nullptr is passed.
using ScopedBooleanArrayRO = ScopedNonNullArrayRO<jbooleanArray, jboolean, BooleanArrayTraits>;
using ScopedByteArrayRO = ScopedNonNullArrayRO<jbyteArray, jbyte, ByteArrayTraits>;
using ScopedCharArrayRO = ScopedNonNullArrayRO<jcharArray, jchar, CharArrayTraits>;
using ScopedDoubleArrayRO = ScopedNonNullArrayRO<jdoubleArray, jdouble, DoubleArrayTraits>;
using ScopedFloatArrayRO = ScopedNonNullArrayRO<jfloatArray, jfloat, FloatArrayTraits>;
using ScopedIntArrayRO = ScopedNonNullArrayRO<jintArray, jint, IntArrayTraits>;
using ScopedLongArrayRO = ScopedNonNullArrayRO<jlongArray, jlong, LongArrayTraits>;
using ScopedShortArrayRO = ScopedNonNullArrayRO<jshortArray, jshort, ShortArrayTraits>;

template<typename JavaArrayType, typename PrimitiveType, class Traits>
class ScopedNullableArrayRO {
public:
    ScopedNullableArrayRO(JNIEnv* env, JavaArrayType javaArray) : mEnv(env), mJavaArray(javaArray) {
        if (mJavaArray == nullptr) {
            mSize = 0;
            mRawArray = nullptr;
        } else {
            mSize = mEnv->GetArrayLength(mJavaArray);
            if (mSize < BUFFER_SIZE) {
                Traits::getArrayRegion(mEnv, mJavaArray, 0, mSize, mBuffer);
                mRawArray = mBuffer;
            } else {
                mRawArray = Traits::getArrayElements(mEnv, mJavaArray);
            }
        }
    }

    ~ScopedNullableArrayRO() {
        if (mRawArray != nullptr && mRawArray != mBuffer) {
            Traits::releaseArrayElements(mEnv, mJavaArray, mRawArray, JNI_ABORT);
        }
    }

    const PrimitiveType* get() const { return mRawArray; }
    JavaArrayType getJavaArray() const { return mJavaArray; }
    const PrimitiveType& operator[](size_t n) const { return mRawArray[n]; }
    size_t size() const { return mSize; }

private:
    constexpr static jsize BUFFER_SIZE = 1024;

    JNIEnv* const mEnv;
    JavaArrayType mJavaArray;
    PrimitiveType* mRawArray;
    jsize mSize;
    PrimitiveType mBuffer[BUFFER_SIZE];
    DISALLOW_COPY_AND_ASSIGN(ScopedNullableArrayRO);
};

// ScopedNullable***ArrayRO also provide convenient read-only access to Java array from JNI code.
// These accept nullptr. In that case, get() returns nullptr and size() returns 0.
using ScopedNullableBooleanArrayRO
        = ScopedNullableArrayRO<jbooleanArray, jboolean, BooleanArrayTraits>;
using ScopedNullableByteArrayRO = ScopedNullableArrayRO<jbyteArray, jbyte, ByteArrayTraits>;
using ScopedNullableCharArrayRO = ScopedNullableArrayRO<jcharArray, jchar, CharArrayTraits>;
using ScopedNullableDoubleArrayRO = ScopedNullableArrayRO<jdoubleArray, jdouble, DoubleArrayTraits>;
using ScopedNullableFloatArrayRO = ScopedNullableArrayRO<jfloatArray, jfloat, FloatArrayTraits>;
using ScopedNullableIntArrayRO = ScopedNullableArrayRO<jintArray, jint, IntArrayTraits>;
using ScopedNullableLongArrayRO = ScopedNullableArrayRO<jlongArray, jlong, LongArrayTraits>;
using ScopedNullableShortArrayRO = ScopedNullableArrayRO<jshortArray, jshort, ShortArrayTraits>;

template<typename JavaArrayType, typename PrimitiveType, class Traits>
class ScopedArrayRW {
public:
    ScopedArrayRW(JNIEnv* env, JavaArrayType javaArray) : mEnv(env), mJavaArray(javaArray) {
        if (mJavaArray == nullptr) {
            mSize = 0;
            mRawArray = nullptr;
            jniThrowNullPointerException(mEnv, nullptr);
        } else {
            mSize = mEnv->GetArrayLength(mJavaArray);
            mRawArray = Traits::getArrayElements(mEnv, mJavaArray);
        }
    }
    ~ScopedArrayRW() {
        if (mRawArray != nullptr) {
            Traits::releaseArrayElements(mEnv, mJavaArray, mRawArray, 0);
        }
    }

    const PrimitiveType* get() const { return mRawArray; }
    JavaArrayType getJavaArray() const { return mJavaArray; }
    const PrimitiveType& operator[](size_t n) const { return mRawArray[n]; }
    PrimitiveType* get() { return mRawArray; }
    PrimitiveType& operator[](size_t n) { return mRawArray[n]; }
    size_t size() const { return mSize; }

private:
    JNIEnv* const mEnv;
    JavaArrayType mJavaArray;
    PrimitiveType* mRawArray;
    jsize mSize;
    DISALLOW_COPY_AND_ASSIGN(ScopedArrayRW);
};

// Scoped***ArrayRW provide convenient read-write access to Java arrays from JNI code.
// These are more expensive, since they entail a copy back onto the Java heap, and should only be
// used when necessary.
// These throw NPE if nullptr is passed.
using ScopedBooleanArrayRW = ScopedArrayRW<jbooleanArray, jboolean, BooleanArrayTraits>;
using ScopedByteArrayRW = ScopedArrayRW<jbyteArray, jbyte, ByteArrayTraits>;
using ScopedCharArrayRW = ScopedArrayRW<jcharArray, jchar, CharArrayTraits>;
using ScopedDoubleArrayRW = ScopedArrayRW<jdoubleArray, jdouble, DoubleArrayTraits>;
using ScopedFloatArrayRW = ScopedArrayRW<jfloatArray, jfloat, FloatArrayTraits>;
using ScopedIntArrayRW = ScopedArrayRW<jintArray, jint, IntArrayTraits>;
using ScopedLongArrayRW = ScopedArrayRW<jlongArray, jlong, LongArrayTraits>;
using ScopedShortArrayRW = ScopedArrayRW<jshortArray, jshort, ShortArrayTraits>;

#endif  // SCOPED_PRIMITIVE_ARRAY_H_
