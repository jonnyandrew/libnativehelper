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

#ifdef POINTER_TYPE
#error POINTER_TYPE is defined.
#else
#define POINTER_TYPE(T) T*  /* NOLINT */
#endif

template<typename JType> struct ScopedPrimitiveArrayTraits {};

#define ARRAY_TRAITS(ARRAY_TYPE, JTYPE, NAME)                                            \
template<> struct ScopedPrimitiveArrayTraits<JTYPE> {                                    \
public:                                                                                  \
    static inline void getArrayRegion(JNIEnv* env, ARRAY_TYPE array, size_t start,       \
                                      size_t len, POINTER_TYPE(JTYPE) out) {             \
        env->Get ## NAME ## ArrayRegion(array, start, len, out);                         \
    }                                                                                    \
                                                                                         \
    static inline POINTER_TYPE(JTYPE) getArrayElements(JNIEnv* env, ARRAY_TYPE array) {  \
        return env->Get ## NAME ## ArrayElements(array, nullptr);                        \
    }                                                                                    \
                                                                                         \
    static inline void releaseArrayElements(JNIEnv* env, ARRAY_TYPE array,               \
                                            POINTER_TYPE(JTYPE) buffer, jint mode) {     \
        env->Release ## NAME ## ArrayElements(array, buffer, mode);                      \
    }                                                                                    \
    using ArrayType = ARRAY_TYPE;                                                        \
};                                                                                       \

ARRAY_TRAITS(jbooleanArray, jboolean, Boolean)
ARRAY_TRAITS(jbyteArray, jbyte, Byte)
ARRAY_TRAITS(jcharArray, jchar, Char)
ARRAY_TRAITS(jdoubleArray, jdouble, Double)
ARRAY_TRAITS(jfloatArray, jfloat, Float)
ARRAY_TRAITS(jintArray, jint, Int)
ARRAY_TRAITS(jlongArray, jlong, Long)
ARRAY_TRAITS(jshortArray, jshort, Short)

#undef ARRAY_TRAITS
#undef POINTER_TYPE

template<typename JType, bool kNullable>
class ScopedArrayRO {
public:
    using Traits = ScopedPrimitiveArrayTraits<JType>;
    using ArrayType = typename Traits::ArrayType;

    // TODO: Remove single argument constructor.
    explicit ScopedArrayRO(JNIEnv* env)
        : mEnv(env), mJavaArray(nullptr), mRawArray(nullptr), mSize(0) {}

    ScopedArrayRO(JNIEnv* env, ArrayType javaArray) : mEnv(env), mJavaArray(javaArray) {
        if (mJavaArray == nullptr) {
            mSize = 0;
            mRawArray = nullptr;
            if (!kNullable) {
                jniThrowNullPointerException(mEnv, nullptr);
            }
        } else {
            reset(mJavaArray);
        }
    }

    ~ScopedArrayRO() {
        if (mRawArray != nullptr && mRawArray != mBuffer) {
            Traits::releaseArrayElements(mEnv, mJavaArray, mRawArray, JNI_ABORT);
        }
    }

    // TODO: Remove reset method.
    void reset(ArrayType javaArray) {
        mJavaArray = javaArray;
        mSize = mEnv->GetArrayLength(mJavaArray);
        if (mSize <= BUFFER_SIZE) {
            Traits::getArrayRegion(mEnv, mJavaArray, 0, mSize, mBuffer);
            mRawArray = mBuffer;
        } else {
            mRawArray = Traits::getArrayElements(mEnv, mJavaArray);
        }
    }

    const JType* get() const { return mRawArray; }
    ArrayType getJavaArray() const { return mJavaArray; }
    const JType& operator[](size_t n) const { return mRawArray[n]; }
    const JType* begin() const { return get(); }
    const JType* end() const { return get() + mSize; }
    size_t size() const { return mSize; }

private:
    constexpr static jsize BUFFER_SIZE = 1024;

