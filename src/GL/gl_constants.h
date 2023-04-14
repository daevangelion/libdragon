#ifndef __GL_CONSTANTS
#define __GL_CONSTANTS

#define MODELVIEW_STACK_SIZE  32
#define PROJECTION_STACK_SIZE 2
#define TEXTURE_STACK_SIZE    2

#define VERTEX_CACHE_SIZE     32

#define CLIPPING_PLANE_COUNT  6
#define CLIPPING_CACHE_SIZE   9
#define CLIPPING_PLANE_SIZE   8

#define MATRIX_SIZE           64

#define TEX_COORD_COUNT         4
#define TEX_GEN_COUNT           TEX_COORD_COUNT
#define TEX_GEN_PLANE_COUNT     2
#define TEX_GEN_SIZE            34

#define TEX_GEN_STRUCT_SIZE     144
#define TEX_GEN_INTEGER_OFFSET  0
#define TEX_GEN_FRACTION_OFFSET 64
#define TEX_GEN_MODE_OFFSET     128
#define TEX_GEN_CONST_SIZE      (4*2)

#define LIGHT_COUNT           8
#define LIGHT_ATTR_SIZE       8
#define LIGHT_ATTR_ARRAY_SIZE (LIGHT_COUNT*LIGHT_ATTR_SIZE)
#define LIGHT_STRUCT_SIZE     (LIGHT_ATTR_ARRAY_SIZE*5)

#define LIGHT_POSITION_OFFSET           (LIGHT_ATTR_ARRAY_SIZE*0)
#define LIGHT_AMBIENT_OFFSET            (LIGHT_ATTR_ARRAY_SIZE*1)
#define LIGHT_DIFFUSE_OFFSET            (LIGHT_ATTR_ARRAY_SIZE*2)
#define LIGHT_ATTENUATION_INT_OFFSET    (LIGHT_ATTR_ARRAY_SIZE*3)
#define LIGHT_ATTENUATION_FRAC_OFFSET   (LIGHT_ATTR_ARRAY_SIZE*4)

#define MAX_TEXTURE_SIZE      64
#define MAX_TEXTURE_LEVELS    7

#define TEXTURE_IMAGE_SIZE          32
#define TEXTURE_OBJECT_PROPS_OFFSET (TEXTURE_IMAGE_SIZE * MAX_TEXTURE_LEVELS)
#define TEXTURE_OBJECT_SIZE         (TEXTURE_OBJECT_PROPS_OFFSET + 32)
#define TEXTURE_OBJECT_DMA_SIZE     (TEXTURE_OBJECT_SIZE - 16)
#define TEXTURE_OBJECT_SIZE_LOG     8

#define TEXTURE_FLAGS_OFFSET            (TEXTURE_OBJECT_PROPS_OFFSET + 0)
#define TEXTURE_PRIORITY_OFFSET         (TEXTURE_OBJECT_PROPS_OFFSET + 4)
#define TEXTURE_WRAP_S_OFFSET           (TEXTURE_OBJECT_PROPS_OFFSET + 8)
#define TEXTURE_WRAP_T_OFFSET           (TEXTURE_OBJECT_PROPS_OFFSET + 10)
#define TEXTURE_MIN_FILTER_OFFSET       (TEXTURE_OBJECT_PROPS_OFFSET + 12)
#define TEXTURE_MAG_FILTER_OFFSET       (TEXTURE_OBJECT_PROPS_OFFSET + 14)
#define TEXTURE_DIMENSIONALITY_OFFSET   (TEXTURE_OBJECT_PROPS_OFFSET + 16)

#define IMAGE_TEX_IMAGE_OFFSET          0
#define IMAGE_DATA_OFFSET               4
#define IMAGE_SET_LOAD_TILE_OFFSET      8
#define IMAGE_LOAD_BLOCK_OFFSET         12
#define IMAGE_SET_TILE_OFFSET           16
#define IMAGE_WIDTH_OFFSET              20
#define IMAGE_HEIGHT_OFFSET             22
#define IMAGE_STRIDE_OFFSET             24
#define IMAGE_INTERNAL_FORMAT_OFFSET    26
#define IMAGE_TMEM_SIZE_OFFSET          28
#define IMAGE_WIDTH_LOG_OFFSET          30
#define IMAGE_HEIGHT_LOG_OFFSET         31

