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

#include <android-base/logging.h>
#include <android-base/stringprintf.h>
#include <gtest/gtest.h>

#include <jni.h>
#include <nativehelper/JniConstants.h>
#include <nativehelper/JniInvocation.h>

namespace android {

// Base test class starting and setting up test objects.
class ScopedLocalRefTest : public testing::Test {
public:
    void SetUp() override {
        if (!sInitialized) {
            // We'll leak this. It's an internal pointer in the
            // JniInvocation singleton code.
            JniInvocation* jni_invocation = new JniInvocation();
            std::string lib = GetLibrary();
            jni_invocation->Init(lib.empty() ? nullptr : lib.c_str());
            // Must not do this twice, only one singleton allowed.
            sInitialized = true;
        }

        std::vector<std::string> options = GetVMOptions();
        std::unique_ptr<JavaVMOption[]> options_uptr;
        if (!options.empty()) {
            options_uptr.reset(new JavaVMOption[options.size()]);
            for (size_t i = 0; i != options.size(); ++i) {
                options_uptr[i].optionString = options[i].c_str();
            }
        }

        JavaVMInitArgs initArgs;
        initArgs.version = JNI_VERSION_1_2;
        initArgs.nOptions = static_cast<jint>(options.size());
        initArgs.options = options_uptr.get();
        initArgs.ignoreUnrecognized = true;

        jint create_result = JNI_CreateJavaVM(&java_vm, &env, &initArgs);
        ASSERT_EQ(JNI_OK, create_result);

        obj = env->NewStringUTF("Test");
        ASSERT_TRUE(obj != nullptr);

        obj2 = env->NewStringUTF("Test2");
        ASSERT_TRUE(obj2 != nullptr);
        ASSERT_TRUE(obj != obj2);
        ASSERT_FALSE(env->IsSameObject(obj, obj2));
    }

    void TearDown() override {
        jint destroy_result = java_vm->DestroyJavaVM();
        ASSERT_EQ(JNI_OK, destroy_result);
    }

protected:
    JNIEnv* env;
    JavaVM* java_vm;
    jobject obj;
    jobject obj2;

    virtual std::vector<std::string> GetVMOptions() = 0;
    virtual std::string GetLibrary() {
        return "";
    }

private:
    static bool sInitialized;
};
bool ScopedLocalRefTest::sInitialized = false;

// A test class doing ART-specific work.
class ARTScopedLocalRefTest : public ScopedLocalRefTest {
public:
    void SetUp() override {
        JniConstants::clear();

        SetUpAndroidRoot();
        SetUpAndroidData();

        ScopedLocalRefTest::SetUp();
    }

    std::vector<std::string> GetVMOptions() override {
        return std::vector<std::string>({GetBootClasspath(), "-Xnoimage-dex2oat"});
    }

    std::string GetLibrary() {
        return "libart.so";
    }

    void TearDown() override {
        ScopedLocalRefTest::TearDown();
        TearDownAndroidData();
    }

private:
    // This is necessary for ART, and copied from common_runtime_test.
    static constexpr bool kIsTargetBuild =
#ifdef TARGET_BUILD
            true;
#else
            false;
#endif
    void SetUpAndroidRoot() {
        const char* android_root_from_env = getenv("ANDROID_ROOT");
        if (!kIsTargetBuild) {
            // $ANDROID_ROOT is set on the device, but not necessarily on the host.
            // But it needs to be set so that icu4c can find its locale data.
            if (android_root_from_env == nullptr) {
                // Use ANDROID_HOST_OUT for ANDROID_ROOT if it is set.
                const char* android_host_out = getenv("ANDROID_HOST_OUT");
                if (android_host_out != nullptr) {
                    setenv("ANDROID_ROOT", android_host_out, 1);
                    android_root = android_host_out;
                } else {
                    // Build it from ANDROID_BUILD_TOP or cwd
                    std::string root;
                    const char* android_build_top = getenv("ANDROID_BUILD_TOP");
                    if (android_build_top != nullptr) {
                        root += android_build_top;
                    } else {
                        // Not set by build server, so default to current directory
                        char* cwd = getcwd(nullptr, 0);
                        setenv("ANDROID_BUILD_TOP", cwd, 1);
                        root += cwd;
                        free(cwd);
                    }
                    root += "/out/host/linux-x86";
                    setenv("ANDROID_ROOT", root.c_str(), 1);
                }
            } else {
                android_root = android_root_from_env;
            }
            setenv("LD_LIBRARY_PATH", ":", 0);  // Required by java.lang.System.<clinit>.

            // Not set by build server, so default
            if (getenv("ANDROID_HOST_OUT") == nullptr) {
                setenv("ANDROID_HOST_OUT", getenv("ANDROID_ROOT"), 1);
            }
        } else {
            ASSERT_TRUE(android_root_from_env != nullptr);
            android_root = android_root_from_env;
        }
    }

