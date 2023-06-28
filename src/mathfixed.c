/*=================================================================================
FILE:           MathFixed.c
  
DESCRIPTION:    This file provides fixed point math functions
                          
ABSTRACT:       This file contains the implementation of basic fixed point
                math functions.  

AUTHOR:         QUALCOMM
                        
                Copyright (c) 2004, 2005, 2006 QUALCOMM Incorporated.
                       All Rights Reserved.
                    QUALCOMM Proprietary/GTDR
=================================================================================*/

#include "AEEComdef.h"
#include "mathfixed.h"

/*-------------------------------------------------------------------------------*
 *                           C O N S T A N T S                                   *
 *-------------------------------------------------------------------------------*/

// Table for Arctan16 is in Q16 for trig computations.
const uint16 mathfx_arctanTable16[17] = {
    0xC910, 0x76B2, 0x3EB7, 0x1FD6,
    0x0FFB, 0x07FF, 0x0400, 0x0200,
    0x0100, 0x0080, 0x0040, 0x0020,
    0x0010, 0x0008, 0x0004, 0x0002,
    0x0001
};

// Table for routine Pow2().                           
const Word16 mathfx_tabpow[33] = {
    16384, 16743, 17109, 17484, 17867, 18258, 18658, 19066, 19484, 19911,
    20347, 20792, 21247, 21713, 22188, 22674, 23170, 23678, 24196, 24726,
    25268, 25821, 26386, 26964, 27554, 28158, 28774, 29405, 30048, 30706,
    31379, 32066, 32767 
};

// Table for routine Log2().                   
const Word16 mathfx_tablog[33] = {
        0,  1455,  2866,  4236,  5568,  6863,  8124,  9352, 10549, 11716,
    12855, 13967, 15054, 16117, 17156, 18172, 19167, 20142, 21097, 22033,
    22951, 23852, 24735, 25603, 26455, 27291, 28113, 28922, 29716, 30497,
    31266, 32023, 32767 
};

/*-------------------------------------------------------------------------------*
 *                    C O N V E R S I O N   F U N C T I O N S                    *
 *-------------------------------------------------------------------------------*/

#if _FAST_FIXED_TO_FLOAT
/*===========================================================================

FUNCTION    F2X

DESCRIPTION
    convert a IEEE 754 floating point number into a fixed point approximation
    without using any floating point calculations.

RETURN VALUE
  Returns the resulting s15.16 fixed point value.

SIDE EFFECTS

===========================================================================*/
Fixed F2X(float f)
{
    //interpret our floating point as unsigned integer
    uint32 data = *(uint32*)&f;
    //extract the mantissa and add the leading 1
    uint32 mant = (data & 0x007fffff) | 0x00800000;
    //extract the exponent. de-normalize it (-127), shift by the precision of the mantissa (-23) and multiply by the precision of our fixed point (16)
    int8  exp   = (int8)((data >> 23) & 0xff) - 127 - 23 + 16;
    //shift our mantissa by the remaining exponent
    mant = exp > 0 ? mant << exp : (exp < -23 ? 0x1 : mant >> (-exp));
    //apply sign bit
    return (data & 0x80000000) ? 0-mant : mant;
}
#endif  //_FAST_FIXED_TO_FLOAT

