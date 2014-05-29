/*
 * Copyright (C) 2014 The Android Open Source Project
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

#ifndef _NATIVE_BRIDGE_H_
#define _NATIVE_BRIDGE_H_

#ifdef __cplusplus
namespace nativebridge {
#endif

#define SYS_LIB_PATH        "/system/lib/"
#define SYS_LIB_PATH_LEN    (sizeof(SYS_LIB_PATH) - 1)

#define SYS_LIB64_PATH      "/system/lib64/"
#define SYS_LIB64_PATH_LEN  (sizeof(SYS_LIB64_PATH) - 1)

#define IS_64BIT_PROC()     ((sizeof(void*) == 8)?true:false)

#define PROP_LIB_NB     "persist.sys.native.bridge"
#define PROP_ENABLE_NB  "persist.enable.native.bridge"
#define PROP_LIB_VM     "persist.sys.dalvik.vm.lib.1"

#define VM_GET_SHORTY_SYM   "GetMethodShorty"

/*
 * common native bridge interfaces
 */

typedef bool (*NB_ITF_INIT)(void *);

typedef void* (*NB_ITF_DLOPEN)(const char *libpath, int flag);

typedef void* (*NB_ITF_DLSYM)(void* handle, const char* symbol);

typedef bool (*NB_ITF_IS_SUPPORTED)(const char * libpath);

/*
 * native bridge interfaces for vm
 */

#define NB_VM_ITF_SYM      "native_bridge_vm_itf"

typedef void (*NB_ITF_INVOKE)(void* pEnv, void* clazz, int argInfo, int argc,
            const int* argv, const char* shorty, void* func, void* pReturn);

typedef int (*NB_ITF_JNI_ONLOAD)(void* func, void* jniVm, void* arg);

typedef bool (*NB_ITF_IS_NEEDED)(void *fnPtr);

typedef struct nb_vm_itf_s {
    NB_ITF_INIT         init;
    NB_ITF_DLOPEN       dlopen;
    NB_ITF_DLSYM        dlsym;
    NB_ITF_INVOKE       invoke;
    NB_ITF_JNI_ONLOAD   jniOnLoad;
    NB_ITF_IS_NEEDED    isNeeded;
    NB_ITF_IS_SUPPORTED isSupported;
} nb_vm_itf_t;


/*
 * native bridge interface for native activity
 */

#define NB_NA_ITF_SYM      "native_bridge_na_itf"

typedef void (*NB_ITF_CREAT_ACT)(void* createActivityFunc,
        void* activity, void* savedState, size_t savedStateSize);

typedef struct nb_na_itf_s {
    NB_ITF_INIT         init;
    NB_ITF_DLOPEN       dlopen;
    NB_ITF_DLSYM        dlsym;
    NB_ITF_CREAT_ACT    createActivity;
    NB_ITF_IS_SUPPORTED isSupported;
} nb_na_itf_t;

#ifdef __cplusplus
};
#endif

#endif
