/*/////////////////////////////////////////////////////////////////////////////
/// @summary Implements functions for working with OpenAL buffers and sources 
/// for audio playback. Currently, the exposed functionality is pretty basic.
/// @author Russell Klenk (contact@russellklenk.com)
///////////////////////////////////////////////////////////////////////////80*/

/*////////////////
//   Includes   //
////////////////*/
#include <math.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include "llopenal.hpp"

/*/////////////////
//   Constants   //
/////////////////*/

/*///////////////////////
//   Local Functions   //
///////////////////////*/
/// @summary Wraps the sequence of calls necessary to retrieve a string value
/// using the alcGetString API. The string storage is allocated using the
/// standard C library function malloc(), and should be freed using free().
/// The returned string is guaranteed to be NULL-terminated.
/// @param dev The OpenAL device object to query, or NULL for the default device. 
/// @param param The parameter value being queried.
/// @return A NULL-terminated string containing the parameter value.
static char* al_device_str(ALCdevice *dev, ALenum param)
{
    ALCchar const* value = alcGetString(dev, param);
    if (value != NULL)
    {
        // return a copy of the driver string.
        return strdup(value);
    }
    else
    {
        char *str = (char*) malloc(1);
        str[0]    = '\0';
        return str;
    }
}

/// @summary Converts a channel count and bits-per-sample pair into the 
/// corresponding OpenAL format value.
/// @param channel_count The number of channels of sample data, either 1 or 2.
/// @param bits_per_sample The number of bits-per-sample, either 8 or 16.
/// @return The corresponding OpenAL format enumeration value, or -1.
static ALenum al_format(size_t channel_count, size_t bits_per_sample)
{
    switch (bits_per_sample)
    {
        case 16:
            return channel_count > 1 ? AL_FORMAT_STEREO16 : AL_FORMAT_MONO16;

        case 8:
            return channel_count > 1 ? AL_FORMAT_STEREO8  : AL_FORMAT_MONO8;

        default:
            break;
    }
    return (ALenum) -1;
}

/*////////////////////////
//   Public Functions   //
////////////////////////*/
bool al::open_device(al::device_t *dev)
{
    if (dev)
    {
        ALCdevice *device = alcOpenDevice(NULL);
        if (device == NULL) return false;

        ALCcontext *context = alcCreateContext(device, NULL);
        if (context == NULL)
        {
            alcCloseDevice(device);
            return false;
        }
        alcMakeContextCurrent(context);
        alGetError();

        ALfloat orientation[] = {
            0.0f, 0.0f, -1.0f, /* at vector */
            0.0f, 1.0f,  0.0f  /* up vector */
        };
        alListener3f(AL_POSITION, 0.0f, 0.0f, 1.0f);
        alListener3f(AL_VELOCITY, 0.0f, 0.0f, 0.0f);
        alListenerfv(AL_ORIENTATION, orientation);

        dev->Device     = device;
        dev->Context    = context;
        dev->DeviceName = al_device_str(device, ALC_DEVICE_SPECIFIER);
        dev->Extensions = al_device_str(device, ALC_EXTENSIONS);
        dev->MaxBuffers = LLOPENAL_MAX_BUFFERS;
        dev->MaxSources = LLOPENAL_MAX_SOURCES;
        return true;
    }
    else return false;
}

void al::close_device(al::device_t *dev)
{
    if (dev)
    {
        if (dev->Context)
        {
            alcMakeContextCurrent(NULL);
            alcDestroyContext(dev->Context);
        }
        if (dev->Device)
        {
            alcCloseDevice(dev->Device);
        }
        if (dev->Extensions) free(dev->Extensions);
        if (dev->DeviceName) free(dev->DeviceName);
        dev->Device          = NULL;
        dev->Context         = NULL;
        dev->DeviceName      = NULL;
        dev->Extensions      = NULL;
    }
}

void al::init_buffer(al::buffer_t *buffer)
{
    buffer->Id            = 0;
    buffer->Format        = AL_INVALID_ENUM;
    buffer->ChannelCount  = 0;
    buffer->SampleRate    = 1;
    buffer->BitsPerSample = 1;
    buffer->DataSize      = 0;
    buffer->Duration      = 0.0f;
}