/*===========================================================================

FUNCTION    mathfx_sinCosQFx

DESCRIPTION
  Simultaneously computes the fixed point Sin and Cos of the radian val.
  Algorithm developed from the book:
  Title:   Computer Arithmetic Algorithms (2nd Edition)
  Author:  Israel Koren 
  ISBN:    1-56881-160-8
  PP:      232-235

DEPENDENCIES
  Input value assumed not normalized to PI Q16 format

RETURN VALUE
  Returns the resulting Q16 Sin and Q16 Cos of Q16 Radian val.

SIDE EFFECTS

===========================================================================*/
void mathfx_sinCosQFx(int32 val, int32 accuracyFactor, int32 *sin, int32 *cos)
{

    int64 x64,s64,r64,w64,z64,newz64;
    int32 x,s,r,w,z,newz, sin_Qfactor, sin_Qdiff;
    int i, shift, quadrant;

    // Trivial 0 check
    if(val == 0) {
        *sin = 0;
        *cos = 1<<F_FACTOR;
        return;
    }

    sin_Qfactor = accuracyFactor;
    sin_Qdiff = F_FACTOR - sin_Qfactor;

    // Error check
    if( sin_Qfactor > F_FACTOR )
        return;

    if( sin_Qfactor < 15 )
    {
        x = (val>>sin_Qdiff);
        s = 1;
     
        z = (F_ONEOVERK16>>sin_Qdiff);
        w = 0;
    
        // Compute Quadrant
        // Normalize to 2Pi
        x = x%(F_2PI>>sin_Qdiff);
    
        if(x < 0) {
            x = -x;
            r = -1;
        }
        else r = 1;
    
        if(x < (F_PIDIV2>>sin_Qdiff))
            quadrant = 0;
        else if(x < (F_PI>>sin_Qdiff))
        {
            quadrant = 1;
            x = (F_PI>>sin_Qdiff)-x;
        }
        else if(x < (F_3PIDIV2>>sin_Qdiff))
        {
            quadrant = 2;
            x -= (F_PI>>sin_Qdiff);
        }
        else
        {
            quadrant = 3;
            x = (F_2PI>>sin_Qdiff)-x;   
        }
    
        // Compute sin and cos with domain 0 < x < Pi/2
        for(i = 0; i < (sin_Qfactor+1); i++)
        {
            shift = (sin_Qfactor)-i;
            if(x < 0)
            {
                s = -1;
            }
            else s = 1;
            newz = z - ((s*(1<<shift) * w)>>sin_Qfactor);
            w = w + ((s*(1<<shift) * z) >> sin_Qfactor);
            z = newz;
            x = x - ((s*(mathfx_arctanTable16[i]>>sin_Qdiff)));
        }
    
        // Clamp Sin and Cos to between -1 and 1 (precision issues can cause small overruns)
        if(w > (1 << (F_FACTOR - sin_Qdiff)))
        {
            w = (1 << (F_FACTOR - sin_Qdiff));
        }
        else if(w < -(1 << (F_FACTOR - sin_Qdiff)))
        {
            w = -(1 << (F_FACTOR - sin_Qdiff));
        }
        if(z > (1 << (F_FACTOR - sin_Qdiff)))
        {
            z = (1 << (F_FACTOR - sin_Qdiff));
        }
        else if(z < -(1 << (F_FACTOR - sin_Qdiff)))
        {
            z = -(1 << (F_FACTOR - sin_Qdiff));
        }
    
        switch(quadrant)
        {
            case 0:
                *sin =  (int32)(r*w<<sin_Qdiff);
                *cos =  (int32)(z<<sin_Qdiff);
                break;
            case 1:
                *sin =  (int32)(r*w<<sin_Qdiff);
                *cos =  (int32)(-z<<sin_Qdiff);
                break;
            case 2:
                *sin =  (int32)(r*-w<<sin_Qdiff);
                *cos =  (int32)(-z<<sin_Qdiff);
                break;
            case 3:
                *sin =  (int32)(r*-w<<sin_Qdiff);
                *cos =  (int32)(z<<sin_Qdiff);
                break;
        }
    }
    else
    {
        x64 = (val>>sin_Qdiff);
        s64 = 1;    
    
        z64 = (F_ONEOVERK16>>sin_Qdiff);
        w64 = 0;    
    
        // Compute Quadrant
        // Normalize to 2Pi
        x64 = x64%(F_2PI>>sin_Qdiff);
    
        if(x64 < 0) {
            x64 = -x64;
            r64 = -1;
        }
        else r64 = 1;
    
        if(x64 < (F_PIDIV2>>sin_Qdiff))
            quadrant = 0;
        else if(x64 < (F_PI>>sin_Qdiff))
        {
            quadrant = 1;
            x64 = (F_PI>>sin_Qdiff)-x64;
        }
        else if(x64 < (F_3PIDIV2>>sin_Qdiff))
        {
            quadrant = 2;
            x64 -= (F_PI>>sin_Qdiff);
        }
        else
        {
            quadrant = 3;
            x64 = (F_2PI>>sin_Qdiff)-x64;   
        }
    
        // Compute sin and cos with domain 0 < x < Pi/2
        for(i = 0; i < (sin_Qfactor+1); i++)
        {
            shift = (sin_Qfactor)-i;
            if(x64 < 0)
            {
                s64 = -1;
            }
            else s64 = 1;
            newz64 = z64 - ((s64*(1<<shift) * w64)>>sin_Qfactor);
            w64 = w64 + ((s64*(1<<shift) * z64) >> sin_Qfactor);
            z64 = newz64;
            x64 = x64 - ((s64*(mathfx_arctanTable16[i]>>sin_Qdiff)));
        }
    
        // Clamp Sin and Cos to between -1 and 1 (precision issues can cause small overruns)
        if(w64 > (1 << (F_FACTOR - sin_Qdiff)))
        {
            w64 = (1 << (F_FACTOR - sin_Qdiff));
        }
        else if(w64 < -(1 << (F_FACTOR - sin_Qdiff)))
        {
            w64 = -(1 << (F_FACTOR - sin_Qdiff));
        }
        if(z64 > (1 << (F_FACTOR - sin_Qdiff)))
        {
            z64 = (1 << (F_FACTOR - sin_Qdiff));
        }
        else if(z64 < -(1 << (F_FACTOR - sin_Qdiff)))
        {
            z64 = -(1 << (F_FACTOR - sin_Qdiff));
        }
    
        switch(quadrant)
        {
            case 0:
                *sin =  (int32)(r64*w64<<sin_Qdiff);
                *cos =  (int32)(z64<<sin_Qdiff);
                break;
            case 1:
                *sin =  (int32)(r64*w64<<sin_Qdiff);
                *cos =  (int32)(-z64<<sin_Qdiff);
                break;
            case 2:
                *sin =  (int32)(r64*-w64<<sin_Qdiff);
                *cos =  (int32)(-z64<<sin_Qdiff);
                break;
            case 3:
                *sin =  (int32)(r64*-w64<<sin_Qdiff);
                *cos =  (int32)(z64<<sin_Qdiff);
                break;
        }
    }
}

/*===========================================================================

FUNCTION    mathfx_sinCosFx

DESCRIPTION
    Simultaneously computes the fixed point Sin and Cos of the radian val.
    Algorithm developed from the book:
    Title:   Computer Arithmetic Algorithms (2nd Edition)
    Author:  Israel Koren 
    ISBN:    1-56881-160-8
    PP:      232-235

DEPENDENCIES
    Input value assumed not normalized to PI Q16 format

RETURN VALUE
    Returns the resulting Q16 Sin and Q16 Cos of Q16 Radian val.

SIDE EFFECTS

===========================================================================*/
void mathfx_sinCosFx(int32 val, int32 *sin, int32 *cos)
{
    mathfx_sinCosQFx(val, (F_FACTOR-2), sin, cos);
}


