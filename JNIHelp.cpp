/*
 * Copyright (C) 2006 The Android Open Source Project
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

#define LOG_TAG "JNIHelp"

#include "nativehelper/JNIHelp.h"

#include "ALog-priv.h"

#include <assert.h>
#include <atomic>
#include <mutex>
#include <stdio.h>
#include <stdlib.h>
#include <string>

namespace {

/**
 * Equivalent to ScopedLocalRef, but for C_JNIEnv instead. (And slightly more powerful.)
 */
template<typename T>
class scoped_local_ref {
public:
    explicit scoped_local_ref(JNIEnv* env, T localRef = NULL)
    : mEnv(env), mLocalRef(localRef)
    {
    }

    ~scoped_local_ref() {
        reset();
    }

    void reset(T localRef = nullptr) {
        if (mLocalRef != nullptr) {
            mEnv->DeleteLocalRef(mLocalRef);
            mLocalRef = localRef;
        }
    }

    T get() const {
        return mLocalRef;
    }

private:
    JNIEnv* const mEnv;
    T mLocalRef;

    DISALLOW_COPY_AND_ASSIGN(scoped_local_ref);
};

class Constants final {
  public:
    // java.io.FileDescriptor
    static jclass fileDescriptorClass;
    // java.io.FileDescriptor.descriptor.
    static jfieldID fileDescriptorDescriptorField;
    // java.io.FileDescriptor.ownerId.
    static jfieldID fileDescriptorOwnerIdField;
    // void java.io.FileDescriptor.<init>().
    static jmethodID fileDescriptorInitMethod;
    // Object java.lang.ref.Reference.get()
    static jmethodID referenceGetMethod;

    static void EnsureInitialized(JNIEnv* env) {
        if (gInitialized) {
            return;
        }

        std::lock_guard<std::mutex> guard(gInitializerMutex);
        if (gInitialized) {
            return;
        }

        scoped_local_ref<jclass> localRef(env, FindClassOrDie(env, "java/io/FileDescriptor"));
        fileDescriptorClass = reinterpret_cast<jclass>(env->NewGlobalRef(localRef.get()));
        fileDescriptorDescriptorField = FindFieldOrDie(env, fileDescriptorClass, "descriptor", "I");
        fileDescriptorOwnerIdField = FindFieldOrDie(env, fileDescriptorClass, "ownerId", "J");
        fileDescriptorInitMethod = FindMethodOrDie(env, fileDescriptorClass, "<init>", "()V");

        jclass referenceClass = FindClassOrDie(env, "java/lang/ref/Reference");
        referenceGetMethod = FindMethodOrDie(env, referenceClass, "get", "()Ljava/lang/Object;");
    }

  private:
    static jclass FindClassOrDie(JNIEnv* env, const char* name) {
        jclass result = env->FindClass(name);
        if (result == NULL) {
            ALOGV("failed to find class '%s'", name);
            abort();
        }
        return result;
    }

    static jfieldID FindFieldOrDie(JNIEnv* env, jclass klass, const char* name, const char* desc) {
        jfieldID result = env->GetFieldID(klass, name, desc);
        if (result == NULL) {
            ALOGV("failed to find field '%s:%s'", name, desc);
            abort();
        }
        return result;
    }

    static jmethodID FindMethodOrDie(JNIEnv* env, jclass klass, const char* name, const char* signature) {
        jmethodID result = env->GetMethodID(klass, name, signature);
        if (result == NULL) {
            ALOGV("failed to find method '%s%s'", name, signature);
            abort();
        }
        return result;
    }

    static std::atomic<bool> gInitialized;
    static std::mutex gInitializerMutex;

    // DISALLOW_IMPLICIT_CONSTRUCTORS() - no dependency on android-base here.
    DISALLOW_COPY_AND_ASSIGN(Constants);
    Constants() = delete;
};

jclass Constants::fileDescriptorClass;
jfieldID Constants::fileDescriptorDescriptorField;
jfieldID Constants::fileDescriptorOwnerIdField;
jmethodID Constants::fileDescriptorInitMethod;
jmethodID Constants::referenceGetMethod;

