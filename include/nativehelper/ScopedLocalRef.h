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

#ifndef SCOPED_LOCAL_REF_H_included
#define SCOPED_LOCAL_REF_H_included

#include <cstddef>

#include "jni.h"
#include "JNIHelp.h"  // for DISALLOW_COPY_AND_ASSIGN.

// A smart pointer that deletes a JNI local reference when it goes out of scope.
template<typename T>
class ScopedLocalRef {
public:
    ScopedLocalRef(JNIEnv* env, T localRef) : mEnv(env), mLocalRef(localRef) {
    }

    ~ScopedLocalRef() {
        reset();
    }

    void reset(T ptr = NULL) {
        if (ptr != mLocalRef) {
            if (mLocalRef != NULL) {
                mEnv->DeleteLocalRef(mLocalRef);
            }
            mLocalRef = ptr;
        }
    }

    T release() __attribute__((warn_unused_result)) {
        T localRef = mLocalRef;
        mLocalRef = NULL;
        return localRef;
    }

    T get() const {
        return mLocalRef;
    }

// Some better C++11 support.
#if __cplusplus >= 201103L
    // Move constructor.
    ScopedLocalRef(ScopedLocalRef&& s) : mEnv(s.mEnv), mLocalRef(s.release()) {
    }

    // Empty constructor now makes sense, as we can move-assign.
    ScopedLocalRef() : mEnv(nullptr), mLocalRef(nullptr) {
    }

    // Move assignment operator.
    ScopedLocalRef& operator=(ScopedLocalRef&& s) {
        reset(s.release());
        mEnv = s.mEnv;
    }

    // Workaround for missing nullptr_t in NDK's cxx-stl. b/65466378
    using nullptr_t = decltype(nullptr);

    // Allows "if (scoped_ref == nullptr)"
    bool operator==(nullptr_t) const {
        return mLocalRef == nullptr;
    }

    // Allows "if (scoped_ref != nullptr)"
    bool operator!=(nullptr_t) const {
        return mLocalRef != nullptr;
    }
#endif

private:
    JNIEnv* const mEnv;
    T mLocalRef;

    DISALLOW_COPY_AND_ASSIGN(ScopedLocalRef);
};

#endif  // SCOPED_LOCAL_REF_H_included
