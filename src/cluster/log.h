/*
* Copyright (C) 2017-2018 by Massachusetts Institute of Technology
*
* This file is part of KPart.
*
* Permission is hereby granted, free of charge, to any person obtaining a copy
* of this software and associated documentation files (the "Software"), to deal
* in the Software without restriction, including without limitation the rights
* to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
* copies of the Software, and to permit persons to whom the Software is
* furnished to do so, subject to the following conditions:
*
* The above copyright notice and this permission notice shall be included in all
* copies or substantial portions of the Software.
*
* If you use this software in your research, we request that you reference
* the KPart paper ("KPart: A Hybrid Cache Partitioning-Sharing Technique for
* Commodity Multicores", El-Sayed, Mukkara, Tsai, Kasture, Ma and Sanchez,
* HPCA-24, February 2018) as the source in any publications that use this
* software, and that you send us a citation of your work.
*
* THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
* IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
* FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
* AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
* LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
* OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
* SOFTWARE.
*/
#pragma once

#include <sstream>
#include <stdio.h>
#include <stdlib.h>

#define PANIC_EXIT_CODE (112)

// assertions are often frequently executed but never inlined. Might as well
// tell the compiler about it
#define likely(x) __builtin_expect((x), 1)
#define unlikely(x) __builtin_expect((x), 0)

/* Helper class to print expression with values
 * Inpired by Phil Nash's CATCH, https://github.com/philsquared/Catch
 * const enough that asserts that use this are still optimized through
 * loop-invariant code motion
 */
class PrintExpr {
private:
  std::stringstream &ss;

public:
  PrintExpr(std::stringstream &_ss) : ss(_ss) {}

  // Start capturing values
  template <typename T> const PrintExpr operator->*(T t) const {
    ss << t;
    return *this;
  }

  // Overloads for all lower-precedence operators
  template <typename T> const PrintExpr operator==(T t) const {
    ss << " == " << t;
    return *this;
  }
  template <typename T> const PrintExpr operator!=(T t) const {
    ss << " != " << t;
    return *this;
  }
  template <typename T> const PrintExpr operator<=(T t) const {
    ss << " <= " << t;
    return *this;
  }
  template <typename T> const PrintExpr operator>=(T t) const {
    ss << " >= " << t;
    return *this;
  }
  template <typename T> const PrintExpr operator<(T t) const {
    ss << " < " << t;
    return *this;
  }
  template <typename T> const PrintExpr operator>(T t) const {
    ss << " > " << t;
    return *this;
  }
  template <typename T> const PrintExpr operator&(T t) const {
    ss << " & " << t;
    return *this;
  }
  template <typename T> const PrintExpr operator|(T t) const {
    ss << " | " << t;
    return *this;
  }
  template <typename T> const PrintExpr operator^(T t) const {
    ss << " ^ " << t;
    return *this;
  }
  template <typename T> const PrintExpr operator&&(T t) const {
    ss << " && " << t;
    return *this;
  }
  template <typename T> const PrintExpr operator||(T t) const {
    ss << " || " << t;
    return *this;
  }
  template <typename T> const PrintExpr operator+(T t) const {
    ss << " + " << t;
    return *this;
  }
  template <typename T> const PrintExpr operator-(T t) const {
    ss << " - " << t;
    return *this;
  }
  template <typename T> const PrintExpr operator*(T t) const {
    ss << " * " << t;
    return *this;
  }
  template <typename T> const PrintExpr operator/(T t) const {
    ss << " / " << t;
    return *this;
  }
  template <typename T> const PrintExpr operator%(T t) const {
    ss << " % " << t;
    return *this;
  }
  template <typename T> const PrintExpr operator<<(T t) const {
    ss << " << " << t;
    return *this;
  }
  template <typename T> const PrintExpr operator>>(T t) const {
    ss << " >> " << t;
    return *this;
  }

  // std::nullptr_t overloads (for nullptr's in assertions)
  // Only a few are needed, since most ops w/ nullptr are invalid
  const PrintExpr operator->*(std::nullptr_t t) const {
    ss << "nullptr";
    return *this;
  }
  const PrintExpr operator==(std::nullptr_t t) const {
    ss << " == nullptr";
    return *this;
  }
  const PrintExpr operator!=(std::nullptr_t t) const {
    ss << " != nullptr";
    return *this;
  }

private:
  template <typename T>
  const PrintExpr operator=(T t) const; // will fail, can't assign in assertion
};

#define panic(args...)                                                         \
  {                                                                            \
    fprintf(stderr, "Panic on %s:%d: ", __FILE__, __LINE__);                   \
    fprintf(stderr, args);                                                     \
    fprintf(stderr, "\n");                                                     \
    fflush(stderr);                                                            \
    exit(PANIC_EXIT_CODE);                                                     \
  }

#define warn(args...)                                                          \
  {                                                                            \
    fprintf(stderr, "WARN: ");                                                 \
    fprintf(stderr, args);                                                     \
    fprintf(stderr, "\n");                                                     \
    fflush(stderr);                                                            \
  }

#define info(args...)                                                          \
  {                                                                            \
    fprintf(stdout, args);                                                     \
    fprintf(stdout, "\n");                                                     \
    fflush(stdout);                                                            \
  }

#ifndef NASSERT
#define assert(expr)                                                           \
  if (unlikely(!(expr))) {                                                     \
    std::stringstream __assert_ss__LINE__;                                     \
    (PrintExpr(__assert_ss__LINE__)->*expr);                                   \
    fprintf(stderr, "Failed assertion on %s:%d '%s' (with '%s')\n", __FILE__,  \
            __LINE__, #expr, __assert_ss__LINE__.str().c_str());               \
    fflush(stderr);                                                            \
    *reinterpret_cast<int *>(0L) = 42; /*SIGSEGVs*/                            \
    exit(1);                                                                   \
  }                                                                            \
  ;

#define assert_msg(cond, args...)                                              \
  if (unlikely(!(cond))) {                                                     \
    fprintf(stderr, "Failed assertion on %s:%d: ", __FILE__, __LINE__);        \
    fprintf(stderr, args);                                                     \
    fprintf(stderr, "\n");                                                     \
    fflush(stderr);                                                            \
    *reinterpret_cast<int *>(0L) = 42; /*SIGSEGVs*/                            \
    exit(1);                                                                   \
  }                                                                            \
  ;
#else
// Avoid unused warnings, never emit any code
// see http://cnicholson.net/2009/02/stupid-c-tricks-adventures-in-assert/
#define assert(cond)                                                           \
  do {                                                                         \
    (void) sizeof(cond);                                                       \
  } while (0);
#define assert_msg(cond, args...)                                              \
  do {                                                                         \
    (void) sizeof(cond);                                                       \
  } while (0);
#endif
