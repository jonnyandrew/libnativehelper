/*
 * Copyright 2018 The Android Open Source Project
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

#pragma once

#include "jni.h"

struct JniConstants {
    // java.io.FileDescriptor
    static jclass fileDescriptorClass;

    // java.io.FileDescriptor.descriptor.
    static jfieldID fileDescriptorDescriptorField;

    // java.io.FileDescriptor.ownerId.
    static jfieldID fileDescriptorOwnerIdField;

    // void java.io.FileDescriptor.<init>().
    static jmethodID fileDescriptorInitMethod;

    // java.lang.ref.Reference
    static jclass referenceClass;

    // Object java.lang.ref.Reference.get()
    static jmethodID referenceGetMethod;

    // java.lang.String
    static jclass stringClass;

    // Ensure constants are initialized before use.
    static void Initialize(JNIEnv* env);

    // Ensure any cached heap objects from previous VM instances are
    // invalidated. There is no notification here that a VM is destroyed. These
    // cached objects limit us to one VM instance per process.
    static void Uninitialize();
};