    JNIEnv* const mEnv;
    ArrayType mJavaArray;
    JType* mRawArray;
    jsize mSize;

    // Speed-up JNI array access for small arrays, see I703d7346de732199be1feadbead021c6647a554a
    // for more details.
    JType mBuffer[BUFFER_SIZE];

    DISALLOW_COPY_AND_ASSIGN(ScopedArrayRO);
};

// Scoped***ArrayRO provide convenient read-only access to Java array from JNI code.
// This is cheaper than read-write access and should be used by default.
// These throw NPE if nullptr is passed.
using ScopedBooleanArrayRO = ScopedArrayRO<jboolean, false>;
using ScopedByteArrayRO = ScopedArrayRO<jbyte, false>;
using ScopedCharArrayRO = ScopedArrayRO<jchar, false>;
using ScopedDoubleArrayRO = ScopedArrayRO<jdouble, false>;
using ScopedFloatArrayRO = ScopedArrayRO<jfloat, false>;
using ScopedIntArrayRO = ScopedArrayRO<jint, false>;
using ScopedLongArrayRO = ScopedArrayRO<jlong, false>;
using ScopedShortArrayRO = ScopedArrayRO<jshort, false>;

// ScopedNullable***ArrayRO also provide convenient read-only access to Java array from JNI code.
// These accept nullptr. In that case, get() returns nullptr and size() returns 0.
using ScopedNullableBooleanArrayRO = ScopedArrayRO<jboolean, true>;
using ScopedNullableByteArrayRO = ScopedArrayRO<jbyte, true>;
using ScopedNullableCharArrayRO = ScopedArrayRO<jchar, true>;
using ScopedNullableDoubleArrayRO = ScopedArrayRO<jdouble, true>;
using ScopedNullableFloatArrayRO = ScopedArrayRO<jfloat, true>;
using ScopedNullableIntArrayRO = ScopedArrayRO<jint, true>;
using ScopedNullableLongArrayRO = ScopedArrayRO<jlong, true>;
using ScopedNullableShortArrayRO = ScopedArrayRO<jshort, true>;

template<typename JType>
class ScopedArrayRW {
public:
    using Traits = ScopedPrimitiveArrayTraits<JType>;
    using ArrayType = typename Traits::ArrayType;

    ScopedArrayRW(JNIEnv* env, ArrayType javaArray) : mEnv(env), mJavaArray(javaArray) {
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

    const JType* get() const { return mRawArray; }
    ArrayType getJavaArray() const { return mJavaArray; }
    const JType& operator[](size_t n) const { return mRawArray[n]; }
    const JType* begin() const { return get(); }
    const JType* end() const { return get() + mSize; }
    JType* get() { return mRawArray; }
    JType& operator[](size_t n) { return mRawArray[n]; }
    JType* begin() { return get(); }
    JType* end() { return get() + mSize; }
    size_t size() const { return mSize; }

private:
    JNIEnv* const mEnv;
    ArrayType mJavaArray;
    JType* mRawArray;
    jsize mSize;
    DISALLOW_COPY_AND_ASSIGN(ScopedArrayRW);
};

// Scoped***ArrayRW provide convenient read-write access to Java arrays from JNI code.
// These are more expensive, since they entail a copy back onto the Java heap, and should only be
// used when necessary.
// These throw NPE if nullptr is passed.
using ScopedBooleanArrayRW = ScopedArrayRW<jboolean>;
using ScopedByteArrayRW = ScopedArrayRW<jbyte>;
using ScopedCharArrayRW = ScopedArrayRW<jchar>;
using ScopedDoubleArrayRW = ScopedArrayRW<jdouble>;
using ScopedFloatArrayRW = ScopedArrayRW<jfloat>;
using ScopedIntArrayRW = ScopedArrayRW<jint>;
using ScopedLongArrayRW = ScopedArrayRW<jlong>;
using ScopedShortArrayRW = ScopedArrayRW<jshort>;

#endif  // SCOPED_PRIMITIVE_ARRAY_H_