/*===========================================================================

FUNCTION    mathfx_sinFx

DESCRIPTION
    Computes the fixed point Sin of the parameter val.

DEPENDENCIES
    Input value assumed not normalized to PI Q16 format

RETURN VALUE
    Returns the resulting Q16 Sin of val.

SIDE EFFECTS

===========================================================================*/
int32 mathfx_sinFx(int32 val)
{
    int32 sin,cos;
    mathfx_sinCosFx(val, &sin, &cos);
    return(sin);
}


/*===========================================================================

FUNCTION    mathfx_cosFx

DESCRIPTION
    Computes the fixed point Cos of the parameter val.

DEPENDENCIES
    None

RETURN VALUE
    Returns the resulting Q16 Cos of val.

SIDE EFFECTS

===========================================================================*/
int32 mathfx_cosFx(int32 val)
{
    int32 sin,cos;
    mathfx_sinCosFx(val, &sin, &cos);
    return(cos);
}


/*===========================================================================

FUNCTION    mathfx_tanFx

DESCRIPTION
  Computes the fixed point Tangent of the parameter val.
  Algorithm developed from the book:
  Title:   Computer Arithmetic Algorithms (2nd Edition)
  Author:  Israel Koren 
  ISBN:    1-56881-160-8

DEPENDENCIES
  None

RETURN VALUE
  Returns the resulting Q16 tangent of val.

SIDE EFFECTS

===========================================================================*/
int32 mathfx_tanFx(int32 val)
{

#define TAN_Q_FACTOR    14
#define TAN_Q_DIFF      (F_FACTOR - TAN_Q_FACTOR)

#if (TAN_Q_FACTOR > F_FACTOR)
#error Tangent Q Factor greater than System Q Factor not allowed.
#endif

#if(TAN_Q_FACTOR > 14)
    int64 x,s,w,z,newz;
#else
    int32 x,s,w,z,newz;
#endif

    int i, shift, quadrant;

    x = (val>>TAN_Q_DIFF);
    s = 1;

    z = (F_ONEOVERK16>>TAN_Q_DIFF);
    w = 0;

    if(x < 0)
    {
        x = -x;
        quadrant = -1;
    }
    else quadrant = 1;

    // Compute Quadrant
    // Normalize to Pi
    x = x%(F_PI>>TAN_Q_DIFF);

    if(x > (F_PIDIV2>>TAN_Q_DIFF))
    {
        quadrant = -quadrant;
        x = (F_PIDIV2>>TAN_Q_DIFF) - 
            (x - (F_PIDIV2>>TAN_Q_DIFF));
    }

    // Compute sin with domain 0 < x < Pi/2
    for(i = 0; i < (TAN_Q_FACTOR+1); i++)
    {
        shift = TAN_Q_FACTOR-i;
        if(x < 0)
        {
            s = -1;
        }
        else s = 1;
        newz = z - ((s*(1<<shift) * w)>>TAN_Q_FACTOR);
        w = w + ((s*(1<<shift) * z) >> TAN_Q_FACTOR);
        z = newz;
        x = x - (s*(mathfx_arctanTable16[i]>>TAN_Q_DIFF));
    }

    return ((quadrant*(w<<TAN_Q_FACTOR)/z)<<TAN_Q_DIFF);
}


/*===========================================================================

FUNCTION    mathfx_aSinFx

DESCRIPTION
  Computes the fixed point Arc Sin of the parameter val.
  Algorithm developed from the book:
  Title:   Computer Arithmetic Algorithms (2nd Edition)
  Author:  Israel Koren 
  ISBN:    1-56881-160-8

DEPENDENCIES
  None

RETURN VALUE
  Returns the resulting Q16 arc sine of val.

SIDE EFFECTS

===========================================================================*/
int32 mathfx_aSinFx(int32 val)
{

#define ARCSIN_Q_FACTOR 14
#define ARCSIN_Q_DIFF       (F_FACTOR - ARCSIN_Q_FACTOR)

#if(ARCSIN_Q_FACTOR > F_FACTOR)
#error ArcSin Q Factor greater than System Q Factor not allowed.
#endif

#if (ARCSIN_Q_FACTOR > 14)
    int64 u,v,s,y, sin, newv,newv2;
#else
    int32 u,v,s,y, sin, newv,newv2;
#endif

    int i, shift;

    sin = val>>ARCSIN_Q_DIFF;

    // Real values of arcSin only between 1 and -1
    if((sin < -1<<ARCSIN_Q_FACTOR) || (sin > 1<<ARCSIN_Q_FACTOR))
        return 0xFFFFFFFF;

    u = F_ONEOVERK16>>ARCSIN_Q_DIFF;
    v = 0;
    y = 0;

    for(i = 0; i < (ARCSIN_Q_FACTOR+1); i++)
    {
        shift = ARCSIN_Q_FACTOR-i;
        newv  = v + (((1<<shift) * u) >> ARCSIN_Q_FACTOR);
        newv2 = v - (((1<<shift) * u) >> ARCSIN_Q_FACTOR);
        if(F_ABS(newv2 - sin) < F_ABS(newv - sin))
        {
            newv = newv2; s = -1;
        }
        else
            s = 1;
        u = u - (s*(((1 << shift) * v) >> ARCSIN_Q_FACTOR));
        v = newv;
        y = y - (s*(mathfx_arctanTable16[i]>>ARCSIN_Q_DIFF));
    }

    y = -y;

    return(y<<ARCSIN_Q_DIFF);
}

