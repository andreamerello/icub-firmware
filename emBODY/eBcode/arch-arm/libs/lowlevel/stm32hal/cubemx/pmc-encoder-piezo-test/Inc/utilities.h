/*******************************************************************************************************************//**
 * @file    utilities.h
 * @author  G.Zini
 * @version 1.0
 * @date    2018 July, 24
 * @brief   General purpose utility functions
 **********************************************************************************************************************/

/* Define to prevent recursive inclusion ******************************************************************************/
#ifndef __UTILITIES_H
#define __UTILITIES_H

#ifdef __cplusplus
  extern "C" {
#endif

/* Include files ******************************************************************************************************/
#include <stdlib.h>
#include <stdint.h>
#include <stdbool.h>
#include <stdarg.h>


/* Exported typedefs **************************************************************************************************/

// Polar bidimensional coordinates
typedef struct
{
    int16_t radius ;
    int16_t theta ;
} tPolar_t ;

// Cartesian bidimensional coordinates
typedef struct
{
    int16_t x ;
    int16_t y ;
} tCartesian_t ;

// Wide-char definition (ATTENTION: do not use standard C-library functions with WCHAR)
#ifdef WCHAR_ENABLE
    typedef wchar_t WCHAR ;
#endif

// Fixed point types (also defined in "arm_math.h" file)

// Fixed point 1.7 format
// LSB = 1/2^7 (7.8125e-3)
// Range from -1 to 0,9921875
#ifndef q7_t
    typedef int8_t q7_t ;
#endif

// Fixed point 1.15 format
// LSB = 1/2^15 (3.0517578125e-5)
// Range from -1 to 0,9999847412109375
#ifndef q15_t
    typedef int16_t q15_t ;
#endif

// Fixed point 1.31 format
// LSB = 1/2^31 (4.656612873077392578125e-10)
// Range from -1 to 0.9999999995343387126922607421875
#ifndef q31_t
    typedef int32_t q31_t ;
#endif

// Fixed point 1.63 format
// LSB = 1/2^63 (1.0842021724855044340074528008699e-19)
// Range from -1 to 0.99999999999999999989157978275145
#ifndef q63_t
    typedef int64_t q63_t ;
#endif


/* Exported macro *****************************************************************************************************/

// Save/Restore interrupt enable status.
// Caution: do not use those in FreeRTOS environment unless you need a complete interrupt suspension
#define __BEGIN_CRITICAL()      {volatile unsigned __primask=__get_PRIMASK();__disable_irq();do{}while(0)
#define __END_CRITICAL()        __set_PRIMASK(__primask);}do{}while(0)
#define __EXIT_CRITICAL()       __set_PRIMASK(__primask);do{}while(0)
#define __ENTER_CRITICAL()      __disable_irq();do{}while(0)

// Number of items in an array
#ifndef lengthof
#define lengthof(array)         (sizeof(array)/sizeof((array)[0]))
#endif

// Offset of a member in a structure
#ifndef offsetof
#define offsetof(type,member)   ((int32_t)&(((type*)0)->member))
#endif

// Force in not-initialized area of RAM
#ifndef __NOINIT__
  #define __NOINIT__    __attribute__((section (".noinitdata")))
#endif

/* Force inline function */
#ifndef __ALWAYS_INLINE
  #define __ALWAYS_INLINE __attribute__((always_inline)) static inline
#endif

///* Avoid "unused argument" warning */
//#ifndef UNUSED
//  #define UNUSED(x)     ((void)(x))
//#endif

// Test for NULL pointer with forced "volatile" specifier
#define IS_NULL(ptr)            (((volatile void *)NULL) == ((volatile void *)(ptr)))

// Forced "volatile" access. It is an l-value
#define FORCE_ACCESS(name)      (*((volatile typeof(name) *)&(name)))

// Assign a variable to the ".noinit" section
#define _NOINIT                 __attribute__((section(".noinit")))

// Pointer to a struct type, given the pointer to one of its member
#define _BASE(type,mbr,ptr)     ((type*)&(((char*)(ptr))[-(int)&(((type*)0)->mbr)]))

// Get an aligned pointer to the next or the previous address boundary
#define align_up(x,t)           ((void*)(((unsigned)(x)+(sizeof(t)-1))&~(sizeof(t)-1)))
#define align_down(x,t)         ((void*)((unsigned)(x)&~(sizeof(t)-1)))

// Exchange contents of variables
#define exchange(x,y)           do{typeof(x)t=x;x=y;y=t;}while(0)

// Get the bit-band address pointer
#define BIT_BAND(base,ptr,bitn) \
    (*(volatile uint32_t *)(32*((uint32_t)ptr-(uint32_t)base)+4*(uint32_t)bitn+(uint32_t)base+0x02000000L))

// Get the BIT-BAND address pointer in SRAM memory area
#define MEMORY_BIT_BAND(ptr,bitn)   BIT_BAND(0x20000000,(ptr),(bitn))
#define _MBB(ptr,bitn)              BIT_BAND(0x20000000,(ptr),(bitn))

// Get the BIT-BAND address pointer in peripheral area
#define PERIPH_BIT_BAND(ptr,bitn)   BIT_BAND(0x40000000,(ptr),(bitn))
#define _PBB(ptr,bitn)              BIT_BAND(0x40000000,(ptr),(bitn))

// Short version of PERIPH_BIT_BAND macro for peripheral registers and single bit mask
#define REG_BIT(reg,mask)           PERIPH_BIT_BAND(&(reg),LSB(mask))

// Get the MSB of an integer. The compiler generates an immediate constant value when the argument is a constant
// value known at compile time. Use macro msb(x) for not constant values
// @param   x   Integer constant value
// @return      Bit number of the left-most not-zero bit of the argument, or -1 when the argument is 0
#define MSB(x)      ((x)?MSB$64(((uint64_t)(x))):-1)
#define MSB$64(x)   (x&0xFFFFFFFF00000000LL?(32+MSB$32(x>>32)):MSB$32(x))
#define MSB$32(x)   (x&0x00000000FFFF0000LL?(16+MSB$16(x>>16)):MSB$16(x))
#define MSB$16(x)   (x&0x000000000000FF00LL?(8+MSB$8(x>>8)):MSB$8(x))
#define MSB$8(x)    (x&0x00000000000000F0LL?(4+MSB$4(x>>4)):MSB$4(x))
#define MSB$4(x)    (x&0x000000000000000CLL?(2+MSB$2(x>>2)):MSB$2(x))
#define MSB$2(x)    (x&0x0000000000000002LL?1:0)

// Get the LSB of an integer. The compiler generates an immediate constant value when the argument is a constant
// value known at compile time. Use macro lsb(x) for not constant values
// @param   x   Integer constant value
// @return      Bit number of the right-most not-zero bit of the argument, or -1 when the argument is 0
#define LSB(x)      ((x)?LSB$64(((uint64_t)(x))):-1)
#define LSB$64(x)   (x&0x00000000FFFFFFFFLL?(LSB$32(x)):(32+LSB$32(x>>32)))
#define LSB$32(x)   (x&0x000000000000FFFFLL?(LSB$16(x)):(16+LSB$16(x>>16)))
#define LSB$16(x)   (x&0x00000000000000FFLL?(LSB$8(x)):(8+LSB$8(x>>8)))
#define LSB$8(x)    (x&0x000000000000000FLL?(LSB$4(x)):(4+LSB$4(x>>4)))
#define LSB$4(x)    (x&0x0000000000000003LL?(LSB$2(x)):(2+LSB$2(x>>2)))
#define LSB$2(x)    (x&0x0000000000000001LL?0:1)

// Return the position of the MSB or LSB of a variable. Those macros should be used for not-constants values
#define msb(x)      ((signed)(31U-(unsigned)__CLZ((uint32_t)(x))))
#define lsb(x)      (__CLZ(__RBIT((uint32_t)(x))))

// Calculates the bit-field value, given the mask
#define BITFIELD(mask,value)    ((mask)&((value)<<LSB(mask)))

// Maximum and minimum values for a given integer variable
#define MAX_SIGNED_OF(var)      ((typeof(var))(~0<<(8*sizeof(var)-1)))
#define MIN_SIGNED_OF(var)      ((typeof(var))(1<<(8*sizeof(var)-1)))
#define MAX_UNSIGNED_OF(var)    ((typeof(var))(~0))

// Arithmetic utilities (rounded to nearest unsigned/signed division)
#define URDIV(x,y)      (((x)+(y)/2)/(y))
#define RDIV(x,y)       ((x)>=0?(((x)+abs(y)/2)/(y)):(((x)-abs(y)/2)/(y)))

// Signed integer multiply and accumulate (instructions SMULL/SMLAL)
#define SMULTADD(a,x,y) (((int64_t)(a))+(((int64_t)(x))*((int64_t)(y))))

// Unsigned integer multiply and accumulate (instruction UMULL/UMLAL)
#define UMULTADD(a,x,y) (((uint64_t)(a))+(((uint64_t)(x))*((uint64_t)(y))))

// Cast and rounding of a float
#define ROUND(type,x)   ((type)((double)(x)>=0.0?(double)(x)+0.5:(double)(x)-0.5))

// float to integer translation, with 'n' bit fractional part and rounding to nearest
#define FIX(x,n)        ((int32_t)((double)(x)*(double)(1<<(n))+0.5))

// Q32.16 format conversion
#define Q31_16(x)       (ROUND(int32_t, ((double)(x) * 65536.0)))
#define _1_             (Q31_16(1.0))

// q7_t, q15_t, q31_t, q63_t conversions
#define _Q7(x)          (ROUND(int8_t, ((double)(x) * 128.0)))
#define _Q15(x)         (ROUND(int16_t, ((double)(x) * 32768.0)))
#define _Q31(x)         (ROUND(int32_t, ((double)(x) * 2147483648.0)))
#define _Q63(x)         (ROUND(int64_t, ((double)(x) * 9223372036854775808.0)))

// Rounding and right shift
#define RSHIFT(v,n)     (((n)>0)?(((v)+(1LL<<((n)-1)))>>(n)):(v))

// Multiplication with rounding and right shift
#define MULRSH(x,y,n)   RSHIFT(((int32_t)(x))*((int32_t)(y)),(n))

// Saturation within an interval
#define SATURATE(min,max,x)     (((x)<(min))?(min):(((x)>(max))?(max):(x)))

// Mathematical constants
#ifdef PI
  #undef PI
#endif
#define PI      (3.1415926535897932384626433832795)
#define SQRT2   (1.4142135623730950488016887242097)
#define _E      (2.7182818284590452353602874713527)

// Useful constants
#define KBYTE   (1024u)
#define MBYTE   (1048576u)
#define GBYTE   (1073741824u)

// Some useful costants for functions tSin, tCos, sin32, cos32
#define _0_DEGREES      (0)
#define _45_DEGREES     (8192)
#define _90_DEGREES     (16384)
#define _135_DEGREES    (24576)
#define _180_DEGREES    (-32768)
#define _225_DEGREES    (-24576)
#define _270_DEGREES    (-16384)
#define _315_DEGREES    (-8192)
#define _360_DEGREES    (0)
#define __DEGREES(x)    (ROUND(int16_t,((double)(x)*32768.0/180.0)))
#define __RADIANTS(x)   (ROUND(int16_t,((double)(x)*32768.0/PI)))


/* Exported functions prototypes **************************************************************************************/

extern char *ultoa(char *buffer, uint32_t number) ;
extern char *ulltoa(char *buffer, uint64_t number) ;
extern char *ultox(char *buffer, uint32_t number) ;
extern char *sltoa(char *buffer, int32_t number) ;
extern char *slltoa(char *buffer, int64_t number) ;
extern char *ftoa(char *szBuf, double fNbr, int iFrac) ;
extern char *EditString(void (*pfTx)(char), char (*pfRx)(void), char *str, size_t size) ;
extern char skipblank(const char **pCur) ;
extern bool charcmp(char ch, const char **pCur) ;
extern bool tokencmp(const char *tok, const char **pCur) ;
extern bool atoul(const char **pCur, uint32_t *pNbr) ;
extern bool atosl(const char **pCur, int32_t *pNbr) ;
extern bool xtoul(const char **pCur, uint32_t *pNbr) ;
extern uint16_t SqrtU32(uint32_t Rad, uint32_t *pRem) ;
extern int16_t tSin(int16_t alpha) ;
#define tCos(x) (tSin((x)+0x4000))
extern int32_t sin32( int16_t alpha ) ;
#define cos32(x) (sin32((x)+0x4000))
extern int tiny_vprintf( void (*fp)(char), const char *fmt, va_list ap) ;

#ifdef WCHAR_ENABLE
extern unsigned wsize(const WCHAR *pSrc) ;
extern WCHAR *wcpy(WCHAR *pDst, const WCHAR *pSrc) ;
extern WCHAR *wcat(WCHAR *pDst, const WCHAR *pSrc) ;
#endif

#ifdef __cplusplus
  } /* extern "C" */
#endif

#endif /* __UTILITIES_H */
/* END OF FILE ********************************************************************************************************/
