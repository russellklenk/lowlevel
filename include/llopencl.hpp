/*/////////////////////////////////////////////////////////////////////////////
/// @summary Define some utility classes for working with OpenCL platforms,
/// devices, and performing useful helper functions like loading text files.
/// @author Russell Klenk (contact@russellklenk.com)
///////////////////////////////////////////////////////////////////////////80*/

#ifndef LLOPENCL_HPP_INCLUDED
#define LLOPENCL_HPP_INCLUDED

/*////////////////
//   Includes   //
////////////////*/
#include <CL/cl.h>
#include <stddef.h>

/*////////////////////
//   Preprocessor   //
////////////////////*/
/// @summary Abstract away Windows 32-bit calling conventions.
#if defined(_WIN32) && defined(_MSC_VER)
    #define  LLOPENCL_CALL_C    __cdecl
#else
    #define  LLOPENCL_CALL_C
#endif

/// @summary Abstract away Windows DLL import/export.
#if defined(_MSC_VER)
    #define  LLOPENCL_IMPORT    __declspec(dllimport)
    #define  LLOPENCL_EXPORT    __declspec(dllexport)
#else
    #define  LLOPENCL_IMPORT
    #define  LLOPENCL_EXPORT
#endif

/// @summary Define import/export based on whether we're being used as a DLL.
#if defined(LLOPENCL_SHARED)
    #ifdef  LLOPENCL_EXPORTS
    #define LLOPENCL_PUBLIC     LLOPENCL_EXPORT
    #else
    #define LLOPENCL_PUBLIC     LLOPENCL_IMPORT
    #endif
#else
    #define LLOPENCL_PUBLIC
#endif