/*===========================================================================

FUNCTION    mathfx_aCosFx

DESCRIPTION
  Computes the fixed point Arc Cosine of the parameter val.
  Algorithm developed from the book:
  Title:   Computer Arithmetic Algorithms (2nd Edition)
  Author:  Israel Koren 
  ISBN:    1-56881-160-8

DEPENDENCIES
  None

RETURN VALUE
  Returns the resulting fixed point Arc Cosine of the parameter val.

SIDE EFFECTS

===========================================================================*/
int32 mathfx_aCosFx(int32 val)
{
#define ARCCOS_Q_FACTOR 14
#define ARCCOS_Q_DIFF       (F_FACTOR - ARCCOS_Q_FACTOR)

#if(ARCCOS_Q_FACTOR > F_FACTOR)
#error ArcCos Q Factor greater than System Q Factor not allowed.
#endif

#if (ARCCOS_Q_FACTOR > 14)
    int64 cos, u,v,s,y, newu,newu2;
#else
    int32 cos, u,v,s,y, newu,newu2;
#endif

    int i, shift, ci;

    cos = val>>ARCCOS_Q_DIFF;

    // Real values of arcCos only between 1 and -1
    if((cos < -1<<ARCCOS_Q_FACTOR) || (cos > 1<<ARCCOS_Q_FACTOR))
        return 0xFFFFFFFF;

    if(cos < 0)
    {
        cos = -cos;
        ci = 1;
    }
    else ci = 0;

    u = F_ONEOVERK16>>ARCCOS_Q_DIFF;
    v = 0;
    y = 0;

    for(i = 0; i < (ARCCOS_Q_FACTOR+1); i++)
    {
        shift = (ARCCOS_Q_FACTOR)-i;
        newu  = u - (((1<<shift) * v) >> ARCCOS_Q_FACTOR);
        newu2 = u + (((1<<shift) * v) >> ARCCOS_Q_FACTOR);
        if(F_ABS(newu2 - cos) < F_ABS(newu - cos))
        {
            newu = newu2; s = -1;
        }
        else
            s = 1;
        v = v + (s*(((1 << shift) * u) >> ARCCOS_Q_FACTOR));
        u = newu;
        y = y - (s*(mathfx_arctanTable16[i]>>ARCCOS_Q_DIFF));
    }

    y = -y;
    if(ci)
    {
        y = (F_PIDIV2>>ARCCOS_Q_DIFF) + ((F_PIDIV2>>ARCCOS_Q_DIFF)-y);
    }
    return(y<<ARCCOS_Q_DIFF);
}

#define sqrtStep(shift)                                 \
    if((0x40000000l >> shift) + sqrtVal <= val)         \
    {                                                   \
        val -= (0x40000000l >> shift) + sqrtVal;        \
        sqrtVal = (sqrtVal >> 1) | (0x40000000l >> shift);    \
    }                                                   \
    else                                                \
    {                                                   \
        sqrtVal = sqrtVal >> 1;                         \
    }

/*===========================================================================

FUNCTION    mathfx_sqrtQFx

DESCRIPTION
    This square root routine uses the fact that fixed point numbers are really
    integers. The integer square root of a fixed point number divided by the
    square root of 2 to the F power where F is the number of bits of fraction
    in the fixed point number is the square root of the original fixed point
    number. If you have an even number of bits of fraction you can convert the
    integer square root to the fixed point square root with a shift.

DEPENDENCIES
    None

RETURN VALUE
    Returns the computed fixed point square root value.

SIDE EFFECTS

===========================================================================*/
int32 mathfx_sqrtQFx(int32 val, int32 QFactor)
{   
// Note: This fast square root function only works with an even F_FACTOR
    int32 sqrtVal = 0;
    
    // Check for even QFactor
    if(QFactor & 0x1)
    {
        // QFactor not even
        // Correct values to even Q Factor
        QFactor -= 1;
        val >>= 1;
    }
    sqrtStep(0);
    sqrtStep(2);
    sqrtStep(4);
    sqrtStep(6);
    sqrtStep(8);
    sqrtStep(10);
    sqrtStep(12);
    sqrtStep(14);
    sqrtStep(16);
    sqrtStep(18);
    sqrtStep(20);
    sqrtStep(22);
    sqrtStep(24);
    sqrtStep(26);
    sqrtStep(28);
    sqrtStep(30);
    
    if(sqrtVal < val)
    {
        ++sqrtVal;
    }
    
    sqrtVal <<= (QFactor)/2;
    if(QFactor < 16) 
      return(sqrtVal<<(16-QFactor));
    else
      return(sqrtVal>>(QFactor-16));
}


/*===========================================================================

FUNCTION    mathfx_sqrtFx

DESCRIPTION
    Computes the Q16 fixed point square root of the parameter val.

DEPENDENCIES
    Input value assumed normalized to Q16 format

RETURN VALUE
    Returns the resulting Q16 squart root of val.

SIDE EFFECTS

===========================================================================*/
int32 mathfx_sqrtFx(int32 val)
{
    return (mathfx_sqrtQFx(val,F_FACTOR));
}

