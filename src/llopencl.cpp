/*/////////////////////////////////////////////////////////////////////////////
/// @summary Define some utility classes for working with OpenCL platforms,
/// devices, and performing useful helper functions like loading text files.
/// @author Russell Klenk (contact@russellklenk.com)
///////////////////////////////////////////////////////////////////////////80*/

/*////////////////
//   Includes   //
////////////////*/
#include <string.h>
#include <stdlib.h>
#include "llopencl.hpp"

/*/////////////////
//   Constants   //
/////////////////*/
static const cl_uint MAX_PLATFORM_COUNT = (cl_uint) -1;

/*///////////////////////
//   Local Functions   //
///////////////////////*/
/// @summary Wraps the sequence of calls necessary to retrieve a string value
/// using the clGetPlatformInfo API. The string storage is allocated using the
/// standard C library function malloc(), and should be freed using free().
/// The returned string is guaranteed to be NULL-terminated.
/// @param id The OpenCL unique identifier of the platform to query.
/// @param param The parameter value being queried.
/// @return A NULL-terminated string containing the parameter value.
static char* cl_platform_str(cl_platform_id const &id, cl_platform_info param)
{
    size_t nbytes = 0;
    char  *buffer = NULL;
    clGetPlatformInfo(id, param, 0, NULL, &nbytes);
    buffer = (char*) malloc(sizeof(char) * nbytes);
    clGetPlatformInfo(id, param, nbytes, buffer, NULL);
    buffer[nbytes-1] = '\0';
    return buffer;
}

/// @summary Wraps the sequence of calls necessary to retrieve a string value
/// using the clGetDeviceInfo API. The string storage is allocated using the
/// standard C library function malloc(), and should be freed using free().
/// The returned string is guaranteed to be NULL-terminated.
/// @param id The OpenCL unique identifier of the device to query.
/// @param param The parameter value being queried.
/// @return A NULL-terminated string containing the parameter value.
static char* cl_device_str(cl_device_id const &id, cl_device_info param)
{
    size_t nbytes = 0;
    char  *buffer = NULL;
    clGetDeviceInfo(id, param, 0, NULL, &nbytes);
    buffer = (char*) malloc(sizeof(char) * nbytes);
    clGetDeviceInfo(id, param, nbytes, buffer, NULL);
    buffer[nbytes-1] = '\0';
    return buffer;
}

/*////////////////////////
//   Public Functions   //
////////////////////////*/
cl_uint cl::platform_count(void)
{
    cl_uint n = 0;
    clGetPlatformIDs(MAX_PLATFORM_COUNT, NULL, &n);
    return n;
}

void cl::platform_init(cl::platform_t *platform)
{
    if (platform)
    {
        platform->Id         = (cl_platform_id) 0;
        platform->Name       = NULL;
        platform->Vendor     = NULL;
        platform->Version    = NULL;
        platform->Profile    = NULL;
        platform->Extensions = NULL;
    }
}

void cl::platform_free(cl::platform_t *platform)
{
    if (platform)
    {
        if (platform->Extensions) free(platform->Extensions);
        if (platform->Profile)    free(platform->Profile);
        if (platform->Version)    free(platform->Version);
        if (platform->Vendor)     free(platform->Vendor);
        if (platform->Name)       free(platform->Name);
        platform->Id              = (cl_platform_id) 0;
        platform->Name            = NULL;
        platform->Vendor          = NULL;
        platform->Version         = NULL;
        platform->Profile         = NULL;
        platform->Extensions      = NULL;
    }
}

void cl::platform_info(cl::platform_t *platform, cl_platform_id const &id)
{
    platform->Id         = id;
    platform->Name       = cl_platform_str(id, CL_PLATFORM_NAME);
    platform->Vendor     = cl_platform_str(id, CL_PLATFORM_VENDOR);
    platform->Version    = cl_platform_str(id, CL_PLATFORM_VERSION);
    platform->Profile    = cl_platform_str(id, CL_PLATFORM_PROFILE);
    platform->Extensions = cl_platform_str(id, CL_PLATFORM_EXTENSIONS);
}

bool cl::platform_support(cl::platform_t *platform, char const *extension)
{
    return (strstr(platform->Extensions, extension) != NULL);
}

cl_uint cl::device_count(cl_platform_id const &platform, cl_device_type of_type)
{
    cl_uint ndevs = 0;
    clGetDeviceIDs(platform, of_type, 0, NULL, &ndevs);
    return ndevs;
}

void cl::device_init(cl::device_t *dev)
{
    if (dev)
    {
        dev->Id            = (cl_device_id) 0;
        dev->Type          = CL_DEVICE_TYPE_DEFAULT;
        dev->Platform      = (cl_platform_id) 0;
        dev->Name          = NULL;
        dev->Vendor        = NULL;
        dev->Version       = NULL;
        dev->DriverVersion = NULL;
        dev->Extensions    = NULL;
    }
}

