/*/////////////////////////////////////////////////////////////////////////////
/// @summary Define some functions and types for working with OpenAL buffers 
/// and sources for audio playback. Currently, the exposed functionality is 
/// pretty basic; the intended use is simple 2D sound playback.
/// @author Russell Klenk (contact@russellklenk.com)
///////////////////////////////////////////////////////////////////////////80*/

#ifndef LLOPENCL_HPP_INCLUDED
#define LLOPENCL_HPP_INCLUDED

/*////////////////
//   Includes   //
////////////////*/
#include <stddef.h>

#ifdef __APPLE__
#include <OpenAL/al.h>
#include <OpenAL/alc.h>
#else
#include <AL/al.h>
#include <AL/alc.h>
#endif

/*////////////////////
//   Preprocessor   //
////////////////////*/
/// @summary Abstract away Windows 32-bit calling conventions and visibility.
#if defined(_WIN32) && defined(_MSC_VER)
    #define  LLOPENAL_CALL_C    __cdecl
    #if   defined(_MSC_VER)
    #define  LLOPENAL_IMPORT    __declspec(dllimport)
    #define  LLOPENAL_EXPORT    __declspec(dllexport)
    #elif defined(__GNUC__)
    #define  LLOPENAL_IMPORT    __attribute__((dllimport))
    #define  LLOPENAL_EXPORT    __attribute__((dllexport))
    #else
    #define  LLOPENAL_IMPORT    
    #define  LLOPENAL_EXPORT
    #endif
#else
    #define  LLOPENAL_CALL_C
    #if __GNUC__ >= 4
    #define  LLOPENAL_IMPORT    __attribute__((visibility("default")))
    #define  LLOPENAL_EXPORT    __attribute__((visibility("default")))
    #endif
#endif

/// @summary Define import/export based on whether we're being used as a DLL.
#if defined(LLOPENAL_SHARED)
    #ifdef  LLOPENAL_EXPORTS
    #define LLOPENAL_PUBLIC     LLOPENAL_EXPORT
    #else
    #define LLOPENAL_PUBLIC     LLOPENAL_IMPORT
    #endif
#else
    #define LLOPENAL_PUBLIC
#endif