    void SetUpAndroidData() {
        // On target, Cannot use /mnt/sdcard because it is mounted noexec, so use subdir of
        // dalvik-cache.
        if (!kIsTargetBuild) {
            const char* tmpdir = getenv("TMPDIR");
            if (tmpdir != nullptr && tmpdir[0] != 0) {
                android_data = tmpdir;
            } else {
                android_data = "/tmp";
            }
            android_data += "/art-data-XXXXXX";
            if (mkdtemp(&android_data[0]) == nullptr) {
                ASSERT_TRUE(false) << "mkdtemp(\"" << &android_data[0] << "\") failed";
            }
        } else {
            android_data = "/data";
        }

        setenv("ANDROID_DATA", android_data.c_str(), 1);
    }

    void TearDownAndroidData() {
        if (!kIsTargetBuild) {
            ClearDirectory(android_data.c_str());
            ASSERT_EQ(rmdir(android_data.c_str()), 0) << android_data;
        }
    }

    static void ClearDirectory(const char* dirpath) {
        ASSERT_TRUE(dirpath != nullptr);
        DIR* dir = opendir(dirpath);
        ASSERT_TRUE(dir != nullptr);
        dirent* e;
        struct stat s;
        while ((e = readdir(dir)) != nullptr) {
            if ((strcmp(e->d_name, ".") == 0) || (strcmp(e->d_name, "..") == 0)) {
                continue;
            }
            std::string filename(dirpath);
            filename.push_back('/');
            filename.append(e->d_name);
            int stat_result = lstat(filename.c_str(), &s);
            ASSERT_EQ(0, stat_result) << "unable to stat " << filename;
            if (S_ISDIR(s.st_mode)) {
                ClearDirectory(filename.c_str());
                int rmdir_result = rmdir(filename.c_str());
                ASSERT_EQ(0, rmdir_result) << filename;
            } else {
                int unlink_result = unlink(filename.c_str());
                ASSERT_EQ(0, unlink_result) << filename;
            }
        }
        closedir(dir);
    }
    std::string GetDexFileName(const std::string& jar_prefix) {
        std::string path;
        if (!kIsTargetBuild) {
            const char* host_dir = getenv("ANDROID_HOST_OUT");
            CHECK(host_dir != nullptr);
            path = host_dir;
        } else {
            path = android_root;
        }

        std::string suffix = !kIsTargetBuild
                ? "-hostdex"                 // The host version.
                : "-testdex";                // The unstripped target version.

        return base::StringPrintf("%s/framework/%s%s.jar", path.c_str(), jar_prefix.c_str(),
                            suffix.c_str());
    }

    std::string GetBootClasspath() {
        std::string boot_class_path_string = "-Xbootclasspath";
        std::vector<std::string> boot_cp({GetDexFileName("core-oj"),
                                          GetDexFileName("core-libart")});
        for (const std::string &core_dex_file_name : boot_cp) {
             boot_class_path_string += ":";
             boot_class_path_string += core_dex_file_name;
        }
        return boot_class_path_string;
    }