void cl::device_free(cl::device_t *dev)
{
    if (dev)
    {
        if (dev->Extensions)    free(dev->Extensions);
        if (dev->DriverVersion) free(dev->DriverVersion);
        if (dev->Version)       free(dev->Version);
        if (dev->Vendor)        free(dev->Vendor);
        if (dev->Name)          free(dev->Name);
        dev->Id                 = (cl_device_id) 0;
        dev->Type               = CL_DEVICE_TYPE_DEFAULT;
        dev->Platform           = (cl_platform_id) 0;
        dev->Name               = NULL;
        dev->Vendor             = NULL;
        dev->Version            = NULL;
        dev->DriverVersion      = NULL;
        dev->Extensions         = NULL;
    }
}

void cl::device_info(cl::device_t *dev, cl_platform_id const &platform, cl_device_id const &id)
{
    dev->Id            = id;
    dev->Platform      = platform;
    dev->Name          = cl_device_str(id, CL_DEVICE_NAME);
    dev->Vendor        = cl_device_str(id, CL_DEVICE_VENDOR);
    dev->Version       = cl_device_str(id, CL_DEVICE_VERSION);
    dev->DriverVersion = cl_device_str(id, CL_DRIVER_VERSION);
    dev->Extensions    = cl_device_str(id, CL_DEVICE_EXTENSIONS);
    clGetDeviceInfo(id, CL_DEVICE_TYPE, sizeof(dev->Type), &dev->Type, NULL);
}

bool cl::device_support(cl::device_t *dev, char const *extension)
{
    return (strstr(dev->Extensions, extension) != NULL);
}

void cl::device_caps_init(cl::device_caps_t *caps)
{
    if(caps) memset(caps, 0, sizeof(cl::device_caps_t));
}

void cl::device_caps_free(cl::device_caps_t *caps)
{
    if (caps)
    {
        if (caps->MaxWorkItemSizes) free(caps->MaxWorkItemSizes);
        caps->MaxWorkItemDimension  = 0;
        caps->MaxWorkItemSizes      = NULL;
    }
}