bool al::create_buffer(al::buffer_t *buffer, size_t channel_count, size_t sample_rate, size_t bits_per_sample)
{
    if (buffer)
    {
        ALuint id  = 0;
        alGenBuffers(1, &id);
        if (id == 0) return false;

        buffer->Id            = id;
        buffer->Format        = al_format(channel_count, bits_per_sample);
        buffer->ChannelCount  = channel_count;
        buffer->SampleRate    = sample_rate;
        buffer->BitsPerSample = bits_per_sample;
        buffer->DataSize      = 0;
        buffer->Duration      = 0.0f;
        return true;
    }
    else return false;
}

void al::delete_buffer(al::buffer_t *buffer)
{
    if (buffer && buffer->Id)
    {
        alDeleteBuffers(1, &buffer->Id);
        buffer->Id       = 0;
        buffer->DataSize = 0;
        buffer->Duration = 0.0f;
    }
}

void al::buffer_data(al::buffer_t *buffer, void const *data, size_t amount)
{
    size_t bps = buffer->BitsPerSample * buffer->SampleRate * buffer->ChannelCount;
    float  len = float(amount) / float(bps / 8);
    alBufferData(buffer->Id, buffer->Format, data, amount, buffer->SampleRate);
    buffer->DataSize = amount;
    buffer->Duration = len;
}

void al::init_source(al::source_t *source)
{
    source->Id          = 0;
    source->Streaming   = AL_FALSE;
    source->Loop        = AL_FALSE;
    source->Gain        = 1.0f;
    source->Pitch       = 1.0f;
    source->Position[0] = 0.0f;
    source->Position[1] = 0.0f;
    source->Position[2] = 0.0f;
    source->Velocity[0] = 0.0f;
    source->Velocity[1] = 0.0f;
    source->Velocity[2] = 0.0f;
}

bool al::create_source(al::source_t *source)
{
    if (source)
    {
        ALuint id  = 0;
        alGenSources(1, &id);
        if (id == 0) return false;

        source->Id          = id;
        source->Streaming   = AL_FALSE;
        source->Loop        = AL_FALSE;
        source->Gain        = 1.0f;
        source->Pitch       = 1.0f;
        source->Position[0] = 0.0f;
        source->Position[1] = 0.0f;
        source->Position[2] = 0.0f;
        source->Velocity[0] = 0.0f;
        source->Velocity[1] = 0.0f;
        source->Velocity[2] = 0.0f;
        return true;
    }
    else return false;
}

void al::delete_source(al::source_t *source)
{
    if (source && source->Id)
    {
        alSourceStop(source->Id);
        alSourcei(source->Id, AL_BUFFER, 0);
        alDeleteSources(1, &source->Id);
        source->Id = 0;
    }
}

size_t al::buffers_queued(al::source_t *source)
{
    ALint value = 0;
    alGetSourcei(source->Id, AL_BUFFERS_QUEUED, &value);
    return size_t(value);
}

size_t al::buffers_processed(al::source_t *source)
{
    ALint value = 0;
    alGetSourcei(source->Id, AL_BUFFERS_PROCESSED, &value);
    return size_t(value);
}

bool al::stream_buffer(al::source_t *source, al::buffer_t *buffer)
{
    source->Streaming = AL_TRUE;
    alSourceQueueBuffers(source->Id, 1, &buffer->Id);
    return true;
}

bool al::detach_buffer(al::source_t *source, al::buffer_t *buffer)
{
    alSourceUnqueueBuffers(source->Id, 1, &buffer->Id);
    return true;
}

void al::play_sound(al::source_t *source)
{
    al::play_sound(source, NULL);
}

void al::play_sound(al::source_t *source, al::buffer_t *buffer)
{
    alSourcef (source->Id, AL_GAIN    , source->Gain);
    alSourcef (source->Id, AL_PITCH   , source->Pitch);
    alSourcei (source->Id, AL_LOOPING , source->Loop);
    alSourcefv(source->Id, AL_POSITION, source->Position);
    alSourcefv(source->Id, AL_VELOCITY, source->Velocity);
    if (buffer != NULL)
    {
        alSourcei (source->Id, AL_BUFFER  , buffer->Id);
    }
    alSourcePlay(source->Id);
}

void al::stop_sound(al::source_t *source)
{
    alSourceStop(source->Id);
}

void al::pause_sound(al::source_t *source)
{
    alSourcePause(source->Id);
}

