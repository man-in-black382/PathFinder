#ifndef _Constants__
#define _Constants__

static const float Pi = 3.1415926535897932384626433832795;
static const float HalfPi = Pi * 0.5;
static const float QuarterPi = Pi * 0.25;
static const float TwoPi = Pi * 2.0;
static const float PiOver180 = Pi / 180.0;
static const float FloatMax = 3.402823466e+38;
static const uint U8Max = 255u;
static const uint U9Max = 511u;
static const uint U12Max = 4095u;
static const uint U14Max = 16383u;
static const uint U32Max = 4294967295u;

static const uint EntityMaskMeshInstance = 1 << 0;
static const uint EntityMaskLight = 1 << 1;
static const uint EntityMaskAll = 0xFFFFFFFF;

#endif