void cl::device_caps_info(cl::device_caps_t *caps, cl_device_id const &device)
{
    cl_uint mid = 0;
    clGetDeviceInfo(device, CL_DEVICE_MAX_WORK_ITEM_DIMENSIONS, sizeof(mid), &mid, NULL);
    caps->MaxWorkItemDimension = mid;
    caps->MaxWorkItemSizes = (size_t*) malloc(mid * sizeof(size_t));
    clGetDeviceInfo(device, CL_DEVICE_MAX_WORK_ITEM_SIZES, mid * sizeof(size_t),  caps->MaxWorkItemSizes, NULL);

    clGetDeviceInfo(device, CL_DEVICE_ENDIAN_LITTLE,                 sizeof(caps->LittleEndian),     &caps->LittleEndian,     NULL);
    clGetDeviceInfo(device, CL_DEVICE_ERROR_CORRECTION_SUPPORT,      sizeof(caps->SupportECC),       &caps->SupportECC,       NULL);
    clGetDeviceInfo(device, CL_DEVICE_ADDRESS_BITS,                  sizeof(caps->AddressBits),      &caps->AddressBits,      NULL);
    clGetDeviceInfo(device, CL_DEVICE_MEM_BASE_ADDR_ALIGN,           sizeof(caps->AddressAlign),     &caps->AddressAlign,     NULL);
    clGetDeviceInfo(device, CL_DEVICE_MIN_DATA_TYPE_ALIGN_SIZE,      sizeof(caps->MinTypeAlign),     &caps->MinTypeAlign,     NULL);
    clGetDeviceInfo(device, CL_DEVICE_PROFILING_TIMER_RESOLUTION,    sizeof(caps->TimerResolution),  &caps->TimerResolution,  NULL);
    clGetDeviceInfo(device, CL_DEVICE_MAX_WORK_GROUP_SIZE,           sizeof(caps->MaxWorkGroupSize), &caps->MaxWorkGroupSize, NULL);
    clGetDeviceInfo(device, CL_DEVICE_MAX_MEM_ALLOC_SIZE,            sizeof(caps->MaxMallocSize),    &caps->MaxMallocSize,    NULL);
    clGetDeviceInfo(device, CL_DEVICE_MAX_PARAMETER_SIZE,            sizeof(caps->MaxParamSize),     &caps->MaxParamSize,     NULL);
    clGetDeviceInfo(device, CL_DEVICE_MAX_CONSTANT_ARGS,             sizeof(caps->MaxConstantArgs),  &caps->MaxConstantArgs,  NULL);
    clGetDeviceInfo(device, CL_DEVICE_MAX_CONSTANT_BUFFER_SIZE,      sizeof(caps->MaxCBufferSize),   &caps->MaxCBufferSize,   NULL);
    clGetDeviceInfo(device, CL_DEVICE_GLOBAL_MEM_SIZE,               sizeof(caps->GMemorySize),      &caps->GMemorySize,      NULL);
    clGetDeviceInfo(device, CL_DEVICE_GLOBAL_MEM_CACHE_TYPE,         sizeof(caps->GCacheType),       &caps->GCacheType,       NULL);
    clGetDeviceInfo(device, CL_DEVICE_GLOBAL_MEM_CACHE_SIZE,         sizeof(caps->GCacheSize),       &caps->GCacheSize,       NULL);
    clGetDeviceInfo(device, CL_DEVICE_GLOBAL_MEM_CACHELINE_SIZE,     sizeof(caps->GCacheLineSize),   &caps->GCacheLineSize,   NULL);
    clGetDeviceInfo(device, CL_DEVICE_LOCAL_MEM_TYPE,                sizeof(caps->LMemoryType),      &caps->LMemoryType,      NULL);
    clGetDeviceInfo(device, CL_DEVICE_LOCAL_MEM_SIZE,                sizeof(caps->LMemorySize),      &caps->LMemorySize,      NULL);
    clGetDeviceInfo(device, CL_DEVICE_MAX_CLOCK_FREQUENCY,           sizeof(caps->ClockFrequency),   &caps->ClockFrequency,   NULL);
    clGetDeviceInfo(device, CL_DEVICE_MAX_COMPUTE_UNITS,             sizeof(caps->ComputeUnits),     &caps->ComputeUnits,     NULL);
    clGetDeviceInfo(device, CL_DEVICE_PREFERRED_VECTOR_WIDTH_CHAR,   sizeof(caps->VecWidthChar),     &caps->VecWidthChar,     NULL);
    clGetDeviceInfo(device, CL_DEVICE_PREFERRED_VECTOR_WIDTH_SHORT,  sizeof(caps->VecWidthShort),    &caps->VecWidthShort,    NULL);
    clGetDeviceInfo(device, CL_DEVICE_PREFERRED_VECTOR_WIDTH_INT,    sizeof(caps->VecWidthInt),      &caps->VecWidthInt,      NULL);
    clGetDeviceInfo(device, CL_DEVICE_PREFERRED_VECTOR_WIDTH_LONG,   sizeof(caps->VecWidthLong),     &caps->VecWidthLong,     NULL);
    clGetDeviceInfo(device, CL_DEVICE_PREFERRED_VECTOR_WIDTH_FLOAT,  sizeof(caps->VecWidthSingle),   &caps->VecWidthSingle,   NULL);
    clGetDeviceInfo(device, CL_DEVICE_PREFERRED_VECTOR_WIDTH_DOUBLE, sizeof(caps->VecWidthDouble),   &caps->VecWidthDouble,   NULL);
    clGetDeviceInfo(device, CL_DEVICE_SINGLE_FP_CONFIG,              sizeof(caps->FPSingleConfig),   &caps->FPSingleConfig,   NULL);
    //clGetDeviceInfo(device, CL_DEVICE_DOUBLE_FP_CONFIG,              sizeof(caps->FPDoubleConfig),   &caps->FPDoubleConfig,   NULL);
    clGetDeviceInfo(device, CL_DEVICE_QUEUE_PROPERTIES,              sizeof(caps->CmdQueueConfig),   &caps->CmdQueueConfig,   NULL);
    clGetDeviceInfo(device, CL_DEVICE_IMAGE_SUPPORT,                 sizeof(caps->SupportImage),     &caps->SupportImage,     NULL);
    clGetDeviceInfo(device, CL_DEVICE_IMAGE2D_MAX_WIDTH,             sizeof(caps->MaxWidth2D),       &caps->MaxWidth2D,       NULL);
    clGetDeviceInfo(device, CL_DEVICE_IMAGE2D_MAX_HEIGHT,            sizeof(caps->MaxHeight2D),      &caps->MaxHeight2D,      NULL);
    clGetDeviceInfo(device, CL_DEVICE_IMAGE3D_MAX_WIDTH,             sizeof(caps->MaxWidth3D),       &caps->MaxWidth3D,       NULL);
    clGetDeviceInfo(device, CL_DEVICE_IMAGE3D_MAX_HEIGHT,            sizeof(caps->MaxHeight3D),      &caps->MaxHeight3D,      NULL);
    clGetDeviceInfo(device, CL_DEVICE_IMAGE3D_MAX_DEPTH,             sizeof(caps->MaxDepth3D),       &caps->MaxDepth3D,       NULL);
    clGetDeviceInfo(device, CL_DEVICE_MAX_SAMPLERS,                  sizeof(caps->MaxSamplers),      &caps->MaxSamplers,      NULL);
    clGetDeviceInfo(device, CL_DEVICE_MAX_READ_IMAGE_ARGS,           sizeof(caps->MaxImageSources),  &caps->MaxImageSources,  NULL);
    clGetDeviceInfo(device, CL_DEVICE_MAX_WRITE_IMAGE_ARGS,          sizeof(caps->MaxImageTargets),  &caps->MaxImageTargets,  NULL);
}