std::mutex Constants::gInitializerMutex;
std::atomic<bool> Constants::gInitialized(false);

}

extern "C" int jniRegisterNativeMethods(C_JNIEnv* env, const char* className,
    const JNINativeMethod* gMethods, int numMethods)
{
    JNIEnv* e = reinterpret_cast<JNIEnv*>(env);

    ALOGV("Registering %s's %d native methods...", className, numMethods);

    scoped_local_ref<jclass> c(e, e->FindClass(className));
    if (c.get() == NULL) {
        char* tmp;
        const char* msg;
        if (asprintf(&tmp,
                     "Native registration unable to find class '%s'; aborting...",
                     className) == -1) {
            // Allocation failed, print default warning.
            msg = "Native registration unable to find class; aborting...";
        } else {
            msg = tmp;
        }
        e->FatalError(msg);
    }

    if (e->RegisterNatives(c.get(), gMethods, numMethods) < 0) {
        char* tmp;
        const char* msg;
        if (asprintf(&tmp, "RegisterNatives failed for '%s'; aborting...", className) == -1) {
            // Allocation failed, print default warning.
            msg = "RegisterNatives failed; aborting...";
        } else {
            msg = tmp;
        }
        e->FatalError(msg);
    }

    return 0;
}

/*
 * Returns a human-readable summary of an exception object.  The buffer will
 * be populated with the "binary" class name and, if present, the
 * exception message.
 */
static bool getExceptionSummary(C_JNIEnv* env, jthrowable exception, std::string& result) {
    JNIEnv* e = reinterpret_cast<JNIEnv*>(env);

    /* get the name of the exception's class */
    scoped_local_ref<jclass> exceptionClass(e, e->GetObjectClass(exception)); // can't fail
    scoped_local_ref<jclass> classClass(e, e->GetObjectClass(exceptionClass.get())); // java.lang.Class, can't fail
    jmethodID classGetNameMethod = e->GetMethodID(classClass.get(), "getName", "()Ljava/lang/String;");
    scoped_local_ref<jstring> classNameStr(e,
            (jstring) e->CallObjectMethod(exceptionClass.get(), classGetNameMethod));
    if (classNameStr.get() == NULL) {
        e->ExceptionClear();
        result = "<error getting class name>";
        return false;
    }
    const char* classNameChars = e->GetStringUTFChars(classNameStr.get(), NULL);
    if (classNameChars == NULL) {
        e->ExceptionClear();
        result = "<error getting class name UTF-8>";
        return false;
    }
    result += classNameChars;
    e->ReleaseStringUTFChars(classNameStr.get(), classNameChars);

    /* if the exception has a detail message, get that */
    jmethodID getMessage = e->GetMethodID(exceptionClass.get(),
                                          "getMessage",
                                          "()Ljava/lang/String;");
    scoped_local_ref<jstring> messageStr(e, (jstring) e->CallObjectMethod(exception, getMessage));
    if (messageStr.get() == NULL) {
        return true;
    }

    result += ": ";

    const char* messageChars = e->GetStringUTFChars(messageStr.get(), NULL);
    if (messageChars != NULL) {
        result += messageChars;
        e->ReleaseStringUTFChars(messageStr.get(), messageChars);
    } else {
        result += "<error getting message>";
        e->ExceptionClear(); // clear OOM
    }

    return true;
}

/*
 * Returns an exception (with stack trace) as a string.
 */