bool al::create_buffer_pool(al::buffer_pool_t *pool, size_t capacity, size_t channel_count, size_t sample_rate, size_t bits_per_sample)
{
    if (pool)
    {
        pool->UsedCount = 0;
        pool->FreeCount = 0;
        pool->UsedIds   = NULL;
        pool->FreeIds   = NULL;
        pool->Buffers   = NULL;
        pool->BaseId    = 0;
        if (capacity > 0)
        {
            pool->UsedIds = (ALuint*)       malloc(capacity * sizeof(ALuint));
            pool->FreeIds = (ALuint*)       malloc(capacity * sizeof(ALuint));
            pool->Buffers = (al::buffer_t*) malloc(capacity * sizeof(al::buffer_t));

            alGenBuffers(capacity, pool->FreeIds);
            for (size_t i = 0; i < capacity; ++i)
            {
                al::buffer_t &bo = pool->Buffers[i];
                bo.Id            = pool->FreeIds[i];
                bo.Format        = al_format(channel_count, bits_per_sample);
                bo.ChannelCount  = channel_count;
                bo.SampleRate    = sample_rate;
                bo.BitsPerSample = bits_per_sample;
                bo.DataSize      = 0;
                bo.Duration      = 0.0f;
            }
            // save the base ID. since IDs are contiguous, we can use 
            // this ID to quickly index into Buffers for a given ID.
            pool->FreeCount = capacity;
            pool->BaseId    = pool->FreeIds[0];

            // @SANITY CHECK@
            // we make an assumption that IDs are contiguous.
            // the OpenAL spec does *not* guarantee this behavior.
            // it is however likely that this is the case when 
            // pools are being allocated - we typically don't free
            // audio buffers until the application is shut down.
            for (size_t i = 0; i < capacity - 1; ++i)
            {
                if (pool->FreeIds[i+1] != (pool->FreeIds[i] + 1))
                {
                    fprintf(stderr, "Assertion failed: OpenAL buffer IDs are not contiguous.\n");
                    abort();
                }
            }
            // @SANITY CHECK@
        }
        return true;
    }
    else return false;
}

void al::delete_buffer_pool(al::buffer_pool_t *pool)
{
    if (pool)
    {
        if (pool->UsedCount > 0) alDeleteBuffers(pool->UsedCount, pool->UsedIds);
        if (pool->FreeCount > 0) alDeleteBuffers(pool->FreeCount, pool->FreeIds);
        if (pool->Buffers)       free(pool->Buffers);
        if (pool->FreeIds)       free(pool->FreeIds);
        if (pool->UsedIds)       free(pool->UsedIds);
        pool->UsedCount          = 0;
        pool->FreeCount          = 0;
        pool->UsedIds            = NULL;
        pool->FreeIds            = NULL;
        pool->Buffers            = NULL;
        pool->BaseId             = 0;
    }
}

void al::flush_buffer_pool(al::buffer_pool_t *pool)
{
    for (size_t i = 0; i < pool->UsedCount; ++i)
    {
        pool->FreeIds[pool->FreeCount++] = pool->UsedIds[i];
    }
    pool->UsedCount = 0;
}

al::buffer_t* al::acquire_buffer(al::buffer_pool_t *pool)
{
    if (pool->FreeCount > 0)
    {
        size_t nu = pool->UsedCount + 1;
        size_t nf = pool->FreeCount - 1;
        ALuint id = pool->FreeIds[nf];
        pool->UsedIds[pool->UsedCount] = id;
        pool->UsedCount = nu;
        pool->FreeCount = nf;
        return &pool->Buffers[id - pool->BaseId];
    }
    return NULL;
}

al::buffer_t* al::find_buffer(al::buffer_pool_t *pool, ALuint id)
{
    for (size_t i = 0; i < pool->UsedCount; ++i)
    {
        if (pool->UsedIds[i] == id)
        {
            return &pool->Buffers[id - pool->BaseId];
        }
    }
    return NULL;
}

void al::release_buffer(al::buffer_pool_t *pool, ALuint id)
{
    for (size_t i = 0; i < pool->UsedCount; ++i)
    {
        if (pool->UsedIds[i] == id)
        {
            pool->UsedCount--;
            pool->UsedIds[i]  = pool->UsedIds[pool->UsedCount];
            pool->FreeIds[pool->FreeCount++] = id;
            break;
        }
    }
}

