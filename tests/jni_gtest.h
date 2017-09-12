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

#ifndef LIBNATIVEHELPER_TESTS_JNI_GTEST_H
#define LIBNATIVEHELPER_TESTS_JNI_GTEST_H

#include <memory>

#include <android-base/logging.h>
#include <android-base/stringprintf.h>
#include <gtest/gtest.h>

#include <jni.h>
#include <nativehelper/JniInvocation.h>

namespace android {

// Provider is expected to follow this structure:
//
// class JNIProvider {
// public:
//    static JNIProvider* Create();
//
//    void SetUp();
//    JNIEnv* CreateJNIEnv();
//
//    void DestroyJNIEnv(JNIEnv* env);
//    void TearDown();
// }

template <typename Provider>
class JNITestBase : public testing::Test {
protected:
    JNITestBase() : provider_(Provider::Create()), env_(nullptr), java_vm_(nullptr) {
    }

    void SetUp() override {
        provider_->SetUp();
        env_ = provider_->CreateJNIEnv();
        ASSERT_TRUE(env_ != nullptr);
    }

    void TearDown() override {
        provider_->DestroyJNIEnv(env_);
        provider_->TearDown();
    }

protected:
    std::unique_ptr<Provider> provider_;

    JNIEnv* env_;
    JavaVM* java_vm_;
};

// A mock provider. It is the responsibility of the test to stub out any
// needed functions (all function pointers will be null initially).
class MockJNIProvider {
public:
    static MockJNIProvider* Create() {
        return new MockJNIProvider();
    }

    void SetUp() {
        // Nothing to here.
    }

    // TODO: Spawn threads to allow more envs?
    JNIEnv* CreateJNIEnv() {
        return CreateMockedJNIEnv().release();
    }

    void DestroyJNIEnv(JNIEnv* env) {
        delete env->functions;
        delete env;
    }

    void TearDown() {
        // Nothing to do here.
    }

protected:
    std::unique_ptr<JNIEnv> CreateMockedJNIEnv() {
        JNINativeInterface* inf = new JNINativeInterface();
        memset(inf, 0, sizeof(JNINativeInterface));

        std::unique_ptr<JNIEnv> ret(new JNIEnv());
        ret->functions = inf;

        return ret;
    }
};

// Note: tests with the JniInvocationProvider cannot be sharded, as
//       JniInvocation is a singleton.
class JniInvocationProvider {
public:
    virtual ~JniInvocationProvider() {}

    virtual void SetUp() {
        if (jni_invocation_ == nullptr) {
            jni_invocation_.reset(new JniInvocation());
            std::string lib = GetLibrary();
            jni_invocation_->Init(lib.empty() ? nullptr : lib.c_str());
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

        jint create_result = JNI_CreateJavaVM(&local_java_vm_, &local_env_, &initArgs);
        ASSERT_EQ(JNI_OK, create_result);
    }

    // TODO: Spawn threads to allow more envs?
    JNIEnv* CreateJNIEnv() {
        return local_env_;
    }

    void DestroyJNIEnv(JNIEnv* env __attribute__((unused))) {
        // TODO: When CreateJNIEnv is fully implemented, detach here.
    }

    virtual void TearDown() {
        jint destroy_result = local_java_vm_->DestroyJavaVM();
        ASSERT_EQ(JNI_OK, destroy_result);
    }

protected:
    JNIEnv* local_env_;
    JavaVM* local_java_vm_;

    virtual std::vector<std::string> GetVMOptions() = 0;
    virtual std::string GetLibrary() {
        return "";
    }

private:
    std::unique_ptr<JniInvocation> jni_invocation_;
};

// A JniInvocationProvider doing ART-specific work.
class ARTJniInvocationProvider : public JniInvocationProvider {
public:
    static ARTJniInvocationProvider* Create() {
        return new ARTJniInvocationProvider();
    }

    void SetUp() override {
        SetUpAndroidRoot();
        SetUpAndroidData();

        JniInvocationProvider::SetUp();
    }

    std::vector<std::string> GetVMOptions() override {
        // GetBootClasspath(): ART may require a boot classpath (if no default
        //   boot image is available).
        // "-Xnoimage-dex2oat": if no boot image is available, don't attempt to
        //   compile one, it's unnecessary for tests and would be done for each
        //   single test.
        return std::vector<std::string>({GetBootClasspath(), "-Xnoimage-dex2oat"});
    }

    std::string GetLibrary() {
        return "libart.so";
    }

    void TearDown() override {
        JniInvocationProvider::TearDown();
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
        // Ensure ANDROID_ROOT is available as an environment variable, as
        // ART relies on it. Also store the value into android_root_.
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
class RIJniInvocationProvider : public JniInvocationProvider {
protected:
    static RIJniInvocationProvider* Create() {
        return new RIJniInvocationProvider();
    }

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

}  // namespace android

#endif  // LIBNATIVEHELPER_TESTS_JNI_GTEST_H
