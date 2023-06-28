/* //////////////////////////////////////////////////////////////////////////////

FILE: MathFixed.h

GENERAL DESCRIPTION: Fixed point math defines and includes

                Copyright (c) 1999-2004 QUALCOMM Incorporated.
                             All Rights Reserved.
                           QUALCOMM Proprietary/GTDR

				
////////////////////////////////////////////////////////////////////////////// */
#ifndef _MATHFIXED_H_
#define _MATHFIXED_H_

#ifdef __cplusplus
extern "C"
{
#endif

// ---------------------------------------------
// const
// ---------------------------------------------
#define F_FACTOR                16

#define F_ZERO					0x0	        //0	
#define F_ONE					0x00010000  //1
#define F_TWO					0x00020000  //2
#define F_THREE 				0x00030000  //3
#define F_FOUR					0x00040000  //4
#define F_HALF					0x00008000  //1/2
#define F_QUARTER				0x00004000  //1/4
#define F_RECIP255				0x00000101	//1/255
#define F_PI					0x0003243F	//pi
#define F_2PI					0x0006487F	//2pi
#define F_4PI					0x000C90FE	//4pi
#define F_PIDIV2				0x00019220	//pi/2
#define F_PIDIV4				0x0000C910	//pi/4
#define F_3PIDIV2               0x0004B65F  //3*pi/2
#define F_INVPI					0x0000517D	//1/pi
#define F_E                     0x0002B7E1  //e
#define F_SQRT2					0x00016A0A	//sqrt(2)
#define F_SQRT3					0x0001BB68	//sqrt(3)
#define F_SQRTHALF				0x0000B505	//1/sqrt(2)
#define F_INVSQRT3				0x000093CD	//1/sqrt(3)
#define F_DEGTORAD				0x00000478  //degree->radian
#define F_RADTODEG				0x00394BB8  //radian->degree
#define F_ONEOVERK16		    0x00009B74
#define F_HUGE					0x7FFFFFFE
#define F_MAX        	        0x7FFFFFFF
#define F_MIN			        ((Fixed)0x80000000)
#define F_MAX_TANGENT           0x03553A00
#define F_MIN_TANGENT           ((Fixed)0xFCAC7080)
#define F_MAX_16                0x7fff
#define F_MIN_16                ((Word16)0x8000)

// ---------------------------------------------
// math operators
// ---------------------------------------------
#define F_ADD(v1,v2)			((v1)+(v2))
#define F_SUB(v1,v2)			((v1)-(v2))
#define F_MUL(v1,v2)			(((v1)>(v2))? ((Fixed)((((int64)((v1)>>1)) * (v2)) >> (F_FACTOR-1))) : ((Fixed)( (((int64)((v2)>>1)) * (v1) ) >> (F_FACTOR-1) )))
#define F_DIV(v1,v2)			(Fixed)(((int64)(v1) << F_FACTOR) / (v2))
#define F_SQ(v1)				F_MUL((v1),(v1))
#define F_SQRT(v)				mathfx_sqrtFx(v)
#define F_LOG(b,i)              mathfx_logFx((b), (i))
#define F_POW(b,p)              mathfx_powFx((b), (p))
#define F_SIN(v)				mathfx_sinFx(v)
#define F_COS(v)				mathfx_cosFx(v)
#define F_SINCOS(theta,s,c)		mathfx_sinCosFx(theta,s,c)
#define F_TAN(v)				mathfx_tanFx(v)
#define F_ACOS(v)				mathfx_aCosFx(v)
#define F_ASIN(v)				mathfx_aSinFx(v)
#define F_ATAN(v)				mathfx_aTanFx(v)
#define F_ATAN2(a,b)			mathfx_aTan2Fx(a,b)
#define F_ABS(v)				(((v) > F_ZERO)? (v) : -(v))

// ---------------------------------------------
// common typedefs
// ---------------------------------------------

typedef int32   Fixed;
typedef int16   Word16;
typedef int32   Word32;

typedef struct
{
    Fixed       x;
    Fixed       y;
    Fixed       z;
} TVector;

// ---------------------------------------------
// fixed point conversion macros
// ---------------------------------------------
#define _FAST_FIXED_TO_FLOAT    1
#if _FAST_FIXED_TO_FLOAT
Fixed   F2X(float f);
#else
#define F2X(x)                  ((int32)((x)*65536.0f))
#endif

#define I2X(x)                  ((x)<<16)
#define X2I(x)                  ((x)>>16)

// ---------------------------------------------
// operator implementation functions
// ---------------------------------------------
Word32 mathfx_L_shl(Word32 L_var1, Word16 var2); /* Long shift left,     2 */
Word32 mathfx_L_shr(Word32 L_var1, Word16 var2); /* Long shift right,    2 */
Word32 mathfx_aCosFx(Word32 val);
Word32 mathfx_aSinFx(Word32 val);
Word32 mathfx_aTan2Fx(Word32 x, Word32 y);
Word32 mathfx_aTanFx(Word32 val);
Word32 mathfx_cosFx(Word32 val);
Word32 mathfx_expFx(Word32 power);
Word32 mathfx_logFx(Word32 base, Word32 input);
Word32 mathfx_powFx(Word32 base, Word32 power);
void   mathfx_sinCosFx(Word32 val, Word32 *sin, Word32 *cos);
void   mathfx_sinCosQFx(Word32 val, Word32 accuracyFactor, Word32 *sin, Word32 *cos);
Word32 mathfx_sinFx(Word32 val);
Word32 mathfx_sqrtFx(Word32 val);
Word32 mathfx_sqrtQFx(Word32 val, Word32 QFactor);
Word32 mathfx_tanFx(Word32 val);
Word32 mathfx_divFx(Word32 x, Word32 y);
Word32 mathfx_multFx(Word32 x, Word32 y);
Word32 mathfx_Pow2(        /* (o) Q0  : result       (range: 0<=val<=0x7fffffff) */
  Word16 exponent,  /* (i) Q0  : Integer part.      (range: 0<=val<=30)   */
  Word16 fraction   /* (i) Q15 : Fractional part.   (range: 0.0<=val<1.0) */
);
void mathfx_Log2(
  Word32 L_x,       /* (i) Q0 : input value                                 */
  Word16 *exponent, /* (o) Q0 : Integer part of Log2.   (range: 0<=val<=30) */
  Word16 *fraction  /* (o) Q15: Fractional  part of Log2. (range: 0<=val<1) */
);

// ---------------------------------------------
// Vector Calculation Functions
// ---------------------------------------------
void VectorMul(TVector vector, Fixed scalar, TVector* pResult);
void VectorAdd(TVector vector0, TVector vector1, TVector* pResult);
void VectorSub(TVector vector0, TVector vector1, TVector* pResult);
Fixed VectorDot(TVector vector0, TVector vector1);
void VectorCross(TVector vector0, TVector vector1, TVector* pResult);
void VectorNormalize(TVector* pVector);


#ifdef __cplusplus
}
#endif

#endif //_MATHFIXED_H_

