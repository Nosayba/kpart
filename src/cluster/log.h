/** $lic$
 * Copyright (C) 2012-2016 by Massachusetts Institute of Technology
 * Copyright (C) 2010-2013 by The Board of Trustees of Stanford University
 *
 * This file is part of zsim.
 *
 * zsim is free software; you can redistribute it and/or modify it under the
 * terms of the GNU General Public License as published by the Free Software
 * Foundation, version 2.
 *
 * If you use this software in your research, we request that you reference
 * the zsim paper ("ZSim: Fast and Accurate Microarchitectural Simulation of
 * Thousand-Core Systems", Sanchez and Kozyrakis, ISCA-40, June 2013) as the
 * source of the simulator in any publications that use this software, and that
 * you send us a citation of your work. If you use code from Jigsaw
 * (src/jigsaw*, src/gmon*, src/lookahead*), we request that you reference our
 * papers ("Jigsaw: Scalable Software-Defined Caches", Beckmann and Sanchez,
 * PACT-22, September 2013 and "Scaling Distributed Cache Hierarchies through
 * Computation and Data Co-Scheduling", HPCA-21, Feb 2015). If you use code
 * from Talus (src/talus*), we request that you reference our paper ("Talus: A
 * Simple Way to Remove Cliffs in Cache Performance", Beckmann and Sanchez,
 * HPCA-21, February 2015).
 *
 * zsim is distributed in the hope that it will be useful, but WITHOUT ANY
 * WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS
 * FOR A PARTICULAR PURPOSE. See the GNU General Public License for more
 * details.
 *
 * You should have received a copy of the GNU General Public License along with
 * this program. If not, see <http://www.gnu.org/licenses/>.
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