static bool getStackTrace(C_JNIEnv* env, jthrowable exception, std::string& result) {
    JNIEnv* e = reinterpret_cast<JNIEnv*>(env);

    scoped_local_ref<jclass> stringWriterClass(e, e->FindClass("java/io/StringWriter"));
    if (stringWriterClass.get() == NULL) {
        return false;
    }

    jmethodID stringWriterCtor = e->GetMethodID(stringWriterClass.get(), "<init>", "()V");
    jmethodID stringWriterToStringMethod =
        e->GetMethodID(stringWriterClass.get(), "toString", "()Ljava/lang/String;");

    scoped_local_ref<jclass> printWriterClass(e, e->FindClass("java/io/PrintWriter"));
    if (printWriterClass.get() == NULL) {
        return false;
    }

    jmethodID printWriterCtor =
        e->GetMethodID(printWriterClass.get(), "<init>", "(Ljava/io/Writer;)V");

    scoped_local_ref<jobject> stringWriter(e, e->NewObject(stringWriterClass.get(), stringWriterCtor));
    if (stringWriter.get() == NULL) {
        return false;
    }

    scoped_local_ref<jobject> printWriter(e, e->NewObject(printWriterClass.get(), printWriterCtor, stringWriter.get()));
    if (printWriter.get() == NULL) {
        return false;
    }

    scoped_local_ref<jclass> exceptionClass(e, e->GetObjectClass(exception)); // can't fail
    jmethodID printStackTraceMethod =
        e->GetMethodID(exceptionClass.get(), "printStackTrace", "(Ljava/io/PrintWriter;)V");
    e->CallVoidMethod(exception, printStackTraceMethod, printWriter.get());

    if (e->ExceptionCheck()) {
        return false;
    }

    scoped_local_ref<jstring> messageStr(e, (jstring) e->CallObjectMethod(stringWriter.get(),
                                                                          stringWriterToStringMethod));
    if (messageStr.get() == NULL) {
        return false;
    }

    const char* utfChars = e->GetStringUTFChars(messageStr.get(), NULL);
    if (utfChars == NULL) {
        return false;
    }

    result = utfChars;

    e->ReleaseStringUTFChars(messageStr.get(), utfChars);
    return true;
}

extern "C" int jniThrowException(C_JNIEnv* env, const char* className, const char* msg) {
    JNIEnv* e = reinterpret_cast<JNIEnv*>(env);

    if (e->ExceptionCheck()) {
        /* TODO: consider creating the new exception with this as "cause" */
        scoped_local_ref<jthrowable> exception(e, e->ExceptionOccurred());
        e->ExceptionClear();

        if (exception.get() != NULL) {
            std::string text;
            getExceptionSummary(env, exception.get(), text);
            ALOGW("Discarding pending exception (%s) to throw %s", text.c_str(), className);
        }
    }

    scoped_local_ref<jclass> exceptionClass(e, e->FindClass(className));
    if (exceptionClass.get() == NULL) {
        ALOGE("Unable to find exception class %s", className);
        /* ClassNotFoundException now pending */
        return -1;
    }

    if (e->ThrowNew(exceptionClass.get(), msg) != JNI_OK) {
        ALOGE("Failed throwing '%s' '%s'", className, msg);
        /* an exception, most likely OOM, will now be pending */
        return -1;
    }

    return 0;
}

int jniThrowExceptionFmt(C_JNIEnv* env, const char* className, const char* fmt, va_list args) {
    char msgBuf[512];
    vsnprintf(msgBuf, sizeof(msgBuf), fmt, args);
    return jniThrowException(env, className, msgBuf);
}

int jniThrowNullPointerException(C_JNIEnv* env, const char* msg) {
    return jniThrowException(env, "java/lang/NullPointerException", msg);
}

int jniThrowRuntimeException(C_JNIEnv* env, const char* msg) {
    return jniThrowException(env, "java/lang/RuntimeException", msg);
}

int jniThrowIOException(C_JNIEnv* env, int errnum) {
    char buffer[80];
    const char* message = jniStrError(errnum, buffer, sizeof(buffer));
    return jniThrowException(env, "java/io/IOException", message);
}

static std::string jniGetStackTrace(C_JNIEnv* env, jthrowable exception) {
    JNIEnv* e = reinterpret_cast<JNIEnv*>(env);

    scoped_local_ref<jthrowable> currentException(e, e->ExceptionOccurred());
    if (exception == NULL) {
        exception = currentException.get();
        if (exception == NULL) {
          return "<no pending exception>";
        }
    }

    if (currentException.get() != NULL) {
        e->ExceptionClear();
    }

    std::string trace;
    if (!getStackTrace(env, exception, trace)) {
        e->ExceptionClear();
        getExceptionSummary(env, exception, trace);
    }

    if (currentException.get() != NULL) {
        e->Throw(currentException.get()); // rethrow
    }

    return trace;
}

