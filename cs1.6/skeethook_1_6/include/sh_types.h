#pragma once

#define SH_NAME "skeethook_1_6"
#define SH_VERSION "1.0.0"

#define SH_EXPORT extern "C" __declspec(dllexport)

#if defined(_MSC_VER)
#define sh_forceinline __forceinline
#define sh_noreturn __declspec(noreturn)
#else
#define sh_forceinline inline __attribute__((always_inline))
#define sh_noreturn __attribute__((noreturn))
#endif

#define sh_countof(a) (sizeof(a) / sizeof((a)[0]))
#define sh_align(v, a) (((v) + ((a)-1)) & ~((a)-1))

typedef unsigned char u8;
typedef unsigned short u16;
typedef unsigned int u32;
typedef unsigned long long u64;
typedef signed char i8;
typedef short i16;
typedef int i32;
typedef long long i64;
typedef unsigned char byte;
typedef float f32;
typedef double f64;
typedef bool qbool;

typedef struct vec2_s { f32 x, y; } vec2_t;
typedef struct vec3_s { f32 x, y, z; } vec3_t;
typedef struct vec4_s { f32 x, y, z, w; } vec4_t;

typedef struct col4_s { u8 r, g, b, a; } col4_t;

#define SH_COL_RGB(r, g, b) { (u8)(r), (u8)(g), (u8)(b), 255 }
#define SH_COL_RGBA(r, g, b, a) { (u8)(r), (u8)(g), (u8)(b), (u8)(a) }
