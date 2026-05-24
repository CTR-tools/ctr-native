#include <common.h>

struct OverlayRDATA_231 R231 = {
    .emSet_Missile =
        {
            [0] =
                {
                    .flags = 1,
                    .initOffset = 0xc,
                    .InitTypes.FuncInit.particle_funcPtr = NULL,
                    .InitTypes.FuncInit.particle_colorFlags = 0,
                    .InitTypes.FuncInit.particle_lifespan = 5,
                    .InitTypes.FuncInit.particle_Type = 1,
                },
            [1] =
                {
                    .flags = 1,
                    .initOffset = 0,
                    .InitTypes.AxisInit.baseValue.startVal = 1,
                },
            [2] =
                {
                    .flags = 1,
                    .initOffset = 1,
                    .InitTypes.AxisInit.baseValue.startVal = 1,
                },
            [3] =
                {
                    .flags = 1,
                    .initOffset = 2,
                    .InitTypes.AxisInit.baseValue.startVal = 1,
                },
            [4] =
                {
                    .flags = 1,
                    .initOffset = 7,
                    .InitTypes.AxisInit.baseValue.startVal = 0x8000,
                },
            [5] =
                {
                    .flags = 1,
                    .initOffset = 8,
                    .InitTypes.AxisInit.baseValue.startVal = 0x8000,
                },
            [6] =
                {
                    .flags = 1,
                    .initOffset = 9,
                    .InitTypes.AxisInit.baseValue.startVal = 0x8000,
                },
            [7] =
                {
                    .flags = 3,
                    .initOffset = 3,
                    .InitTypes.AxisInit.baseValue.startVal = 0x2000,
                    .InitTypes.AxisInit.baseValue.velocity = 0x666,
                },
            [8] =
                {
                    .flags = 3,
                    .initOffset = 4,
                    .InitTypes.AxisInit.baseValue.startVal = 0x1800,
                    .InitTypes.AxisInit.baseValue.velocity = 0x666,
                },
            [9] =
                {
                    .flags = 3,
                    .initOffset = 5,
                    .InitTypes.AxisInit.baseValue.startVal = 0x1000,
                    .InitTypes.AxisInit.baseValue.velocity = 0x6cc,
                },
            [10] = {0},
        },
    .maskPosArr =
        {
            0,  0,  -2, -4, -8,  -12,  -16,  -19,  -23,  -27,  -29,  -31,  -32,  -31,  -29,  -27,  -23,  -19,  -16,  -12,
            -8, -4, -2, 0,  977, 1835, 1792, 2936, 2205, 2095, 2335, 1254, 1884, 1612, 1433, 1971, 1612, 1881, 1792, 1792,
        },
};