    std::string android_data;
    std::string android_root;
};

#ifndef TARGET_BUILD

// A test class configuring for the RI.
// TODO: This seems to not handle repeated JNI_CreateJavaVM. :-(
class RIScopedLocalRefTest : public ScopedLocalRefTest {
protected:
    std::vector<std::string> GetVMOptions() override {
        return std::vector<std::string>();
    }

    std::string GetLibrary() {
        // Use the prebuilt JDK shipped in the tree.
        const char* android_build_top = getenv("ANDROID_BUILD_TOP");
        std::string top;
        if (android_build_top != nullptr) {
            top = android_build_top;
        } else {
            // Not set by build server, so default to current directory
            char* cwd = getcwd(nullptr, 0);
            top = cwd;
            free(cwd);
        }
        return top + "/prebuilts/jdk/jdk8/linux-x86/jre/lib/amd64/server/libjvm.so";
    }
};

#endif

using ConfiguredTest = ARTScopedLocalRefTest;

TEST_F(ConfiguredTest, EmptyConstructor) {
    ScopedLocalRef<jobject> ref(env);
    EXPECT_TRUE(ref.get() == nullptr);
}

TEST_F(ConfiguredTest, Constructor) {
    ScopedLocalRef<jobject> ref(env, obj);
    EXPECT_EQ(ref.get(), obj);
}

TEST_F(ConfiguredTest, MoveConstructor) {
    ScopedLocalRef<jobject> ref(env, obj);
    ASSERT_EQ(ref.get(), obj);

    ScopedLocalRef<jobject> moved_ref(std::move(ref));
    EXPECT_EQ(moved_ref.get(), obj);
    EXPECT_TRUE(ref.get() == nullptr);
}

TEST_F(ConfiguredTest, Reset) {
    ScopedLocalRef<jobject> ref(env);
    ASSERT_TRUE(ref.get() == nullptr);
    ref.reset(obj);
    EXPECT_EQ(ref.get(), obj);
    ref.reset(obj2);
    EXPECT_EQ(ref.get(), obj2);
    ref.reset();
    EXPECT_TRUE(ref.get() == nullptr);
}

TEST_F(ConfiguredTest, Release) {
    // In a scope to have a simple non-death test.
    {
        ScopedLocalRef<jobject> ref(env, obj);
        ASSERT_TRUE(ref.get() == obj);
        jobject obj3 = ref.release();
        EXPECT_TRUE(ref.get() == nullptr);
        EXPECT_EQ(obj, obj3);
    }
    EXPECT_FALSE(env->IsSameObject(obj, obj2));
}

TEST_F(ConfiguredTest, MoveAssignment) {
    ScopedLocalRef<jobject> ref(env, obj);
    ASSERT_EQ(ref.get(), obj);
    ScopedLocalRef<jobject> ref2(env, obj2);
    ASSERT_EQ(ref2.get(), obj2);

    ref = std::move(ref2);

    EXPECT_EQ(ref.get(), obj2);
    EXPECT_TRUE(ref2.get() == nullptr);
}

TEST_F(ConfiguredTest, OperatorEqNull) {
    ScopedLocalRef<jobject> ref(env, obj);
    ASSERT_EQ(ref.get(), obj);

    EXPECT_FALSE(ref == nullptr);

    ref.reset();
    ASSERT_TRUE(ref.get() == nullptr);

    EXPECT_TRUE(ref == nullptr);
}

TEST_F(ConfiguredTest, OperatorNotNull) {
    ScopedLocalRef<jobject> ref(env, obj);
    ASSERT_EQ(ref.get(), obj);

    EXPECT_TRUE(ref != nullptr);

    ref.reset();
    ASSERT_TRUE(ref.get() == nullptr);

    EXPECT_FALSE(ref != nullptr);
}

}  // namespace android
