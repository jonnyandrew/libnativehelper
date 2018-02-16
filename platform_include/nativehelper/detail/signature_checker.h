/*
 * Copyright (C) 2018 The Android Open Source Project
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

// FIXME: more documentation

#pragma once

#include <jni.h>
#include <exception>

#include <array>  // required for MakeArray.
#include <type_traits>  // std::common_type

#include <iostream>

void jni_assert(bool val, const char* /*msg*/) {
  if (!val) {
    std::terminate();
  }
}
#define JNI_ASSERT(x) jni_assert(x, #x)

#ifndef NDEBUG
#undef NDEBUG
#endif

// #define NDEBUG

// If CHECK evaluates to false then X_ASSERT will halt compilation.
//
// Asserts meant to be used only within constexpr context.
// The runtime 'jni_assert' will never get called; instead compilation will abort.
#if defined NDEBUG
# define X_ASSERT(CHECK) do { if ((false)) { (CHECK) ? void(0) : void(0); } } while(false)
#else
# define X_ASSERT(CHECK) \
    ( (CHECK) ? void(0) : JNI_ASSERT(!#CHECK) )
#endif

// An immutable constexpr string, similar to std::string_view but for C++14.
// For a mutable string see instead ConstexprVector<char>.
struct ConstexprStringErased {
 private:
  const char* _array;  // Never-null for simplicity.
  size_t _size;
 public:
  template <size_t N>
  constexpr ConstexprStringErased(const char (&lit)[N]) : _array(lit), _size(N-1) {  // NOLINT: explicit.
    // Using an array of characters is not allowed because the inferred size would be wrong.
    // Use the other constructor instead for that.
    X_ASSERT(lit[N-1] == '\0');
  }

  constexpr ConstexprStringErased(const char* ptr, size_t size) : _array(ptr), _size(size) {
    // See the below constructor instead.
    X_ASSERT(ptr != nullptr);
  }

  /*explicit constexpr ConstexprStringErased(const char* ptr) : _array(ptr), _size(0u) {
    X_ASSERT(ptr != nullptr);

    while (*ptr != '\0') {
      ++_size;
      ++ptr;
    }
  }*/

  explicit constexpr ConstexprStringErased(const decltype(nullptr)&) : _array(""), _size(0u) {
  }

  constexpr ConstexprStringErased() : _array(""), _size(0u) {}

  constexpr size_t size() const {
    return _size;
  }

  constexpr bool empty() const {
    return size() == 0u;
  }

  constexpr char operator[](size_t i) const {
    X_ASSERT(i <= size());
    return _array[i];
  }

  constexpr ConstexprStringErased substr(size_t start, size_t len) const {
    X_ASSERT(start <= size());
    X_ASSERT(start + len <= size());

    return ConstexprStringErased(&_array[start], len);
  }

  constexpr ConstexprStringErased substr(size_t start) const {
    X_ASSERT(start <= size());
    return substr(start, size() - start);
  }

  using const_iterator = const char*;

  constexpr const_iterator begin() const {
    return &_array[0];
  }

  constexpr const_iterator end() const {
    return &_array[size()];
  }
};

constexpr bool operator==(const ConstexprStringErased& lhs, const ConstexprStringErased& rhs) {
  if (lhs.size() != rhs.size()) {
    return false;
  }
  for (size_t i = 0; i < lhs.size(); ++i) {
    if (lhs[i] != rhs[i]) {
      return false;
    }
  }
  return true;
}

constexpr bool operator!=(const ConstexprStringErased& lhs, const ConstexprStringErased& rhs) {
  return !(lhs == rhs);
}

std::ostream& operator<<(std::ostream& os, const ConstexprStringErased& str) {
  for (char c : str) {
    os << c;
  }
  return os;
}

template <size_t N>
class ConstexprString {
  char _array[N];
 public:
  constexpr char operator[](size_t i) const {
    return _array[i];
  }

  // Note that using "N+1" here will fail type deduction.
  // This is mostly academic since prior to C++17 constructors
  // cannot be type deduced.
  constexpr ConstexprString(const char (& lit)[N])
      : _array() {
    for (size_t i = 0; i < N; ++i) {
      _array[i] = lit[i];
    }
    X_ASSERT(lit[N-1] == '\0');
  }

  constexpr size_t size() const {
    return N-1;
  }

};

template <size_t N>
constexpr ConstexprString<N> MakeConstexprString(const char (& lit)[N]) {
  return ConstexprString<N>(lit);
}

template <size_t SignatureLength>
struct CheckedNativeMethod {
  const char* name;
  ConstexprString<SignatureLength> signature;
  void* fn;
};

// We did not have constructor type inferencing until C++17, so use this make function.
template <size_t SignatureLength>
constexpr CheckedNativeMethod<SignatureLength> MakeCheckedNativeMethod(const char* name, const char (&lit)[SignatureLength], void* fn) {
  return CheckedNativeMethod<SignatureLength>{ name, MakeConstexprString(lit), fn };
}

constexpr bool IsValidJniDescriptorShorty(char shorty) {
  constexpr char kValidJniTypes[] =
      {'V', 'Z', 'B', 'C', 'S', 'I', 'J', 'F', 'D', 'L', '[', '(', ')'};

  for (char c : kValidJniTypes) {
    if (c == shorty) {
      return true;
    }
  }

  return false;
}

template <typename T, size_t kMaxSize>
struct ConstexprVector {
 public:
  constexpr explicit ConstexprVector(size_t initial_size = 0u) : _size(initial_size), _array{} {
    X_ASSERT(initial_size <= kMaxSize);
  }

 private:
  template <typename Elem>
  struct VectorIterator {
    Elem* ptr;

    constexpr VectorIterator& operator++() {
      ++ptr;
      return *this;
    }

    constexpr VectorIterator operator++(int) const {
      VectorIterator tmp(*this);
      ++tmp;
      return tmp;
    }

    constexpr auto& operator*() {
      // Use 'auto' here since using 'T' is incorrect with const_iterator.
      return ptr->_value;
    }

    constexpr const T& operator*() const {
      return ptr->_value;
    }

    constexpr bool operator==(const VectorIterator& other) const {
      return ptr == other.ptr;
    }

    constexpr bool operator!=(const VectorIterator& other) const {
      return !(*this == other);
    }
  };

  // Do not require that T is default-constructible by using a union.
  struct MaybeElement {
    union {
      T _value;
    };
  };

 public:
  using iterator = VectorIterator<MaybeElement>;
  using const_iterator = VectorIterator<const MaybeElement>;

  constexpr iterator begin() {
    return {&_array[0]};
  }

  constexpr iterator end() {
    return {&_array[size()]};
  }

  constexpr const_iterator begin() const {
    return {&_array[0]};
  }

  constexpr const_iterator end() const {
    return {&_array[size()]};
  }

  constexpr void push_back(const T& value) {
    X_ASSERT(_size + 1 <= kMaxSize);

    _array[_size]._value = value;
    _size++;
  }

  constexpr const T& operator[](size_t i) const {
    return _array[i]._value;
  }

  constexpr T& operator[](size_t i) {
    return _array[i]._value;
  }

  constexpr size_t size() const {
    return _size;
  }
 private:

  size_t _size;
  MaybeElement _array[kMaxSize];
};

struct JniDescriptorNode {
  ConstexprStringErased longy;

  constexpr JniDescriptorNode(ConstexprStringErased longy) : longy(longy) {  // NOLINT: explicit.
    X_ASSERT(!longy.empty());
  }
  constexpr JniDescriptorNode() : longy() {}

  constexpr char shorty() {
    // Must be initialized with the non-default constructor.
    X_ASSERT(!longy.empty());
    return longy[0];
  }
};

std::ostream& operator<<(std::ostream& os, const JniDescriptorNode& node) {
  os << node.longy;
  return os;
}

/*
constexpr bool IsSignatureValid(ConstexprStringErased signature) {
  for (size_t i = 0; i < signature.size(); ++i) {
    if (!IsValidJniDescriptorShorty(signature[i])) {
      return false;
    }
  }

  return true;
}
 */

struct ParseTypeDescriptorResult {
  ConstexprStringErased token;
  ConstexprStringErased remainder;

  constexpr bool has_token() const {
    return token.size() > 0u;
  }

  constexpr bool has_remainder() const {
    return remainder.size() > 0u;
  }

  constexpr JniDescriptorNode as_node() const {
    X_ASSERT(has_token());
    return { token };
  }
};

template <typename T>
struct ConstexprOptional {
  constexpr ConstexprOptional() : _has_value(false), _nothing() {
  }

  constexpr ConstexprOptional(const T& value) : _has_value(true), _value(value) {
  }

  constexpr explicit operator bool() const {
    return _has_value;
  }

  constexpr bool has_value() const {
    return _has_value;
  }

  constexpr const T& value() const {
    X_ASSERT(has_value());
    return _value;
  }

  constexpr const T* operator->() const {
    return &(value());
  }

 private:
  bool _has_value;
  struct Nothing {};
  union {
    Nothing _nothing;
    T _value;
  };
};

template <typename T>
constexpr bool operator==(const ConstexprOptional<T>& lhs, const ConstexprOptional<T>& rhs) {
  if (lhs && rhs) {
    return lhs.value() == rhs.value();
  }
  return lhs.has_value() == rhs.has_value();
}

template <typename T>
constexpr bool operator!=(const ConstexprOptional<T>& lhs, const ConstexprOptional<T>& rhs) {
  return ! (lhs == rhs);
}

template <typename T>
std::ostream& operator<<(std::ostream& os, const ConstexprOptional<T>& val) {
  if (val) {
    os << val.value();
  }
  return os;
}

struct NullConstexprOptional {
  template <typename T>
  constexpr operator ConstexprOptional<T>() const {
    return ConstexprOptional<T>();
  }
};

std::ostream& operator<<(std::ostream& os, NullConstexprOptional) {
  return os;
}

constexpr void ParseFailure(const char* msg) {
  (void)msg;  // intentionally no-op.
}

#ifdef PARSE_FAILURES_FATAL
// Unfortunately we cannot have custom messages here, as it just prints a stack trace with the macros expanded.
// This is at least more flexible than static_assert which requires a string literal.
#define PARSE_FAILURE(msg) X_ASSERT(! #msg)
#define PARSE_ASSERT_MSG(cond, msg) X_ASSERT(#msg && (cond))
#define PARSE_ASSERT(cond) X_ASSERT(cond)
#else
#define PARSE_FAILURE(msg) return NullConstexprOptional{};
#define PARSE_ASSERT_MSG(cond, msg) if (!(cond)) { PARSE_FAILURE(msg); }
#define PARSE_ASSERT(cond) if (!(cond)) { PARSE_FAILURE(""); }
#endif

constexpr ConstexprOptional<ParseTypeDescriptorResult> ParseSingleTypeDescriptor(ConstexprStringErased single_type, bool allow_void = false) {
  constexpr NullConstexprOptional kUnreachable = {};

  // Nothing else left.
  if (single_type.size() == 0) {
    return ParseTypeDescriptorResult{};
  }

  ConstexprStringErased token;
  ConstexprStringErased remainder = single_type.substr(/*start*/1u);

  char c = single_type[0];
  PARSE_ASSERT(IsValidJniDescriptorShorty(c));

  enum State {
    kSingleCharacter,
    kArray,
    kObject
  };

  State state = kSingleCharacter;

  // Parse the first character to figure out if we should parse the rest.
  switch (c) {
    case '!': {
      constexpr bool fast_jni_is_deprecated = false;
      PARSE_ASSERT(fast_jni_is_deprecated);
      break;
    }
    case 'V':
      if (!allow_void) {
        constexpr bool void_type_descriptor_only_allowed_in_return_type = false;
        PARSE_ASSERT(void_type_descriptor_only_allowed_in_return_type);
      }
      // FALL-THROUGH:
    case 'Z':
    case 'B':
    case 'C':
    case 'S':
    case 'I':
    case 'J':
    case 'F':
    case 'D':
      token = single_type.substr(/*start*/0u, /*len*/1u);
      break;
    case 'L':
      state = kObject;
      break;
    case '[':
      state = kArray;
      break;
    default: {
      // See JNI Chapter 3: Type Signatures.
      PARSE_FAILURE("Expected a valid type descriptor character.");
      return kUnreachable;
    }
  }

  // Possibly parse an arbitary-long remainder substring.
  switch (state) {
    case kSingleCharacter:
      return {{ token, remainder }};
    case kArray: {
      // Recursively parse the array component, as it's just any non-void type descriptor.
      ConstexprOptional<ParseTypeDescriptorResult>
          maybe_res = ParseSingleTypeDescriptor(remainder, /*allow_void*/false);
      PARSE_ASSERT(maybe_res);  // Downstream parsing has asserted, bail out.

      ParseTypeDescriptorResult res = maybe_res.value();

      // Reject illegal array type descriptors such as "]".
      PARSE_ASSERT_MSG(res.has_token(), "All array types must follow by their component type (e.g. ']I', ']]Z', etc. ");

      token = single_type.substr(/*start*/0u, res.token.size() + 1u);

      return {{ token, res.remainder }};
    }
    case kObject: {
      // Parse the fully qualified class, e.g. Lfoo/bar/baz;
      // Note checking that each part of the class name is a valid class identifier
      // is too complicated (JLS 3.8).
      // This simple check simply scans until the next ';'.
      bool found_semicolon = false;
      size_t semicolon_len = 0;
      for (size_t i = 0; i < single_type.size(); ++i) {
        if (single_type[i] == ';') {
          semicolon_len = i + 1;
          found_semicolon = true;
          break;
        }
      }

      PARSE_ASSERT(found_semicolon);

      token = single_type.substr(/*start*/0u, semicolon_len);
      remainder = single_type.substr(/*start*/semicolon_len);

      bool class_name_is_empty = token.size() <= 2u;  // e.g. "L;"
      PARSE_ASSERT(!class_name_is_empty);

      return {{ token, remainder }};
    }
    default:
      X_ASSERT(false);
  }

  X_ASSERT(false);
  return kUnreachable;
}

template <typename T, size_t kMaxSize>
struct FunctionSignatureDescriptor {
  ConstexprVector<T, kMaxSize> args;
  T ret;

  static constexpr size_t max_size = kMaxSize;
};

template <size_t kMaxSize>
using JniSignatureDescriptor = FunctionSignatureDescriptor<JniDescriptorNode, kMaxSize>;

template <typename T, size_t kMaxSize>
std::ostream& operator<<(std::ostream& os, const FunctionSignatureDescriptor<T, kMaxSize>& signature) {
  size_t count = 0;
  os << "args={";
  for (auto& arg : signature.args) {
    os << arg;

    if (count != signature.args.size() - 1) {
      os << ",";
    }

    ++count;
  }
  os << "}, ret=";
  os << signature.ret;
  return os;
}

#define PARSE_SIGNATURE_AS_LIST(str) (ParseSignatureAsList<sizeof(str)>(str))

template <size_t kMaxSize>
constexpr ConstexprOptional<JniSignatureDescriptor<kMaxSize>>
      ParseSignatureAsList(ConstexprStringErased signature) {
  // The list of JNI descritors cannot possibly exceed the number of characters
  // in the JNI string literal. We leverage this to give an upper bound of the strlen.
  // This is a bit wasteful but in constexpr there *must* be a fixed upper size for data structures.
  ConstexprVector<JniDescriptorNode, kMaxSize> jni_desc_node_list;
  JniDescriptorNode return_jni_desc;

  enum State {
    kInitial = 0,
    kParsingParameters = 1,
    kParsingReturnType = 2,
    kCompleted = 3,
  };

  State state = kInitial;

  while (!signature.empty()) {
    switch (state) {
      case kInitial: {
        char c = signature[0];
        PARSE_ASSERT_MSG(c == '(', "First character of a JNI signature must be a '('");
        state = kParsingParameters;
        signature = signature.substr(/*start*/1u);
        break;
      }
      case kParsingParameters: {
        char c = signature[0];
        if (c == ')') {
          state = kParsingReturnType;
          signature = signature.substr(/*start*/1u);
          break;
        }

        ConstexprOptional<ParseTypeDescriptorResult>
            res = ParseSingleTypeDescriptor(signature, /*allow_void*/false);
        PARSE_ASSERT(res);

        jni_desc_node_list.push_back(res->as_node());

        signature = res->remainder;
        break;
      }
      case kParsingReturnType: {
        ConstexprOptional<ParseTypeDescriptorResult>
            res = ParseSingleTypeDescriptor(signature, /*allow_void*/true);
        PARSE_ASSERT(res);

        return_jni_desc = res->as_node();
        signature = res->remainder;
        state = kCompleted;
        break;
      }
      default: {
        // e.g. "()VI" is illegal because the V terminates the signature.
        PARSE_FAILURE("Signature had left over tokens after parsing return type");
        break;
      }
    }
  }

  switch (state) {
    case kCompleted:
      // Everything is ok.
      break;
    case kParsingParameters:
      PARSE_FAILURE("Signature was missing ')'");
      break;
    case kParsingReturnType:
      PARSE_FAILURE("Missing return type");
    case kInitial:
      PARSE_FAILURE("Cannot have an empty signature");
    default:
      X_ASSERT(false);
  }

  return {{ jni_desc_node_list, return_jni_desc }};
}

enum NativeKind {
  kNotJni,        // Illegal parameter used inside of a function type.
  kNormalJniCallingConventionParameter,
  kNormalNative,
  kFastNative,
  kCriticalNative,
};

enum TypeFinal {
  kNotFinal,
  kFinal         // e.g. any primitive or any "final" class such as String.
};

// What position is the JNI type allowed to be in?
// Ignored when in a CriticalNative context.
enum NativePositionAllowed {
  kNotAnyPosition,
  kReturnPosition,
  kZerothPosition,
  kFirstOrLaterPosition,
  kSecondOrLaterPosition,
};

constexpr NativePositionAllowed ConvertPositionToAllowed(size_t position) {
  switch (position) {
    case 0:
      return kZerothPosition;
    case 1:
      return kFirstOrLaterPosition;
    default:
      return kSecondOrLaterPosition;
  }
}

// Type traits for a JNI parameter type. See below for specializations.
template <typename T>
struct jni_type_trait {
  static constexpr NativeKind native_kind = kNotJni;
  static constexpr const char type_descriptor[] = "(illegal)";
  static constexpr NativePositionAllowed position_allowed = kNotAnyPosition;
  static constexpr TypeFinal type_finality = kNotFinal;
  static constexpr const char type_name[] = "(illegal)";
};

// Access the jni_type_trait<T> from a non-templated constexpr function.
// Identical non-static fields to jni_type_trait, see Reify().
struct ReifiedJniTypeTrait {
  NativeKind native_kind;
  ConstexprStringErased type_descriptor;
  NativePositionAllowed position_allowed;
  TypeFinal type_finality;
  ConstexprStringErased type_name;

  template <typename T>
  static constexpr ReifiedJniTypeTrait Reify() {
    // This should perhaps be called 'Type Erasure' except we don't use virtuals,
    // so it's not quite the same idiom.
    using TR = jni_type_trait<T>;
    return { TR::native_kind, TR::type_descriptor, TR::position_allowed, TR::type_finality, TR::type_name };
  }

  static constexpr ConstexprOptional<ReifiedJniTypeTrait> MostSimilarTypeDescriptor(ConstexprStringErased type_descriptor);
};

constexpr bool operator==(const ReifiedJniTypeTrait& lhs, const ReifiedJniTypeTrait& rhs) {
  return lhs.native_kind == rhs.native_kind && rhs.type_descriptor == lhs.type_descriptor &&
      lhs.position_allowed == rhs.position_allowed && rhs.type_finality == lhs.type_finality &&
      lhs.type_name == rhs.type_name;
}

std::ostream& operator<<(std::ostream& os, const ReifiedJniTypeTrait& rjft) {
  // os << "ReifiedJniTypeTrait<" << rjft.type_name << ">";
  os << rjft.type_name;
  return os;
}

#define JNI_TYPE_TRAIT(jtype, the_type_descriptor, the_native_kind, the_type_finality, the_position) \
template <>                                                                    \
struct jni_type_trait< jtype > {                                               \
  static constexpr NativeKind native_kind = the_native_kind;                   \
  static constexpr const char type_descriptor[] = the_type_descriptor;         \
  static constexpr NativePositionAllowed position_allowed = the_position;      \
  static constexpr TypeFinal type_finality = the_type_finality;                \
  static constexpr const char type_name[] = #jtype;                            \
};                                                                             \
constexpr char jni_type_trait<jtype>::type_descriptor[];                       \
constexpr char jni_type_trait<jtype>::type_name[];

#define DEFINE_JNI_TYPE_TRAIT(TYPE_TRAIT_FN)                                                                  \
TYPE_TRAIT_FN(jboolean,          "Z",                      kCriticalNative,   kFinal, kSecondOrLaterPosition) \
TYPE_TRAIT_FN(jbyte,             "B",                      kCriticalNative,   kFinal, kSecondOrLaterPosition) \
TYPE_TRAIT_FN(jchar,             "C",                      kCriticalNative,   kFinal, kSecondOrLaterPosition) \
TYPE_TRAIT_FN(jshort,            "S",                      kCriticalNative,   kFinal, kSecondOrLaterPosition) \
TYPE_TRAIT_FN(jint,              "I",                      kCriticalNative,   kFinal, kSecondOrLaterPosition) \
TYPE_TRAIT_FN(jlong,             "J",                      kCriticalNative,   kFinal, kSecondOrLaterPosition) \
TYPE_TRAIT_FN(jfloat,            "F",                      kCriticalNative,   kFinal, kSecondOrLaterPosition) \
TYPE_TRAIT_FN(jdouble,           "D",                      kCriticalNative,   kFinal, kSecondOrLaterPosition) \
TYPE_TRAIT_FN(jobject,           "Ljava/lang/Object;",     kFastNative,    kNotFinal, kFirstOrLaterPosition)  \
TYPE_TRAIT_FN(jclass,            "Ljava/lang/Class;",      kFastNative,       kFinal, kFirstOrLaterPosition)  \
TYPE_TRAIT_FN(jstring,           "Ljava/lang/String;",     kFastNative,       kFinal, kSecondOrLaterPosition) \
TYPE_TRAIT_FN(jarray,            "Ljava/lang/Object;",     kFastNative,    kNotFinal, kSecondOrLaterPosition) \
TYPE_TRAIT_FN(jobjectArray,      "[Ljava/lang/Object;",    kFastNative,    kNotFinal, kSecondOrLaterPosition) \
TYPE_TRAIT_FN(jbooleanArray,     "[Z",                     kFastNative,       kFinal, kSecondOrLaterPosition) \
TYPE_TRAIT_FN(jcharArray,        "[C",                     kFastNative,       kFinal, kSecondOrLaterPosition) \
TYPE_TRAIT_FN(jshortArray,       "[S",                     kFastNative,       kFinal, kSecondOrLaterPosition) \
TYPE_TRAIT_FN(jintArray,         "[I",                     kFastNative,       kFinal, kSecondOrLaterPosition) \
TYPE_TRAIT_FN(jlongArray,        "[J",                     kFastNative,       kFinal, kSecondOrLaterPosition) \
TYPE_TRAIT_FN(jfloatArray,       "[F",                     kFastNative,       kFinal, kSecondOrLaterPosition) \
TYPE_TRAIT_FN(jdoubleArray,      "[D",                     kFastNative,       kFinal, kSecondOrLaterPosition) \
TYPE_TRAIT_FN(jthrowable,        "Ljava/lang/Throwable;",  kFastNative,    kNotFinal, kSecondOrLaterPosition) \
TYPE_TRAIT_FN(JNIEnv*,           "",                       kNormalJniCallingConventionParameter, kFinal, kZerothPosition) \
TYPE_TRAIT_FN(void,              "V",                      kCriticalNative,   kFinal, kReturnPosition)        \

DEFINE_JNI_TYPE_TRAIT(JNI_TYPE_TRAIT)

constexpr ConstexprOptional<ReifiedJniTypeTrait> ReifiedJniTypeTrait::MostSimilarTypeDescriptor(ConstexprStringErased type_descriptor) {
  #define MATCH_EXACT_TYPE_DESCRIPTOR_FN(type, type_desc, native_kind, ...) \
    if (type_descriptor == type_desc && native_kind >= kNormalNative) {                        \
      return { Reify<type>() };                                \
    }

  // Attempt to look up by the precise type match first.
  DEFINE_JNI_TYPE_TRAIT(MATCH_EXACT_TYPE_DESCRIPTOR_FN);

  // Otherwise, we need to do an imprecise match:
  char shorty = type_descriptor.size() >= 1 ? type_descriptor[0] : '\0';
  if (shorty == 'L') {
    // Something more specific like Ljava/lang/Throwable, string, etc
    // is already matched by the macro-expanded conditions above.
    return { Reify<jobject>() };
  } else if (type_descriptor.size() >= 2) {
    auto shorty_shorty = type_descriptor.substr(/*start*/0, /*size*/2u);
    if (shorty_shorty == "[[" || shorty_shorty == "[L") {
      // JNI arrays are covariant, so any type T[] (T!=primitive) is castable to Object[].
      return { Reify<jobjectArray>() };
    }
  }

  // To handle completely invalid values.
  return NullConstexprOptional{};
}

// Check if a jni parameter type is valid given its position
template <typename T>
constexpr bool IsValidJniParameter(NativeKind native_kind, NativePositionAllowed position) {
  using expected_trait = jni_type_trait<typename std::decay<T>::type>;
  NativeKind expected_native_kind = expected_trait::native_kind;

  // Most types 'T' are not valid for JNI.
  if (expected_native_kind == NativeKind::kNotJni) {
    return false;
  }

  // The rest of the types might be valid, but it depends on the context (native_kind)
  // and also on their position within the parameters.

  // Position-check first. CriticalNatives ignore positions since the first 2 special parameters are stripped.
  while (native_kind != kCriticalNative) {
    NativePositionAllowed expected_position = expected_trait::position_allowed;
    X_ASSERT(expected_position != kNotAnyPosition);

    // Is this a return-only position?
    if (expected_position == kReturnPosition) {
      if (position != kReturnPosition) {
        // void can only be in the return position.
        return false;
      }
      // Don't do the other non-return position checks for a return-only position.
      break;
    }

    // JNIEnv* can only be in the first spot.
    if (position == kZerothPosition && expected_position != kZerothPosition) {
      return false;
      // jobject, jclass can be 1st or anywhere afterwards.
    } else if (position == kFirstOrLaterPosition && expected_position != kFirstOrLaterPosition) {
      return false;
      // All other parameters must be in 2nd+ spot, or in the return type.
    } else if (position == kSecondOrLaterPosition || position == kReturnPosition) {
      if (expected_position != kFirstOrLaterPosition && expected_position != kSecondOrLaterPosition) {
        return false;
      }
    }

    break;
  }

  // Ensure the type appropriate is for the native kind.

  if (expected_native_kind == kNormalJniCallingConventionParameter) {
    // It's always wrong to use a JNIEnv* anywhere but the 0th spot.
     if (native_kind == kCriticalNative) {
      // CriticalNative does not allow using a JNIEnv*.
      return false;
    }

    return true;  // OK: JniEnv* used in 0th position.
  } else if (expected_native_kind == kCriticalNative) {
    // CriticalNative arguments are always valid JNI types anywhere used.
    return true;
  } else if (native_kind == kCriticalNative) {
    // The expected_native_kind was non-critical but we are in a critical context.
    // Illegal type.
    return false;
  }

  // Everything else is fine, e.g. fast/normal native + fast/normal native parameters.
  return true;
}

constexpr bool IsJniParameterCountValid(NativeKind native_kind, size_t count) {
  if (native_kind == kNormalNative || native_kind == kFastNative) {
    return count >= 2u;
  } else if (native_kind == kCriticalNative) {
    return true;
  }

  constexpr bool invalid_parameter = false;
  X_ASSERT(invalid_parameter);
  return false;
}

template <NativeKind native_kind, size_t position, typename ... Args>
struct is_valid_jni_argument_type {
};

template <NativeKind native_kind, size_t position>
struct is_valid_jni_argument_type<native_kind, position> {
  static constexpr bool value = true;
};

template <NativeKind native_kind, size_t position, typename T>
struct is_valid_jni_argument_type<native_kind, position, T> {
  static constexpr bool value =
      IsValidJniParameter<T>(native_kind, ConvertPositionToAllowed(position));
};

template <NativeKind native_kind, size_t position, typename T, typename ... Args>
struct is_valid_jni_argument_type<native_kind, position, T, Args...> {
  static constexpr bool value =
      IsValidJniParameter<T>(native_kind, ConvertPositionToAllowed(position))
          && is_valid_jni_argument_type<native_kind, position + 1, Args...>::value;
};

template <NativeKind native_kind, typename T, T fn>
struct is_valid_jni_function_type_helper;

template <NativeKind native_kind, typename R, typename ... Args, R fn(Args...)>
struct is_valid_jni_function_type_helper<native_kind, R(Args...), fn> {
  static constexpr bool value =
      IsJniParameterCountValid(native_kind, sizeof...(Args))
          && IsValidJniParameter<R>(native_kind, kReturnPosition)
          && is_valid_jni_argument_type<native_kind, /*position*/0, Args...>::value;
};

template <NativeKind native_kind, typename T, T fn>
constexpr bool IsValidJniFunctionType() {
  return is_valid_jni_function_type_helper<native_kind, T, fn>::value;
  // TODO: we could replace template metaprogramming with constexpr by using FunctionTypeTraits metafunctions.
}

#define IS_VALID_JNI_FUNCTION_TYPE(native_kind, func) (IsValidJniFunctionType<native_kind, decltype(func), (func)>())
#define IS_VALID_NORMAL_JNI_FUNCTION_TYPE(func) IS_VALID_JNI_FUNCTION_TYPE(kNormalNative, func)
#define IS_VALID_CRITICAL_JNI_FUNCTION_TYPE(func) IS_VALID_JNI_FUNCTION_TYPE(kCriticalNative, func)

template <typename ... Args>
struct ArgumentList {};

// Many parts of std::array is not constexpr until C++17.
template <typename T, size_t N>
struct ConstexprArray {
  // Intentionally public to conform to std::array.
  // This means all constructors are implicit.
  // *NOT* meant to be used directly, use the below functions instead.
  T _array[N];

  constexpr size_t size() const {
    return N;
  }

  using iterator = T*;
  using const_iterator = const T*;

  constexpr iterator begin() {
    return &_array[0];
  }

  constexpr iterator end() {
    return &_array[N];  // XX: what about _size == kMaxSize?
  }

  constexpr const_iterator begin() const {
    return &_array[0];
  }

  constexpr const_iterator end() const {
    return &_array[N];  // XX: what about _size == kMaxSize?
  }

  constexpr T& operator[](size_t i) {
    return _array[i];
  }

  constexpr const T& operator[](size_t i) const {
    return _array[i];
  }
};

// Why do we need this?
// auto x = {1,2,3} creates an initializer_list,
//   but they can't be returned because it contains pointers to temporaries.
// auto x[] = {1,2,3} doesn't even work because auto for arrays is not supported.
//
// an alternative would be to pull up std::common_t directly into the call site
//   std::common_type_t<Args...> array[] = {1,2,3}
// but that's even more cludgier.
//
// As the other "stdlib-wannabe" functions, it's weaker than the library
// fundamentals std::make_array but good enough for our use.
template <typename... Args>
constexpr auto MakeArray(Args&&... args) {
  return ConstexprArray<typename std::common_type<Args...>::type, sizeof...(Args)>{args...};
}

// TODO: is FunctionTypeMetafunction a better name?
template <typename T, T fn>
struct FunctionTypeTraits {
};

template <typename R, typename ... Args, R fn(Args...)>
struct FunctionTypeTraits<R(Args...), fn> {
  static constexpr size_t count = sizeof...(Args) + 1u;  // args and return type.

  // Return an array where the metafunction 'Func' has been applied
  // to every argument type. The metafunction must be returning a common type.
  template <template <typename Arg> class Func>
  static constexpr auto map_args() {
    return map_args_impl<Func>(holder<Args>{}...);
  }

  // Apply the metafunction 'Func' over the return type.
  template <template <typename Ret> class Func>
  static constexpr auto map_return() {
    return Func<R>{}();
  }

 private:
  template <typename T>
  struct holder {
  };

  template <template <typename Arg> class Func>
  static constexpr auto map_args_impl() {
    using ComponentType = decltype(Func<void>{}());

    return ConstexprArray<ComponentType, /*size*/0u>{};
  }

  template <template <typename Arg> class Func, typename Arg0, typename... ArgsRest>
  static constexpr auto map_args_impl(holder<Arg0>, holder<ArgsRest>...) {
    // One does not simply call MakeArray with 0 template arguments...
    auto array = MakeArray(
        Func<Args>{}()...
    );

    return array;
  }
};

template <typename T>
struct ReifyJniTypeMetafunction {
  constexpr ReifiedJniTypeTrait operator()() const {
    auto res = ReifiedJniTypeTrait::Reify<T>();
    X_ASSERT(res.native_kind != kNotJni);
    return res;
  }
};

template <size_t kMaxSize>
using ReifiedJniSignature = FunctionSignatureDescriptor<ReifiedJniTypeTrait, kMaxSize>;

template <NativeKind native_kind, typename T, T fn, size_t kMaxSize = FunctionTypeTraits<T, fn>::count>
constexpr ConstexprOptional<ReifiedJniSignature<kMaxSize>>
      MaybeMakeReifiedJniSignature() {
  if (!IsValidJniFunctionType<native_kind, T, fn>()) {
    PARSE_FAILURE("The function signature has one or more types incompatible with JNI.");
  }

  ReifiedJniTypeTrait return_jni_trait =
      FunctionTypeTraits<T, fn>::template map_return<ReifyJniTypeMetafunction>();

  constexpr size_t kSkipArgumentPrefix = (native_kind != kCriticalNative) ? 2u : 0u;
  ConstexprVector<ReifiedJniTypeTrait, kMaxSize> args;
  auto args_list = FunctionTypeTraits<T, fn>::template map_args<ReifyJniTypeMetafunction>();
  size_t args_index = 0;
  for (auto& arg : args_list) {
    // Ignore the 'JNIEnv*, jobject' / 'JNIEnv*, jclass' prefix,
    // as its not part of the function descriptor string.
    if (args_index >= kSkipArgumentPrefix) {
      args.push_back(arg);
    }

    ++args_index;
  }

  return {{ args, return_jni_trait }};
}

#define COMPARE_DESCRIPTOR_CHECK(expr) if (!(expr)) return false
#define COMPARE_DESCRIPTOR_FAILURE_MSG(msg) if ((true)) return false

constexpr bool CompareJniDescriptorNodeErased(JniDescriptorNode user_defined_descriptor,
                                              ReifiedJniTypeTrait derived) {

  ConstexprOptional<ReifiedJniTypeTrait> user_reified_opt =
      ReifiedJniTypeTrait::MostSimilarTypeDescriptor(user_defined_descriptor.longy);

  if (!user_reified_opt.has_value()) {
    COMPARE_DESCRIPTOR_FAILURE_MSG(
        "Could not find any JNI C++ type corresponding to the type descriptor");
  }

  char user_shorty = user_defined_descriptor.longy.size() > 0 ?
      user_defined_descriptor.longy[0] :
      '\0';

  ReifiedJniTypeTrait user = user_reified_opt.value();
  if (user == derived) {
    // If we had a similar match, immediately return success.
    return true;
  } else if (derived.type_name == "jthrowable") {
    if (user_shorty == 'L') {
      // Weakly allow any objects to correspond to a jthrowable.
      // We do not know the managed type system so we have to be permissive here.
      return true;
    } else {
      COMPARE_DESCRIPTOR_FAILURE_MSG("jthrowable must correspond to an object type descriptor");
    }
  } else if (derived.type_name == "jarray") {
    if (user_shorty == '[') {
      // a jarray is the base type for all other array types. Allow.
      return true;
    } else {
      // Ljava/lang/Object; is the root for all array types.
      // Already handled above in 'if user == derived'.
      COMPARE_DESCRIPTOR_FAILURE_MSG("jarray must correspond to array type descriptor");
    }
  }
  // Otherwise, the comparison has failed and the rest of this is only to
  // pick the most appropriate error message.
  //
  // Note: A weaker form of comparison would allow matching 'Ljava/lang/String;'
  // against 'jobject', etc. However the policy choice here is to enforce the strictest
  // comparison that we can to utilize the type system to its fullest.

  if (derived.type_finality == kFinal || user.type_finality == kFinal) {
    // Final types, e.g. "I", "Ljava/lang/String;" etc must match exactly
    // the C++ jni descriptor string ('I' -> jint, 'Ljava/lang/String;' -> jstring).
    COMPARE_DESCRIPTOR_FAILURE_MSG(
        "The JNI descriptor string must be the exact type equivalent of the "
        "C++ function signature.");
  } else if (user_shorty == '[') {
    COMPARE_DESCRIPTOR_FAILURE_MSG(
        "The array JNI descriptor must correspond to j${type}Array or jarray");
  } else if (user_shorty == 'L') {
    COMPARE_DESCRIPTOR_FAILURE_MSG("The object JNI descriptor must correspond to jobject.");
  } else {
    X_ASSERT(false);  // We should never get here, but either way this means the types did not match
    COMPARE_DESCRIPTOR_FAILURE_MSG(
        "The JNI type descriptor string does not correspond to the C++ JNI type.");
  }
}

template <NativeKind native_kind, typename T, T fn, size_t kMaxSize>
constexpr bool MatchJniDescriptorWithFunctionType(ConstexprStringErased user_function_descriptor) {
  constexpr size_t kReifiedMaxSize = FunctionTypeTraits<T, fn>::count;

  ConstexprOptional<ReifiedJniSignature<kReifiedMaxSize>> reified_signature_opt =
      MaybeMakeReifiedJniSignature<native_kind, T, fn>();
  if (!reified_signature_opt) {
    // Assertion handling done by MaybeMakeReifiedJniSignature.
    return false;
  }

  ConstexprOptional<JniSignatureDescriptor<kMaxSize>> user_jni_sig_desc_opt =
      ParseSignatureAsList<kMaxSize>(user_function_descriptor);

  if (!user_jni_sig_desc_opt) {
    // Assertion handling done by ParseSignatureAsList.
    return false;
  }

  ReifiedJniSignature<kReifiedMaxSize> reified_signature = reified_signature_opt.value();
  JniSignatureDescriptor<kMaxSize> user_jni_sig_desc = user_jni_sig_desc_opt.value();

  if (reified_signature.args.size() != user_jni_sig_desc.args.size()) {
    COMPARE_DESCRIPTOR_FAILURE_MSG("Number of parameters in JNI descriptor string"
                                   "did not match number of parameters in C++ function type");
  } else if (!CompareJniDescriptorNodeErased(user_jni_sig_desc.ret, reified_signature.ret)) {
    // Assertion handling done by CompareJniDescriptorNodeErased.
    return false;
  } else {
    for (size_t i = 0; i < user_jni_sig_desc.args.size(); ++i) {
      if (!CompareJniDescriptorNodeErased(user_jni_sig_desc.args[i],
                                          reified_signature.args[i])) {
        // Assertion handling done by CompareJniDescriptorNodeErased.
        return false;
      }
    }
  }

  return true;
}

template <NativeKind native_kind, typename T, T fn>
struct InferJniDescriptor {
  static constexpr size_t kMaxSize = FunctionTypeTraits<T, fn>::count;

  static constexpr ConstexprOptional<JniSignatureDescriptor<kMaxSize>> FromFunctionType() {
    constexpr size_t kReifiedMaxSize = kMaxSize;
    ConstexprOptional<ReifiedJniSignature<kReifiedMaxSize>> reified_signature_opt =
        MaybeMakeReifiedJniSignature<native_kind, T, fn>();
    if (!reified_signature_opt) {
      // Assertion handling done by MaybeMakeReifiedJniSignature.
      return NullConstexprOptional{};
    }

    ReifiedJniSignature<kReifiedMaxSize> reified_signature = reified_signature_opt.value();

    JniSignatureDescriptor<kReifiedMaxSize> signature_descriptor;

    if (reified_signature.ret.type_finality != kFinal) {
      // e.g. jint, jfloatArray, jstring, jclass are ok. jobject, jthrowable, jarray are not.
      PARSE_FAILURE("Bad return type. Only unambigous (final) types can be used to infer a signature.");
    }
    signature_descriptor.ret = JniDescriptorNode{reified_signature.ret.type_descriptor};

    for (size_t i = 0; i < reified_signature.args.size(); ++i) {
      const ReifiedJniTypeTrait& arg_trait = reified_signature.args[i];
      if (arg_trait.type_finality != kFinal) {
        PARSE_FAILURE("Bad parameter type. Only unambigous (final) types can be used to infer a signature.");
      }
      signature_descriptor.args.push_back(JniDescriptorNode{arg_trait.type_descriptor});
    }
    /*
    for (const ReifiedJniTypeTrait& arg_trait : reified_signature.args) {
    }
     */

    return { signature_descriptor };
  }

  static constexpr size_t CalculateStringSize() {
    ConstexprOptional<JniSignatureDescriptor<kMaxSize>> signature_descriptor_opt =
        FromFunctionType();
    if (!signature_descriptor_opt) {
      // Assertion handling done by FromFunctionType.
      return 0u;
    }

    JniSignatureDescriptor<kMaxSize> signature_descriptor =
        signature_descriptor_opt.value();

    size_t acc_size = 1u;  // All sigs start with '('.

    // Now add every parameter.
    for (size_t j = 0; j < signature_descriptor.args.size(); ++j) {
      const JniDescriptorNode& arg_descriptor = signature_descriptor.args[j];
    // for (const JniDescriptorNode& arg_descriptor : signature_descriptor.args) {
      acc_size += arg_descriptor.longy.size();
    }

    acc_size += 1u;   // Add space for ')'.

    // Add space for the return value.
    acc_size += signature_descriptor.ret.longy.size();

    return acc_size;
  }

  static constexpr size_t kMaxStringSize = CalculateStringSize();
  using ConstexprStringDescriptorType = ConstexprArray<char, kMaxStringSize+1>;

  static constexpr bool kAllowPartialStrings = true;

  static constexpr ConstexprStringDescriptorType GetString() {
    ConstexprStringDescriptorType c_str{};

    ConstexprOptional<JniSignatureDescriptor<kMaxSize>> signature_descriptor_opt =
        FromFunctionType();
    if (!signature_descriptor_opt.has_value()) {
      // Assertion handling done by FromFunctionType.
      c_str[0] = '\0';
      return c_str;
    }

    JniSignatureDescriptor<kMaxSize> signature_descriptor =
        signature_descriptor_opt.value();

    size_t pos = 0u;
    c_str[pos++] = '(';

    // Copy all parameter descriptors.
    //for (const JniDescriptorNode& arg_descriptor : signature_descriptor.args) {
    for (size_t j = 0; j < signature_descriptor.args.size(); ++j) {
      const JniDescriptorNode& arg_descriptor = signature_descriptor.args[j];
      ConstexprStringErased longy = arg_descriptor.longy;
      for (size_t i = 0; i < longy.size(); ++i) {
        if (kAllowPartialStrings && pos >= kMaxStringSize) {
          break;
        }
        c_str[pos++] = longy[i];
      }
    }

    if (!kAllowPartialStrings || pos < kMaxStringSize) {
      c_str[pos++] = ')';
    }

    // Copy return descriptor.
    ConstexprStringErased longy = signature_descriptor.ret.longy;
    for (size_t i = 0; i < longy.size(); ++i) {
        if (kAllowPartialStrings && pos >= kMaxStringSize) {
          break;
        }
      c_str[pos++] = longy[i];
    }

    //X_ASSERT(pos == kMaxStringSize);

    c_str[pos] = '\0';

    return c_str;
  }

  static const char* GetStringAtRuntime() {
    static constexpr ConstexprStringDescriptorType str = GetString();
    return &str[0];
  }
};

#define MAKE_CHECKED_JNI_NATIVE_METHOD(native_kind, name_, signature_, fn) \
  ({                                                                     \
    constexpr bool is_signature_valid =                                  \
        MatchJniDescriptorWithFunctionType<native_kind,                  \
                                           decltype(fn),                 \
                                           fn,                           \
                                           sizeof(signature_)>(signature_);\
    static_assert(is_signature_valid, "JNI signature doesn't match C++ function type."); \
    /* Suppress implicit cast warnings by explicitly casting. */         \
    JNINativeMethod tmp_native_method{                                   \
        const_cast<decltype(JNINativeMethod::name)>(name_),              \
        const_cast<decltype(JNINativeMethod::signature)>(signature_),    \
        reinterpret_cast<void*>(&fn)};                                   \
    tmp_native_method;                                                   \
  })

#define MAKE_INFERRED_JNI_NATIVE_METHOD(native_kind, name_, fn)          \
  ({                                                                     \
    using Infer_t = InferJniDescriptor<    native_kind,                  \
                                           decltype(fn),                 \
                                           fn>;                          \
    const char* inferred_signature = Infer_t::GetStringAtRuntime();      \
    /* Suppress implicit cast warnings by explicitly casting. */         \
    JNINativeMethod tmp_native_method{                                   \
        const_cast<decltype(JNINativeMethod::name)>(name_),              \
        const_cast<decltype(JNINativeMethod::signature)>(inferred_signature),    \
        reinterpret_cast<void*>(&fn)};                                   \
    tmp_native_method;                                                   \
  })