/*===========================================================================

FUNCTION    mathfx_aTanFx

DESCRIPTION
    Computes the fixed point Arc tangent of the parameter val.
    Algorithm developed from the book:
    Title:   Computer Arithmetic Algorithms (2nd Edition)
    Author:  Israel Koren 
    ISBN:    1-56881-160-8

DEPENDENCIES
    Input value assumed normalized to Q16 format

RETURN VALUE
    Returns the resulting Q16 Arc tangent of the parameter val.

SIDE EFFECTS

===========================================================================*/
int32 mathfx_aTanFx(int32 val)
{
#define ARCTAN_Q_FACTOR 10
#define ARCTAN_Q_DIFF       (F_FACTOR - ARCTAN_Q_FACTOR)

#if(ARCTAN_Q_FACTOR > F_FACTOR)
#error ArcTan Q Factor greater than System Q Factor not allowed.
#endif

#if (ARCTAN_Q_FACTOR > 14)
    int64 v,u;
    int64 newv, newv2;
    int64 y, tangent;
#else
    int32 v,u;
    int32 newv, newv2;
    int32 y, tangent;
#endif

    int s,i;
    int shift;

    // Domain Check
    if(val > F_MAX_TANGENT)
    {
        return F_PIDIV2;
    }
    if(val < (int32)F_MIN_TANGENT )
    {
        return -(F_PIDIV2);
    }

    tangent = (val>>ARCTAN_Q_DIFF);

    u = 1<<ARCTAN_Q_FACTOR;
    v = tangent;
    y = 0;
    s = -1;

    for(i = 0; i < (ARCTAN_Q_FACTOR+1); i++)
    {
        shift = ARCTAN_Q_FACTOR-i;
        newv  = v + (((1<<shift) * u)>>ARCTAN_Q_FACTOR);
        newv2 = v - (((1<<shift) * u)>>ARCTAN_Q_FACTOR);
        if(F_ABS(newv2) < F_ABS(newv))
        {
            newv = newv2; s = -1;
        }
        else
        {
            s = 1;
        }
        u = u - ((s*(1<<shift) * v) >> ARCTAN_Q_FACTOR);
        v = newv;
        y = y - (s*(mathfx_arctanTable16[i]>>ARCTAN_Q_DIFF));
    }

    return (y<<ARCTAN_Q_DIFF);
}

/*===========================================================================

FUNCTION    mathfx_aTan2Fx

DESCRIPTION
    Computes the fixed point Arc tangent of the parameter x and y.

DEPENDENCIES
    Input values assumed normalized to Q16 format

RETURN VALUE
    Returns the resulting Q16 Arc tangent of the parameter x and y.

SIDE EFFECTS

===========================================================================*/
int32 mathfx_aTan2Fx(int32 x, int32 y)
{
    int32 val;
    if(x == 0 || y == 0)
        return 0;

    val = mathfx_divFx(x,y);

    return(mathfx_aTanFx(val));
}

/*===========================================================================

FUNCTION    mathfx_powFx

DESCRIPTION
    Computes the fixed point power of the parameter base and power.

DEPENDENCIES
    Input values assumed normalized to Q16 format

RETURN VALUE
    Returns the resulting Q16 power of the parameter base.

SIDE EFFECTS

===========================================================================*/
int32 mathfx_powFx(int32 base, int32 power)
{
    int16 baseInt, baseFrac;
    int32 interVal, returnVal;

    if(power == 0)
    {
        return 1<<F_FACTOR;
    }
    if(base == 0)
    {
        return 0;
    }

    mathfx_Log2(base, &baseInt, &baseFrac);

    baseInt-=F_FACTOR;
    interVal = (int32)baseInt<<15;
    interVal |= baseFrac;
    interVal = (int32)(((__int64)interVal*power)>>F_FACTOR);
    if((interVal >> 15) > 30)
    {
        return 0xFFFFFFFF;
    }

    baseInt = (int16)(interVal>>15);
    baseInt += F_FACTOR;
    baseFrac = (int16)(interVal & 0x7FFF);
    returnVal = mathfx_Pow2(baseInt,baseFrac);

    return(returnVal);
}

/*===========================================================================

FUNCTION    mathfx_expFx

DESCRIPTION
    Computes the fixed point exponential of the parameter power.

DEPENDENCIES
    Input values assumed normalized to Q16 format

RETURN VALUE
    Returns the resulting Q16 exponential of the parameter power.

SIDE EFFECTS

===========================================================================*/
int32 mathfx_expFx(int32 power)
{
    return (mathfx_powFx(F_E, power));
}



/*===========================================================================

FUNCTION    mathfx_logFx

DESCRIPTION
    Compute the fixed point Log of the parameter input with base of parameter base.
    Used identity 
        Logb(X) = Log2(X) / Log2(b)
    to get this log function to calculate log function with any log base and input
    based on the Log2 function.

DEPENDENCIES
    Input values assumed normalized to Q16 format

RETURN VALUE
    Returns the resulting Q16 log base the parameter base of parameter input

SIDE EFFECTS

===========================================================================*/
int32 mathfx_logFx(int32 base, int32 input)
{
    int16 baseInt, baseFrac;
    int16 inputInt, inputFrac;
    int32 baseVal, inputVal;

    if(base == 0)
        return 0;
    if(base == 1)
        return 1;

    mathfx_Log2(base, &baseInt, &baseFrac);
    baseVal = (int32)((baseInt-F_FACTOR)<<15) + baseFrac;

    mathfx_Log2(input, &inputInt, &inputFrac);
    inputVal = (int32)((inputInt-F_FACTOR)<<15) + inputFrac;

    return mathfx_divFx(inputVal, baseVal);
    
}

/*===========================================================================

FUNCTION    mathfx_divFx

DESCRIPTION
    Compute the fixed point division of the parameter input x as dividend 
    and y as divisor.

DEPENDENCIES
    Input values assumed normalized to Q16 format

RETURN VALUE
    Returns the resulting Q16 division of the parameter x and y.

SIDE EFFECTS

===========================================================================*/
int32 mathfx_divFx(int32 x, int32 y)
{
    return (int32)(( ( ( (__int64)(x) )<<16 ) ) / y);
}