void jniLogException(C_JNIEnv* env, int priority, const char* tag, jthrowable exception) {
    std::string trace(jniGetStackTrace(env, exception));
    __android_log_write(priority, tag, trace.c_str());
}

// Note: glibc has a nonstandard strerror_r that returns char* rather than POSIX's int.
// char *strerror_r(int errnum, char *buf, size_t n);
//
// Some versions of bionic support the glibc style call. Since the set of defines that determine
// which version is used is byzantine in its complexity we will just use this C++ template hack to
// select the correct jniStrError implementation based on the libc being used.
namespace impl {

using GNUStrError = char* (*)(int,char*,size_t);
using POSIXStrError = int (*)(int,char*,size_t);

inline const char* realJniStrError(GNUStrError func, int errnum, char* buf, size_t buflen) {
    return func(errnum, buf, buflen);
}

inline const char* realJniStrError(POSIXStrError func, int errnum, char* buf, size_t buflen) {
    int rc = func(errnum, buf, buflen);
    if (rc != 0) {
        // (POSIX only guarantees a value other than 0. The safest
        // way to implement this function is to use C++ and overload on the
        // type of strerror_r to accurately distinguish GNU from POSIX.)
        snprintf(buf, buflen, "errno %d", errnum);
    }
    return buf;
}

}  // namespace impl

const char* jniStrError(int errnum, char* buf, size_t buflen) {
  // The magic of C++ overloading selects the correct implementation based on the declared type of
  // strerror_r. The inline will ensure that we don't have any indirect calls.
  return impl::realJniStrError(strerror_r, errnum, buf, buflen);
}

jobject jniCreateFileDescriptor(C_JNIEnv* env, int fd) {
    JNIEnv* e = reinterpret_cast<JNIEnv*>(env);
    Constants::EnsureInitialized(e);
    jobject fileDescriptor = e->NewObject(Constants::fileDescriptorClass,
                                          Constants::fileDescriptorInitMethod);
    // NOTE: NewObject ensures that an OutOfMemoryError will be seen by the Java
    // caller if the alloc fails, so we just return NULL when that happens.
    if (fileDescriptor != nullptr)  {
        jniSetFileDescriptorOfFD(env, fileDescriptor, fd);
    }
    return fileDescriptor;
}

int jniGetFDFromFileDescriptor(C_JNIEnv* env, jobject fileDescriptor) {
    JNIEnv* e = reinterpret_cast<JNIEnv*>(env);
    Constants::EnsureInitialized(e);

    if (fileDescriptor != nullptr) {
        return e->GetIntField(fileDescriptor, Constants::fileDescriptorDescriptorField);
    } else {
        return -1;
    }
}

void jniSetFileDescriptorOfFD(C_JNIEnv* env, jobject fileDescriptor, int value) {
    JNIEnv* e = reinterpret_cast<JNIEnv*>(env);
    Constants::EnsureInitialized(e);
    if (fileDescriptor != nullptr) {
        e->SetIntField(fileDescriptor, Constants::fileDescriptorDescriptorField, value);
    } else {
        jniThrowNullPointerException(e, "null FileDescriptor");
    }
}

jlong jniGetOwnerIdFromFileDescriptor(C_JNIEnv* env, jobject fileDescriptor) {
    JNIEnv* e = reinterpret_cast<JNIEnv*>(env);
    Constants::EnsureInitialized(e);
    return e->GetLongField(fileDescriptor, Constants::fileDescriptorOwnerIdField);
}

jobject jniGetReferent(C_JNIEnv* env, jobject ref) {
    JNIEnv* e = reinterpret_cast<JNIEnv*>(env);
    Constants::EnsureInitialized(e);
    return e->CallObjectMethod(ref, Constants::referenceGetMethod);
}

jstring jniCreateString(C_JNIEnv* env, const jchar* unicodeChars, jsize len) {
    JNIEnv* e = reinterpret_cast<JNIEnv*>(env);
    return e->NewString(unicodeChars, len);
}