#define TEXTURE_BILINEAR_MASK       0x001
#define TEXTURE_INTERPOLATE_MASK    0x002
#define TEXTURE_MIPMAP_MASK         0x100

#define MAX_PIXEL_MAP_SIZE    32

#define DELETION_LIST_SIZE   64
#define MAX_DELETION_LISTS   4

#define FLAG_DITHER             (1 << 0)
#define FLAG_BLEND              (1 << 1)
#define FLAG_DEPTH_TEST         (1 << 2)
#define FLAG_DEPTH_MASK         (1 << 3)
#define FLAG_ALPHA_TEST         (1 << 4)
#define FLAG_FOG                (1 << 5)
#define FLAG_MULTISAMPLE        (1 << 6)
#define FLAG_SCISSOR_TEST       (1 << 7)
#define FLAG_TEXTURE_1D         (1 << 8)
#define FLAG_TEXTURE_2D         (1 << 9)
#define FLAG_CULL_FACE          (1 << 10)
#define FLAG_LIGHTING           (1 << 11)
#define FLAG_COLOR_MATERIAL     (1 << 12)
#define FLAG_NORMALIZE          (1 << 13)
#define FLAG_LIGHT0             (1 << 14)
#define FLAG_LIGHT1             (1 << 15)
#define FLAG_LIGHT2             (1 << 16)
#define FLAG_LIGHT3             (1 << 17)
#define FLAG_LIGHT4             (1 << 18)
#define FLAG_LIGHT5             (1 << 19)
#define FLAG_LIGHT6             (1 << 20)
#define FLAG_LIGHT7             (1 << 21)
#define FLAG_TEX_GEN_S          (1 << 22)
#define FLAG_TEX_GEN_T          (1 << 23)
#define FLAG_TEX_GEN_R          (1 << 24)
#define FLAG_TEX_GEN_Q          (1 << 25)
#define FLAG_LIGHT_LOCAL        (1 << 26)
#define FLAG_IMMEDIATE          (1 << 27)
#define FLAG_FINAL_MTX_DIRTY    (1 << 28)
#define FLAG_TEXTURE_ACTIVE     (1 << 29)
#define FLAG_NEED_EYE_SPACE     (1 << 30)

#define TEX_LEVELS_MASK         0x7
#define TEX_FLAG_COMPLETE       (1 << 3)
#define TEX_FLAG_UPLOAD_DIRTY   (1 << 4)

#define DITHER_MASK         (SOM_RGBDITHER_MASK | SOM_ALPHADITHER_MASK)
#define BLEND_MASK          SOM_ZMODE_MASK
#define DEPTH_TEST_MASK     SOM_Z_COMPARE
#define DEPTH_MASK_MASK     SOM_Z_WRITE
#define POINTS_MASK         (SOM_ZSOURCE_MASK | SOM_TEXTURE_PERSP)
#define ALPHA_TEST_MASK     SOM_ALPHACOMPARE_MASK

#define LOAD_TILE 7

#define GUARD_BAND_FACTOR 4

#define ASSERT_INVALID_VTX_ID   0x2001

#define MULTISAMPLE_FLAG_SHIFT      3
#define ZMODE_BLEND_FLAG_SHIFT      10

#define TEX_BILINEAR_SHIFT          13
#define TEX_BILINEAR_OFFSET_SHIFT   4

#define TRICMD_ATTR_SHIFT_Z     6
#define TRICMD_ATTR_SHIFT_TEX   20

#define LIGHT0_SHIFT            14

#define TEX_GEN_S_SHIFT         22

#define NEED_EYE_SPACE_SHIFT    30

#define VTX_LOADER_MAX_COMMANDS 10
#define VTX_LOADER_MAX_SIZE     (VTX_LOADER_MAX_COMMANDS * 4)

#endif