/*===========================================================================

FUNCTION    mathfx_multFx

DESCRIPTION
    Compute the fixed point multiplication of the parameter x and y

DEPENDENCIES
    Input values assumed normalized to Q16 format

RETURN VALUE
    Returns the resulting Q16 multiplication of the parameter x and y.

SIDE EFFECTS

===========================================================================*/
int32 mathfx_multFx(int32 x, int32 y)
{
    if (x > y)
        return (int32)( (((__int64)(x>>1)) * y ) >> 15 );

    return (int32)( (((__int64)(y>>1)) * x ) >> 15 );
    
}


/*===========================================================================
 
FUNCTION    mathfx_L_shl

DESCRIPTION
  Arithmetically shift the 32 bit input L_var1 left var2 positions. Zero 
  fill the var2 LSB of the result. If var2 is negative, L_var1 right by  
  -var2 arithmetically shift with sign extension. Saturate the result in 
  case of underflows or overflows.                                       

DEPENDENCIES
    Input values assumed normalized to Q16 format

PARAMETERS                                                               
  L_var1 : 32 bit long signed integer (Word32) whose value falls in the  
           range : 0x8000 0000 <= L_var3 <= 0x7fff ffff.                 
  var2 : 16 bit short signed integer (Word16) whose value falls in the
         range : 0xffff 8000 <= var1 <= 0x0000 7fff.                  

RETURN VALUE
  L_var_out : 32 bit long signed integer (Word32) whose value falls in the
              range : 0x8000 0000 <= L_var_out <= 0x7fff ffff.            

SIDE EFFECTS

===========================================================================*/

Word32 mathfx_L_shl(Word32 L_var1, Word16 var2)
{
    Word32 L_var_out;

    /* initialization used only to suppress Microsoft Visual C++ warnings */
    L_var_out = 0L;

    if (var2 <= 0)
    {
        L_var_out = mathfx_L_shr(L_var1,(Word16)-var2);
    }
    else
    {
        for(;var2>0;var2--)
        {
            if (L_var1 > (Word32) 0X3fffffffL)
            {
                L_var_out = F_MAX;
                break;
            }
            else
            {
                if (L_var1 < (Word32) 0xc0000000L)
                {
                    L_var_out = F_MIN;
                    break;
                }
            }
            L_var1 *= 2;
            L_var_out = L_var1;
        }
    }

    return(L_var_out);

}


/*===========================================================================
 
FUNCTION    mathfx_L_shr

DESCRIPTION
  Arithmetically shift the 32 bit input L_var1 right var2 positions with 
  sign extension. If var2 is negative, arithmetically shift L_var1 left  
  by -var2 and zero fill the var2 LSB of the result. Saturate the result 
  in case of underflows or overflows.                                      

DEPENDENCIES
    Input values assumed normalized to Q16 format

PARAMETERS                                                               
  L_var1 : 32 bit long signed integer (Word32) whose value falls in the  
           range : 0x8000 0000 <= L_var3 <= 0x7fff ffff.                 
  var2 : 16 bit short signed integer (Word16) whose value falls in the
         range : 0xffff 8000 <= var1 <= 0x0000 7fff.                  

RETURN VALUE
  L_var_out : 32 bit long signed integer (Word32) whose value falls in the
              range : 0x8000 0000 <= L_var_out <= 0x7fff ffff.            

SIDE EFFECTS

===========================================================================*/

Word32 mathfx_L_shr(Word32 L_var1, Word16 var2)
{
    Word32 L_var_out;

    if (var2 < 0)
    {
        L_var_out = mathfx_L_shl(L_var1,(Word16)-var2);
    }
    else
    {
        if (var2 >= 31)
        {
            L_var_out = (L_var1 < 0L) ? -1 : 0;
        }
        else
        {
            if (L_var1<0)
            {
                L_var_out = ~((~L_var1) >> var2);
            }
            else
            {
                L_var_out = L_var1 >> var2;
            }
        }
    }
    return(L_var_out);
}

/*===========================================================================
 
FUNCTION    sature

DESCRIPTION
  Limit the 32 bit input to the range of a 16 bit word.

DEPENDENCIES
  Input values assumed normalized to Q16 format

PARAMETERS                                                               
  L_var1 : 32 bit long signed integer (Word32) whose value falls in the  
           range : 0x8000 0000 <= L_var3 <= 0x7fff ffff.                 

RETURN VALUE
  var_out : 16 bit short signed integer (Word16) whose value falls in the
            range : 0xffff 8000 <= var_out <= 0x0000 7fff.         

SIDE EFFECTS

===========================================================================*/
#define SATURE(var32_in,var16_out) \
{ \
    if (var32_in > 0X00007fffL) { \
        var16_out = (Word16)F_MAX_16; \
    } \
    else if (var32_in < (Word32)0xffff8000L) { \
        var16_out = (Word16)F_MIN_16; \
    } \
    else { \
        var16_out = (Word16)var32_in; \
    } \
}

/*===========================================================================
 
FUNCTION    sub

DESCRIPTION
  Performs the subtraction (var1+var2) with overflow control and satu-
  ration; the 16 bit result is set at +32767 when overflow occurs or at
  -32768 when underflow occurs.

DEPENDENCIES
  Input values assumed normalized to Q16 format

PARAMETERS                                                               
  var1 : 16 bit short signed integer (Word16) whose value falls in the 
         range : 0xffff 8000 <= var1 <= 0x0000 7fff.                   
  var2 : 16 bit short signed integer (Word16) whose value falls in the 
         range : 0xffff 8000 <= var1 <= 0x0000 7fff.                   

RETURN VALUE
  var_out : 16 bit short signed integer (Word16) whose value falls in the
            range : 0xffff 8000 <= var_out <= 0x0000 7fff.         

SIDE EFFECTS

===========================================================================*/