/*///////////////////////
//   Namespace Begin   //
///////////////////////*/
namespace cl {

/*/////////////////
//   Data Types  //
/////////////////*/
/// @summary Represents all of the metadata associated with an OpenCL platform.
struct cl_platform_t
{
    cl_platform_id Id;            /// The OpenCL unique platform identifier.
    char          *Name;          /// The name of the platform.
    char          *Vendor;        /// The name of the platform vendor.
    char          *Version;       /// The OpenCL version, "1.2 ..."
    char          *Profile;       /// Either "FULL_PROFILE" or "EMBEDDED_PROFILE".
    char          *Extensions;    /// A space-separated list of extension names.
};

/// @summary Represents all of the metadata associated with an OpenCL device,
/// not including the device capabilities, which are stored and queried separately.
struct cl_device_t
{
    cl_device_id   Id;            /// The OpenCL unique device identifier.
    cl_device_type Type;          /// CL_DEVICE_TYPE_CPU, CL_DEVICE_TYPE_GPU or CL_DEVICE_TYPE_ACCELERATOR.
    cl_platform_id Platform;      /// The OpenCL unique identifier of the platform that owns the device.
    char          *Name;          /// The name of the device.
    char          *Vendor;        /// The name of the platform vendor.
    char          *Version;       /// The version of the device implementation, "1.2 ..."
    char          *DriverVersion; /// The version number of the device driver.
    char          *Extensions;    /// A space-separated list of extension names.
};

/// @summary Stores information about the capabilities of a device.
struct cl_dev_caps_t
{
    cl_bool                     LittleEndian;         /// CL_DEVICE_ENDIAN_LITTLE
    cl_bool                     SupportECC;           /// CL_DEVICE_ERROR_CORRECTION_SUPPORT
    cl_uint                     AddressBits;          /// CL_DEVICE_ADDRESS_BITS
    cl_uint                     AddressAlign;         /// CL_DEVICE_MEM_BASE_ADDR_ALIGN
    cl_uint                     MinTypeAlign;         /// CL_DEVICE_MIN_DATA_TYPE_ALIGN_SIZE
    size_t                      TimerResolution;      /// CL_DEVICE_PROFILING_TIMER_RESOLUTION
    size_t                      MaxWorkGroupSize;     /// CL_DEVICE_MAX_WORK_GROUP_SIZE
    cl_ulong                    MaxMallocSize;        /// CL_DEVICE_MAX_MEM_ALLOC_SIZE
    size_t                      MaxParamSize;         /// CL_DEVICE_MAX_PARAMETER_SIZE
    cl_uint                     MaxConstantArgs;      /// CL_DEVICE_MAX_CONSTANT_ARGS
    cl_ulong                    MaxCBufferSize;       /// CL_DEVICE_MAX_CONSTANT_BUFFER_SIZE
    cl_ulong                    GMemorySize;          /// CL_DEVICE_GLOBAL_MEM_SIZE
    cl_device_mem_cache_type    GCacheType;           /// CL_DEVICE_GLOBAL_MEM_CACHE_TYPE
    cl_ulong                    GCacheSize;           /// CL_DEVICE_GLOBAL_MEM_CACHE_SIZE
    cl_uint                     GCacheLineSize;       /// CL_DEVICE_GLOBAL_MEM_CACHELINE_SIZE
    cl_device_local_mem_type    LMemoryType;          /// CL_DEVICE_LOCAL_MEM_TYPE
    cl_ulong                    LMemorySize;          /// CL_DEVICE_LOCAL_MEM_SIZE
    cl_uint                     ClockFrequency;       /// CL_DEVICE_MAX_CLOCK_FREQUENCY
    cl_uint                     ComputeUnits;         /// CL_DEVICE_MAX_COMPUTE_UNITS
    cl_uint                     VecWidthChar;         /// CL_DEVICE_PREFERRED_VECTOR_WIDTH_CHAR
    cl_uint                     VecWidthShort;        /// CL_DEVICE_PREFERRED_VECTOR_WIDTH_SHORT
    cl_uint                     VecWidthInt;          /// CL_DEVICE_PREFERRED_VECTOR_WIDTH_INT
    cl_uint                     VecWidthLong;         /// CL_DEVICE_PREFERRED_VECTOR_WIDTH_LONG
    cl_uint                     VecWidthSingle;       /// CL_DEVICE_PREFERRED_VECTOR_WIDTH_FLOAT
    cl_uint                     VecWidthDouble;       /// CL_DEVICE_PREFERRED_VECTOR_WIDTH_DOUBLE
    cl_device_fp_config         FPSingleConfig;       /// CL_DEVICE_SINGLE_FP_CONFIG
    cl_device_fp_config         FPDoubleConfig;       /// CL_DEVICE_DOUBLE_FP_CONFIG
    cl_command_queue_properties CmdQueueConfig;       /// CL_DEVICE_QUEUE_PROPERTIES
    cl_bool                     SupportImage;         /// CL_DEVICE_IMAGE_SUPPORT
    size_t                      MaxWidth2D;           /// CL_DEVICE_IMAGE2D_MAX_WIDTH
    size_t                      MaxHeight2D;          /// CL_DEVICE_IMAGE2D_MAX_HEIGHT
    size_t                      MaxWidth3D;           /// CL_DEVICE_IMAGE3D_MAX_WIDTH
    size_t                      MaxHeight3D;          /// CL_DEVICE_IMAGE3D_MAX_HEIGHT
    size_t                      MaxDepth3D;           /// CL_DEVICE_IMAGE3D_MAX_DEPTH
    cl_uint                     MaxSamplers;          /// CL_DEVICE_MAX_SAMPLERS
    cl_uint                     MaxImageSources;      /// CL_DEVICE_MAX_READ_IMAGE_ARGS
    cl_uint                     MaxImageTargets;      /// CL_DEVICE_MAX_WRITE_IMAGE_ARGS
    cl_uint                     MaxWorkItemDimension; /// CL_DEVICE_MAX_WORK_ITEM_DIMENSIONS
    size_t                     *MaxWorkItemSizes;     /// CL_DEVICE_MAX_WORK_ITEM_SIZES
};

/*////////////////
//   Functions  //
////////////////*/
/// @summary Queries the number of OpenCL platforms on the system.
/// @return The number of OpenCL platforms on the system.
cl_uint platform_count(void);

/// @summary Initializes a single cl_platform_t structure.
/// @param platform The platform definition to initialize.
void platform_init(cl_platform_t *platform);

/// @summary Releases resources associated with a cl_platform_t instance.
/// @param platform The platform definition to free.
void platform_free(cl_platform_t *platform);

/// @summary Queries the driver for information about a specific platform.
/// @param platform The platform definition to populate.
/// @param id The unique identifier of the OpenCL platform.
void platform_info(cl_platform_t *platform, cl_platform_id const &id);

/// @summary Determines whether a platform supports a given extension.
/// @param platform The platform to query.
/// @param extension The NULL-terminated extension name.
/// @return true if the platform supports the specified extension.
bool platform_support(cl_platform_t *platform, char const *extension);

/// @summary Queries the number of devices of a given type on a platform.
/// @param platform The OpenCL unique identifier of the platform to query.
/// @param of_type A combination of cl_device_type indicating the device types to count.
/// @return The number of devices matching the specified criteria.
cl_uint device_count(cl_platform_id const &platform, cl_device_type of_type);

/// @summary Initializes a single cl_device_t structure.
/// @param dev The device definition to initialize.
void device_init(cl_device_t *dev);

/// @summary Releases resources associated with a cl_device_t instance.
/// @param dev The device definition to free.
void device_free(cl_device_t *dev);

/// @summary Queries the driver for information about a specific device.
/// @param dev The device definition to populate.
/// @param platform The OpenCL unique identifier of the platform defining the device.
/// @param id The OpenCL unique identifer of the device.
void device_info(cl_device_t *dev, cl_platform_id const &platform, cl_device_id const &id);

/// @summary Determines whether a device supports a given extension.
/// @param dev The device to query.
/// @param extension The NULL-terminated extension name.
/// @return true if the device supports the specified extension.
bool device_support(cl_device_t *dev, char const *extension);

/// @summary Initializes a single cl_dev_caps_t structure.
/// @param caps The device capabilities to initialize.
void dev_caps_init(cl_dev_caps_t *caps);

/// @summary Releases resources associated with a cl_dev_caps_t instance.
/// @param caps The device capabilities to free.
void dev_caps_free(cl_dev_caps_t *caps);

/// @summary Query the capabilities for a device.
/// @param caps The device capabilities structure to populate.
/// @param device The OpenCL unique identifier of the device.
void dev_caps_info(cl_dev_caps_t *caps, cl_device_id const &device);

/*/////////////////////
//   Namespace End   //
/////////////////////*/
}; /* end namespace cl */

#endif /* !defined(LLOPENCL_HPP_INCLUDED) */