bool al::create_source_pool(al::source_pool_t *pool, size_t capacity)
{
    if (pool)
    {
        pool->UsedCount = 0;
        pool->FreeCount = 0;
        pool->UsedIds   = NULL;
        pool->FreeIds   = NULL;
        pool->Sources   = NULL;
        pool->BaseId    = 0;
        if (capacity > 0)
        {
            pool->UsedIds = (ALuint*)       malloc(capacity * sizeof(ALuint));
            pool->FreeIds = (ALuint*)       malloc(capacity * sizeof(ALuint));
            pool->Sources = (al::source_t*) malloc(capacity * sizeof(al::source_t));

            alGenSources(capacity, pool->FreeIds);
            for (size_t i = 0; i < capacity; ++i)
            {
                al::source_t &so = pool->Sources[i];
                so.Id            = pool->FreeIds[i];
                so.Streaming     = AL_FALSE;
                so.Loop          = AL_FALSE;
                so.Gain          = 1.0f;
                so.Pitch         = 1.0f;
                so.Position[0]   = 0.0f;
                so.Position[1]   = 0.0f;
                so.Position[2]   = 0.0f;
                so.Velocity[0]   = 0.0f;
                so.Velocity[1]   = 0.0f;
                so.Velocity[2]   = 0.0f;
            }
            // save the base ID. since IDs are contiguous, we can use 
            // this ID to quickly index into Sources for a given ID.
            pool->FreeCount = capacity;
            pool->BaseId    = pool->FreeIds[0];

            // @SANITY CHECK@
            // we make an assumption that IDs are contiguous.
            // the OpenAL spec does *not* guarantee this behavior.
            // it is however likely that this is the case when 
            // pools are being allocated - we typically don't free
            // audio sources until the application is shut down.
            for (size_t i = 0; i < capacity - 1; ++i)
            {
                if (pool->FreeIds[i+1] != (pool->FreeIds[i] + 1))
                {
                    fprintf(stderr, "Assertion failed: OpenAL source IDs are not contiguous.\n");
                    abort();
                }
            }
            // @SANITY CHECK@
        }
        return true;
    }
    else return false;
}

void al::delete_source_pool(al::source_pool_t *pool)
{
    if (pool)
    {
        if (pool->UsedCount > 0) 
        {
            for (size_t i = 0; i < pool->UsedCount; ++i)
            {
                alSourceStop(pool->UsedIds[i]);
                alSourcei(pool->UsedIds[i], AL_BUFFER, 0);
            }
            alDeleteSources(pool->UsedCount, pool->UsedIds);
        }
        if (pool->FreeCount > 0) alDeleteBuffers(pool->FreeCount, pool->FreeIds);
        if (pool->Sources)       free(pool->Sources);
        if (pool->FreeIds)       free(pool->FreeIds);
        if (pool->UsedIds)       free(pool->UsedIds);
        pool->UsedCount          = 0;
        pool->FreeCount          = 0;
        pool->UsedIds            = NULL;
        pool->FreeIds            = NULL;
        pool->Sources            = NULL;
        pool->BaseId             = 0;
    }
}

void al::flush_source_pool(al::source_pool_t *pool)
{
    for (size_t i = 0; i < pool->UsedCount; ++i)
    {
        alSourceStop(pool->UsedIds[i]);
        alSourcei(pool->UsedIds[i], AL_BUFFER, 0);
        pool->FreeIds[pool->FreeCount++] = pool->UsedIds[i];
    }
    pool->UsedCount = 0;
}

al::source_t* al::acquire_source(al::source_pool_t *pool)
{
    if (pool->FreeCount > 0)
    {
        size_t nu = pool->UsedCount + 1;
        size_t nf = pool->FreeCount - 1;
        ALuint id = pool->FreeIds[nf];
        pool->UsedIds[pool->UsedCount] = id;
        pool->UsedCount = nu;
        pool->FreeCount = nf;

        al::source_t *source = &pool->Sources[id - pool->BaseId];
        source->Streaming    = AL_FALSE;
        source->Loop         = AL_FALSE;
        source->Gain         = 1.0f;
        source->Pitch        = 1.0f;
        source->Position[0]  = 0.0f;
        source->Position[1]  = 0.0f;
        source->Position[2]  = 0.0f;
        source->Velocity[0]  = 0.0f;
        source->Velocity[1]  = 0.0f;
        source->Velocity[2]  = 0.0f;
        return source;
    }
    return NULL;
}

void al::release_source(al::source_pool_t *pool, al::source_t *source)
{
    ALuint id = source->Id;
    for (size_t i = 0; i < pool->UsedCount; ++i)
    {
        if (pool->UsedIds[i] == id)
        {
            alSourceStop(id);
            alSourcei(id, AL_BUFFER, 0);
            pool->UsedCount--;
            pool->UsedIds[i]  = pool->UsedIds[pool->UsedCount];
            pool->FreeIds[pool->FreeCount++] = id;
            break;
        }
    }
}