#define SUB(a16,b16,c16) \
{ \
    Word32 sub_diff = (Word32)(a16) - b16; \
    SATURE(sub_diff,c16); \
}


/*===========================================================================
 
FUNCTION    L_msu

DESCRIPTION
  Multiply var1 by var2 and shift the result left by 1. Subtract the 32
  bit result to L_var3 with saturation, return a 32 bit result:
    L_msu(L_var3,var1,var2) = L_sub(L_var3,(L_mult(var1,var2)).

DEPENDENCIES
  Input values assumed normalized to Q16 format

PARAMETERS                                                               
  L_var3 : 32 bit long signed integer (Word32) whose value falls in the
           range : 0x8000 0000 <= L_var3 <= 0x7fff ffff.  
  var1 : 16 bit short signed integer (Word16) whose value falls in the 
         range : 0xffff 8000 <= var1 <= 0x0000 7fff.                   
  var2 : 16 bit short signed integer (Word16) whose value falls in the 
         range : 0xffff 8000 <= var1 <= 0x0000 7fff.                   

RETURN VALUE
  L_var_out : 32 bit long signed integer (Word32) whose value falls in the
              range : 0x8000 0000 <= L_var_out <= 0x7fff ffff.        

SIDE EFFECTS

===========================================================================*/
 
#define L_MSU(var32,var16_1,var16_2,var32_out) \
{ \
   Word32 msu_product = (Word32)var16_1 * (Word32)var16_2; \
   Word32 msu_tmp32 = var32; \
   if (msu_product != (Word32)0x40000000L) { \
      msu_product *= 2; \
   } \
   else { \
      msu_product = F_MAX; \
   } \
   msu_tmp32 = var32 - msu_product; \
   if (((var32 ^ msu_product) & F_MIN) != 0) { \
      if ((msu_tmp32 ^ var32) & F_MIN) { \
         msu_tmp32 = (var32 < 0L) ? F_MIN : F_MAX; \
      } \
   } \
   var32_out = msu_tmp32; \
}

/*===========================================================================
 
FUNCTION    mathfx_Pow2()

DESCRIPTION
  The function Pow2(L_x) is approximated by a table and linear interpolation.

DEPENDENCIES
  Input values assumed normalized to Q16 format

PARAMETERS                                                               
  exponent : 16 bit short signed integer (Word32) whose value falls in the
             range : 0xffff 8000 <= exponent <= 0x0000 7fff.                   
  fraction : 16 bit short signed integer (Word16) whose value falls in the 
             range : 0xffff 8000 <= fraction <= 0x0000 7fff.                   

RETURN VALUE
  L_x : Returns 32 bit long signed integer (Word32) whose value falls in the
        range : 0x8000 0000 <= L_x <= 0x7fff ffff.        

SIDE EFFECTS

===========================================================================*/

Word32 mathfx_Pow2(        /* (o) Q0  : result       (range: 0<=val<=0x7fffffff) */
  Word16 exponent,  /* (i) Q0  : Integer part.      (range: 0<=val<=30)   */
  Word16 fraction   /* (i) Q15 : Fractional part.   (range: 0.0<=val<1.0) */
)

{

    Word16 exp, i, a, tmp;
    Word32 L_x, tmp32;
    // i = bit10-b15 of fraction,   0 <= i <= 31                            
    // a = bit0-b9   of fraction                                            
    // L_x = tabpow[i]<<16 - (tabpow[i] - tabpow[i+1]) * a * 2              
    // L_x = L_x >> (30-exponent)     (with rounding)                       

    L_x = (Word32)fraction * (Word32)32;
    if (L_x != (Word32)0x40000000L) 
    {
        L_x *= 2;
    }
    else 
    {
        L_x = F_MAX;
    }

    i   = (Word16)(L_x >> 16);            // Extract b10-b15 of fraction 
    L_x = mathfx_L_shr(L_x, 1);
    a   = (Word16)L_x;                 // Extract b0-b9   of fraction 
    a   = a & (Word16)0x7fff;

    L_x = ((Word32)mathfx_tabpow[i]) << 16;          // tabpow[i] << 16        
    SUB(mathfx_tabpow[i], mathfx_tabpow[i+1], tmp);      // tabpow[i] - tabpow[i+1] 

#if 1
    L_MSU(L_x,tmp,a,L_x);
#else
    L_x = L_msu(L_x, tmp, a);             // L_x -= tmp*a*2        
#endif

    SUB(30, exponent, exp);

    // L_x = L_shr_r(L_x, exp); 
    if (exp > 31) 
    {
        L_x = 0;
    }
    else 
    {
        tmp32 = L_x;
        L_x   = mathfx_L_shr(L_x,exp);

        if (exp > 0) 
        {
            if ((tmp32 & ( (Word32)1 << (exp-1) )) != 0) 
            {
                L_x++;
            }
        }
    }

    return(L_x);
}

