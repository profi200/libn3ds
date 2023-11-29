#pragma once

/*
 *   This file is part of libn3ds
 *   Copyright (C) 2023 derrek, profi200
 *
 *   This program is free software: you can redistribute it and/or modify
 *   it under the terms of the GNU General Public License as published by
 *   the Free Software Foundation, either version 3 of the License, or
 *   (at your option) any later version.
 *
 *   This program is distributed in the hope that it will be useful,
 *   but WITHOUT ANY WARRANTY; without even the implied warranty of
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *   GNU General Public License for more details.
 *
 *   You should have received a copy of the GNU General Public License
 *   along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#include <unistd.h> // ssize_t.


#define ALIGN(a)       __attribute__((aligned(a))) // If possible use alignas() instead.
#define NAKED          __attribute__((naked))
#define NOINLINE       __attribute__((noinline))
#define ALWAYS_INLINE  __attribute__((always_inline)) static inline
#define PACKED         __attribute__((packed))
#define TARGET_ARM     __attribute__((target("arm")))
#define TARGET_THUMB   __attribute__((target("thumb")))
#define UNUSED         [[maybe_unused]]
#define USED           __attribute__((used))
#define WEAK           __attribute__((weak))


#ifdef __cplusplus
#include <atomic>    // std::atomic<>.
#include <cinttypes> // printf()/scanf() macros.
#include <cstddef>   // size_t, NULL...
#include <cstdint>   // uint8_t, uint16_t...


using u8  = uint8_t;
using u16 = uint16_t;
using u32 = uint32_t;
using u64 = uint64_t;

using s8  = int8_t;
using s16 = int16_t;
using s32 = int32_t;
using s64 = int64_t;

using vu8  = volatile uint8_t;
using vu16 = volatile uint16_t;
using vu32 = volatile uint32_t;
using vu64 = volatile uint64_t;

using vs8  = volatile int8_t;
using vs16 = volatile int16_t;
using vs32 = volatile int32_t;
using vs64 = volatile int64_t;

using au8  = std::atomic<uint8_t>;
using au16 = std::atomic<uint16_t>;
using au32 = std::atomic<uint32_t>;
using au64 = std::atomic<uint64_t>;

using as8  = std::atomic<int8_t>;
using as16 = std::atomic<int16_t>;
using as32 = std::atomic<int32_t>;
using as64 = std::atomic<int64_t>;

#else // C.

#include <inttypes.h>    // printf()/scanf() macros.
#include <stdatomic.h>   // _Atomic.
#include <stddef.h>      // size_t, NULL...
#include <stdint.h>      // uint8_t, uint16_t...


typedef uint8_t  u8;
typedef uint16_t u16;
typedef uint32_t u32;
typedef uint64_t u64;

typedef int8_t  s8;
typedef int16_t s16;
typedef int32_t s32;
typedef int64_t s64;

typedef volatile uint8_t  vu8;
typedef volatile uint16_t vu16;
typedef volatile uint32_t vu32;
typedef volatile uint64_t vu64;

typedef volatile int8_t  vs8;
typedef volatile int16_t vs16;
typedef volatile int32_t vs32;
typedef volatile int64_t vs64;

typedef _Atomic uint8_t  au8;
typedef _Atomic uint16_t au16;
typedef _Atomic uint32_t au32;
typedef _Atomic uint64_t au64;

typedef _Atomic int8_t  as8;
typedef _Atomic int16_t as16;
typedef _Atomic int32_t as32;
typedef _Atomic int64_t as64;
#endif // __cplusplus