/*///////////////////////
//   Namespace Begin   //
///////////////////////*/
namespace al {

/*/////////////////
//   Constants   //
/////////////////*/
/// @summary Define the number of channels in a mono sound.
#ifndef LLOPENAL_MONO
#define LLOPENAL_MONO            1U
#endif

/// @summary Define the number of channels in a stereo sound.
#ifndef LLOPENAL_STEREO
#define LLOPENAL_STEREO          2U
#endif

/// @summary Define the maximum number of buffers that may exist simultaneously.
/// This is chosen arbitraily, since OpenAL doesn't define any standard way to
/// retrieve the maximum number of supported buffer objects.
#ifndef LLOPENAL_MAX_BUFFERS
#define LLOPENAL_MAX_BUFFERS     256U
#endif

/// @summary Define the maximum number of sources that may exist simultaneously.
/// This is chosen arbitraily, since OpenAL doesn't define any standard way to
/// retrieve the maximum number of supported source objects. Also note that 
/// the device may not allow simultaneous playback of this many sources.
#ifndef LLOPENAL_MAX_SOURCES
#define LLOPENAL_MAX_SOURCES     32U
#endif

/*/////////////////
//   Data Types  //
/////////////////*/
/// @summary Describes an OpenAL audio device.
struct device_t
{
    ALCdevice    *Device;        /// The audio device handle.
    ALCcontext   *Context;       /// The audio device context.
    char         *DeviceName;    /// The name of the audio device.
    char         *Extensions;    /// Space separated list of extension names.
    ALuint        MaxBuffers;    /// The maximum number of supported buffers.
    ALuint        MaxSources;    /// The maximum number of supported sources.
};

/// @summary Describes a sound buffer containing audio sample data. The audio
/// device supports a limited number of sound buffers.
struct buffer_t
{
    ALuint        Id;            /// The unique identifier of the buffer.
    ALenum        Format;        /// One of AL_FORMAT_MONO16, AL_FORMAT_STEREO16, etc.
    size_t        ChannelCount;  /// The number of channels (1 = mono, 2 = stereo).
    size_t        SampleRate;    /// The sample playback rate, in Hz.
    size_t        BitsPerSample; /// The number of bits in a single sample value.
    size_t        DataSize;      /// The number of bytes allocated to the buffer.
    float         Duration;      /// The number of seconds of data the buffer can store.
};

/// @summary Describes a positional sound source that can be moved within the 
/// scene. The audio device supports a limited number of sources.
struct source_t
{
    ALuint        Id;            /// The unique identifier of the source.
    ALboolean     Streaming;     /// AL_TRUE if this is a streaming source.
    ALboolean     Loop;          /// AL_TRUE to loop the sound.
    float         Gain;          /// The current gain value.
    float         Pitch;         /// The current pitch value.
    float         Position[3];   /// The position of the source within the scene.
    float         Velocity[3];   /// The velocity of the source within the scene.
};

/// @summary Represents a pool of pre-allocated OpenAL buffers.
struct buffer_pool_t
{
    size_t        UsedCount;     /// The number of valid buffers in the UsedIds list.
    size_t        FreeCount;     /// The number of valid buffers in the FreeIds list.
    ALuint       *UsedIds;       /// A list of allocated OpenAL buffer IDs.
    ALuint       *FreeIds;       /// A list of available OpenAL buffer IDs.
    al::buffer_t *Buffers;       /// The pool of buffer descriptors.
    ALuint        BaseId;        /// The smallest valid identifier, or 0.
};

/// @summary Represents a pool of pre-allocated OpenAL sources.
struct source_pool_t
{
    size_t        UsedCount;     /// The number of valid sources in the UsedIds list.
    size_t        FreeCount;     /// The number of valid sources in the FreeIds list.
    ALuint       *UsedIds;       /// A list of allocated OpenAL source IDs.
    ALuint       *FreeIds;       /// A list of available OpenAL source IDs.
    al::source_t *Sources;       /// The pool of buffer descriptors.
    ALuint        BaseId;        /// The smallest valid identifier, or 0.
};

/*////////////////
//   Functions  //
////////////////*/
/// @summary Initializes the audio subsystem, opens the default audio playback 
/// device, and configures the default listener.
/// @param dev The device descriptor to populate.
/// @return true if the default device was opened and is ready for use.
LLOPENAL_PUBLIC bool open_device(al::device_t *dev);

/// @summary Closes the an audio playback device.
/// @param dev The device descriptor to close.
LLOPENAL_PUBLIC void close_device(al::device_t *dev);

/// @summary Initializes the fields of a buffer descriptor.
/// @param buffer The buffer descriptor to initialize.
LLOPENAL_PUBLIC void init_buffer(al::buffer_t *buffer);

/// @summary Creates a new sound buffer. Note that the device supports a 
/// limited number of sound buffers.
/// @param buffer The buffer descriptor to initialize.
/// @param channel_count The number of channels (1 = mono, 2 = stereo.)
/// @param sample_rate The desired sample rate, in Hz.
/// @param bits_per_sample The number of bits used for a single sample value.
/// @return true if the sound buffer was allocated on the device.
LLOPENAL_PUBLIC bool create_buffer(
    al::buffer_t *buffer, 
    size_t        channel_count, 
    size_t        sample_rate, 
    size_t        bits_per_sample);

/// @summary Frees a device sound buffer.
/// @param buffer The buffer descriptor to free.
LLOPENAL_PUBLIC void delete_buffer(al::buffer_t *buffer);

/// @summary Uploads sample data into a sound buffer. The underlying storage 
/// for the sound buffer may be reallocated. The upload operation is performed synchronously.
/// @param buffer The buffer descriptor.
/// @param data The raw sample data. For multi-channel data, the sample values 
/// are stored interleaved; that is, [S0_L][S0_R][S1_L][S1_R]... and so on.
/// @param amount The number of bytes of sample data to upload.
LLOPENAL_PUBLIC void buffer_data(al::buffer_t *buffer, void const *data, size_t amount);

/// @summary Initializes the fields of a source descriptor.
/// @param source The source descriptor to initialize.
LLOPENAL_PUBLIC void init_source(al::source_t *source);

/// @summary Creates a new sound source. Note that the device supports a 
/// limited number of sound sources.
/// @param source The source descriptor to initialize.
/// @return true if the sound source was allocated on the device.
LLOPENAL_PUBLIC bool create_source(al::source_t *source);

/// @summary Deletes a sound source.
/// @param source The source descriptor to free.
LLOPENAL_PUBLIC void delete_source(al::source_t *source);

/// @summary Retrieve the number of buffers queued for playback on a streaming source.
/// @param source The source to query.
/// @return The number of buffers queued for playback on the source.
LLOPENAL_PUBLIC size_t buffers_queued(al::source_t *source);

/// @summary Retrieve the number of buffers processed on a streaming source.
/// @param source The source to query.
/// @return The number of buffers fully processed, but still attached.
LLOPENAL_PUBLIC size_t buffers_processed(al::source_t *source);

/// @summary Attaches a buffer to a source for streaming sound data.
/// @param source The target sound source descriptor.
/// @param buffer The buffer to attach.
/// @return true if the buffer was attached successfully.
LLOPENAL_PUBLIC bool stream_buffer(al::source_t *source, al::buffer_t *buffer);

/// @summary Detaches buffers from a streaming sound source.
/// @param source The target sound source descriptor.
/// @param buffer The buffer to detach.
/// @return true if the buffer was detached.
LLOPENAL_PUBLIC bool detach_buffer(al::source_t *source, al::buffer_t *buffer);

/// @summary Begins or resumes playback of a sound buffer.
/// @param source The sound source within the scene.
LLOPENAL_PUBLIC void play_sound(al::source_t *source);

/// @summary Begins or resumes playback of a sound buffer.
/// @param source The sound source within the scene.
/// @param buffer The sound buffer containing the sample data.
LLOPENAL_PUBLIC void play_sound(al::source_t *source, al::buffer_t *buffer);

/// @summary Stops sound playback for a specific source. The playback cursor 
/// is reset to the beginning of the buffer.
/// @param source The sound source to stop.
LLOPENAL_PUBLIC void stop_sound(al::source_t *source); // alSourceStop

/// @summary Pauses sound playback for a specific source.
/// @param source The sound source to pause.
LLOPENAL_PUBLIC void pause_sound(al::source_t *source); // alSourcePause

/// @summary Initializes a pool of sound buffers.
/// @param pool The buffer pool to initialize.
/// @param capacity The number of sound buffers to preallocate.
/// @param channel_count The number of channels (1 = mono, 2 = stereo.)
/// @param sample_rate The desired sample rate, in Hz.
/// @param bits_per_sample The number of bits used for a single sample value.
/// @return true if the buffer pool and all sound buffers were initialized.
LLOPENAL_PUBLIC bool create_buffer_pool(
    al::buffer_pool_t *pool, 
    size_t             capacity,
    size_t             channel_count, 
    size_t             sample_rate, 
    size_t             bits_per_sample);

/// @summary Frees a pool of sound buffers.
/// @param pool The buffer pool to free.
LLOPENAL_PUBLIC void delete_buffer_pool(al::buffer_pool_t *pool);

/// @summary Returns all sound buffers in a pool to the free list.
/// @param pool The buffer pool to flush.
LLOPENAL_PUBLIC void flush_buffer_pool(al::buffer_pool_t *pool);

/// @summary Attempts to acquire a buffer from the free pool.
/// @param pool The pool to allocate from.
/// @return The buffer descriptor, or NULL if no buffers are available.
LLOPENAL_PUBLIC al::buffer_t* acquire_buffer(al::buffer_pool_t *pool);

/// @summary Locates a buffer descriptor for a given identifier.
/// @param pool The pool to search.
/// @param id The OpenAL unique ID of the buffer to locate.
/// @return The buffer descriptor, or NULL.
LLOPENAL_PUBLIC al::buffer_t* find_buffer(al::buffer_pool_t *pool, ALuint id);

/// @summary Returns a buffer to the free pool.
/// @param pool The pool from which the buffer was allocated.
/// @param id The OpenAL unique ID of the buffer to return to the free pool.
LLOPENAL_PUBLIC void release_buffer(al::buffer_pool_t *pool, ALuint id);

/// @summary Initializes a pool of sound sources.
/// @param pool The source pool to initialize.
/// @param capacity The number of sound sources to preallocate.
/// @return true of the source pool and all sound sources were initialized.
LLOPENAL_PUBLIC bool create_source_pool(al::source_pool_t *pool, size_t capacity);

/// @summary Frees a pool of sound sources. All sounds are stopped.
/// @param pool The source pool to free.
LLOPENAL_PUBLIC void delete_source_pool(al::source_pool_t *pool);

/// @summary Returns all sound sources in a pool to the free list. All sounds are stopped.
/// @param pool The pool to flush.
LLOPENAL_PUBLIC void flush_source_pool(al::source_pool_t *pool);

/// @summary Attempts to acquire a source from the free pool.
/// @param pool The pool to allocate from.
/// @return The source descriptor, or NULL of no sources are available.
LLOPENAL_PUBLIC al::source_t* acquire_source(al::source_pool_t *pool);

/// @summary Returns a source to the free pool. Sound playback is stopped.
/// @param pool The pool from which the source was allocated.
/// @param source The source descriptor to return to the free pool.
LLOPENAL_PUBLIC void release_source(al::source_pool_t *pool, al::source_t *source);

/*/////////////////////
//   Namespace End   //
/////////////////////*/
}; /* end namespace al */

#endif /* !defined(LLOPENAL_HPP_INCLUDED) */