/*===========================================================================
 
FUNCTION    mathfx_Log2() 

DESCRIPTION
  Compute log2(L_x). Note: L_x is positive.  The function Log2(L_x) is 
  approximated by a table and linear interpolation.   

DEPENDENCIES
  Input values assumed normalized to Q16 format

PARAMETERS                                                               
  L_var3 : 32 bit long signed integer (Word32) whose value falls in the
           range : 0x8000 0000 <= L_var3 <= 0x7fff ffff.  
  var1 : 16 bit short signed integer (Word16) whose value falls in the 
         range : 0xffff 8000 <= var1 <= 0x0000 7fff.                   
  var2 : 16 bit short signed integer (Word16) whose value falls in the 
         range : 0xffff 8000 <= var1 <= 0x0000 7fff.                   

RETURN VALUE
  L_var_out : 32 bit long signed integer (Word32) whose value falls in the
              range : 0x8000 0000 <= L_var_out <= 0x7fff ffff.        

SIDE EFFECTS

===========================================================================*/

void mathfx_Log2(
  Word32 L_x,       /* (i) Q0 : input value                                 */
  Word16 *exponent, /* (o) Q0 : Integer part of Log2.   (range: 0<=val<=30) */
  Word16 *fraction  /* (o) Q15: Fractional  part of Log2. (range: 0<=val<1) */
)

{
    Word16 exp, i, a, tmp;
    Word32 L_y;
    Word32 tmp32;
    // Normalization of L_x.
    // exponent = 30-exponent
    // i = bit25-b31 of L_x,    32 <= i <= 63  ->because of normalization.
    // a = bit10-b24
    // i -=32
    // fraction = tablog[i]<<16 - (tablog[i] - tablog[i+1]) * a * 2 

    if( L_x <= (Word32)0 )
    {
        *exponent = 0;
        *fraction = 0;
        return;
    }

    // exp = norm_l(L_x); 
    if (L_x == 0) 
    {
        exp = 0;
    }
    else 
    {
        if (L_x == (Word32)0xffffffffL) 
        {
            exp = 31;
        }
        else 
        {
            if (L_x < 0)
                tmp32 = ~L_x;
            else
                tmp32 = L_x;

            for(exp = 0; tmp32 < (Word32)0x40000000L; exp++) 
            {
                tmp32 <<= 1;
            }
        }
    }

    L_x = mathfx_L_shl(L_x, exp );               // L_x is normalized 

    SUB(30, exp, *exponent);

    L_x = mathfx_L_shr(L_x, 9);
    i   = (Word16)(L_x >> 16);            // Extract b25-b31 
    L_x = mathfx_L_shr(L_x, 1);
    a   = (Word16)(L_x);                  // Extract b10-b24 of fraction 
    a   = a & (Word16)0x7fff;

    SUB(i,32, i);

    L_y = (Word32)mathfx_tablog[i] << 16;        // tablog[i] << 16        
    SUB(mathfx_tablog[i], mathfx_tablog[i+1], tmp);     // tablog[i] - tablog[i+1] 
    #if 1
    L_MSU(L_y,tmp,a,L_y);
    #else
    L_y = L_msu(L_y, tmp, a);             // L_y -= tmp*a*2        
    #endif

    *fraction = (Word16)(L_y >> 16);

    return;
}

/*----------------------------------------------------------------------------*
 * Vector Calculation Functions
 *----------------------------------------------------------------------------*/

//-----------------------------------------------------------------------------
void VectorMul(TVector vector, Fixed scalar, TVector* pResult)
//-----------------------------------------------------------------------------
{
    pResult->x = F_MUL(vector.x, scalar);
    pResult->y = F_MUL(vector.y, scalar);
    pResult->z = F_MUL(vector.z, scalar);
}

//-----------------------------------------------------------------------------
void VectorAdd(TVector vector0, TVector vector1, TVector* pResult)
//-----------------------------------------------------------------------------
{
    pResult->x = F_ADD(vector0.x, vector1.x);
    pResult->y = F_ADD(vector0.y, vector1.y);
    pResult->z = F_ADD(vector0.z, vector1.z);
}

//-----------------------------------------------------------------------------
void VectorSub(TVector vector0, TVector vector1, TVector* pResult)
//-----------------------------------------------------------------------------
{
    pResult->x = F_SUB(vector0.x, vector1.x);
    pResult->y = F_SUB(vector0.y, vector1.y);
    pResult->z = F_SUB(vector0.z, vector1.z);
}

//-----------------------------------------------------------------------------
Fixed VectorDot(TVector vector0, TVector vector1)
//-----------------------------------------------------------------------------
{
    return F_ADD(F_ADD(F_MUL(vector0.x, vector1.x), F_MUL(vector0.y, vector1.y)), F_MUL(vector0.z, vector1.z));
}

//-----------------------------------------------------------------------------
void VectorCross(TVector vector0, TVector vector1, TVector* pResult)
//-----------------------------------------------------------------------------
{
    pResult->x = F_SUB(F_MUL(vector0.y, vector1.z), F_MUL(vector0.z, vector1.y));
    pResult->y = F_SUB(F_MUL(vector0.z, vector1.x), F_MUL(vector0.x, vector1.z));
    pResult->z = F_SUB(F_MUL(vector0.x, vector1.y), F_MUL(vector0.y, vector1.x));
}

//-----------------------------------------------------------------------------
void VectorNormalize(TVector* pVector)
//-----------------------------------------------------------------------------
{
    Fixed n = F_ADD(F_ADD(F_MUL(pVector->x,pVector->x), F_MUL(pVector->y,pVector->y)), F_MUL(pVector->z,pVector->z));

    if (n!=0)
    {
        n = F_DIV(F_ONE, F_SQRT(n));
    }

    pVector->x = F_MUL(pVector->x, n);
    pVector->y = F_MUL(pVector->y, n);
    pVector->z = F_MUL(pVector->z, n);
}
