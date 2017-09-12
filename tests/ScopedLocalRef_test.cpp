/*
 * Copyright (C) 2017 The Android Open Source Project
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

#define LOG_TAG "ScopedLocalRef_test"

#include <nativehelper/ScopedLocalRef.h>

#include <dirent.h>
#include <string.h>

#include <gtest/gtest.h>

#include <jni.h>
#include <nativehelper/JniConstants.h>
#include <nativehelper/JniInvocation.h>
#include <nativehelper/jni_gtest.h>

namespace android {

template <typename Provider>
class ScopedLocalRefTest : public JNITestBase<Provider> {
public:
    using Base = JNITestBase<Provider>;
    using Base::env_;

    void SetUp() override {
        Base::SetUp();

        // Install our own function table that's writable. A runtime
        // one might be in the data section.
        InstallInfCopy();

        // Use reserved3 to store this.
        GetCurrentInf()->reserved3 = this;

        // Check whether we're running over a mock, and
        // install interesting functions and "allocate"
        // accordingly.
        if (GetCurrentInf()->IsSameObject == nullptr) {
            // Just simple numbers.
            obj_ = reinterpret_cast<jobject>(1);
            obj2_ = reinterpret_cast<jobject>(2);

            // Simple pointer equality.
            GetCurrentInf()->IsSameObject = IsSameObject;
            // Remember deletion.
            GetCurrentInf()->DeleteLocalRef = DeleteLocalRef;
        } else {
            obj_ = env_->NewStringUTF("Test");
            ASSERT_TRUE(obj_ != nullptr);

            obj2_ = env_->NewStringUTF("Test2");
            ASSERT_TRUE(obj2_ != nullptr);
            ASSERT_TRUE(obj_ != obj2_);
            ASSERT_FALSE(env_->IsSameObject(obj_, obj2_));

            // We still want to check deletion.
            original_delete_local_ref = GetCurrentInf()->DeleteLocalRef;
            GetCurrentInf()->DeleteLocalRef = DeleteLocalRef;
        }
    }

protected:
    void InstallInfCopy() {
        // We'll possibly leak something here. Figure out a way
        // to not do this for the mock provider.
        JNINativeInterface* new_inf = new JNINativeInterface();
        memcpy(new_inf, env_->functions, sizeof(JNINativeInterface));
        env_->functions = new_inf;
    }
    JNINativeInterface* GetCurrentInf() {
        return const_cast<JNINativeInterface*>(env_->functions);
    }

    bool CheckDeleted(size_t index, jobject j) {
        return deleted_[index] == reinterpret_cast<uintptr_t>(j);
    }

    static jboolean IsSameObject(JNIEnv* env __attribute__((unused)),
            jobject j1, jobject j2) {
        return reinterpret_cast<uintptr_t>(j1) ==
                reinterpret_cast<uintptr_t>(j2);
    }

    static void DeleteLocalRef(JNIEnv* env, jobject j) {
        ScopedLocalRefTest* t =  reinterpret_cast<ScopedLocalRefTest*>(
                env->functions->reserved3);
        t->deleted_.push_back(reinterpret_cast<uintptr_t>(j));
    }

    jobject obj_;
    jobject obj2_;

    void (*original_delete_local_ref)(JNIEnv*, jobject);
    std::vector<uintptr_t> deleted_;
};

// Test against the mock provider.
typedef ::testing::Types<MockJNIProvider> Providers;
TYPED_TEST_CASE(ScopedLocalRefTest, Providers);

TYPED_TEST(ScopedLocalRefTest, EmptyConstructor) {
    ScopedLocalRef<jobject> ref(this->env_);
    EXPECT_TRUE(ref.get() == nullptr);
}

TYPED_TEST(ScopedLocalRefTest, Constructor) {
    ScopedLocalRef<jobject> ref(this->env_, this->obj_);
    EXPECT_EQ(ref.get(), this->obj_);
}

TYPED_TEST(ScopedLocalRefTest, MoveConstructor) {
    ScopedLocalRef<jobject> ref(this->env_, this->obj_);
    ASSERT_EQ(ref.get(), this->obj_);

    ScopedLocalRef<jobject> moved_ref(std::move(ref));
    EXPECT_EQ(moved_ref.get(), this->obj_);
    EXPECT_TRUE(ref.get() == nullptr);

    EXPECT_TRUE(this->deleted_.empty());
}

TYPED_TEST(ScopedLocalRefTest, Reset) {
    ScopedLocalRef<jobject> ref(this->env_);
    ASSERT_TRUE(ref.get() == nullptr);
    ref.reset(this->obj_);
    EXPECT_EQ(ref.get(), this->obj_);
    EXPECT_TRUE(this->deleted_.empty());
    ref.reset(this->obj2_);
    EXPECT_EQ(ref.get(), this->obj2_);
    EXPECT_EQ(1u, this->deleted_.size());
    EXPECT_TRUE(this->CheckDeleted(0, this->obj_));
    ref.reset();
    EXPECT_TRUE(ref.get() == nullptr);
    EXPECT_EQ(2u, this->deleted_.size());
    EXPECT_TRUE(this->CheckDeleted(1, this->obj2_));
}

TYPED_TEST(ScopedLocalRefTest, Release) {
    // In a scope to have a simple non-death test.
    {
        ScopedLocalRef<jobject> ref(this->env_, this->obj_);
        ASSERT_TRUE(ref.get() == this->obj_);
        jobject obj3 = ref.release();
        EXPECT_TRUE(ref.get() == nullptr);
        EXPECT_EQ(this->obj_, obj3);
    }
    EXPECT_FALSE(this->env_->IsSameObject(this->obj_, this->obj2_));
    EXPECT_TRUE(this->deleted_.empty());
}

TYPED_TEST(ScopedLocalRefTest, MoveAssignment) {
    ScopedLocalRef<jobject> ref(this->env_, this->obj_);
    ASSERT_EQ(ref.get(), this->obj_);
    ScopedLocalRef<jobject> ref2(this->env_, this->obj2_);
    ASSERT_EQ(ref2.get(), this->obj2_);

    ref = std::move(ref2);

    EXPECT_EQ(ref.get(), this->obj2_);
    EXPECT_TRUE(ref2.get() == nullptr);

    EXPECT_EQ(1u, this->deleted_.size());
    EXPECT_TRUE(this->CheckDeleted(0, this->obj_));
}

TYPED_TEST(ScopedLocalRefTest, OperatorEqNull) {
    ScopedLocalRef<jobject> ref(this->env_, this->obj_);
    ASSERT_EQ(ref.get(), this->obj_);

    EXPECT_FALSE(ref == nullptr);

    ref.reset();
    ASSERT_TRUE(ref.get() == nullptr);

    EXPECT_TRUE(ref == nullptr);
}

TYPED_TEST(ScopedLocalRefTest, OperatorNotNull) {
    ScopedLocalRef<jobject> ref(this->env_, this->obj_);
    ASSERT_EQ(ref.get(), this->obj_);

    EXPECT_TRUE(ref != nullptr);

    ref.reset();
    ASSERT_TRUE(ref.get() == nullptr);

    EXPECT_FALSE(ref != nullptr);
}

}  // namespace android
