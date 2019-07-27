#pragma once

#include "cml/cml.h"
#include <AL/al.h>
#include <AL/alc.h>

#include <cstring>
#include <stdio.h>

class SoundSource
{
	float pitch = 1.0f;
	float gain = 1.0f;
	bool loop = false;
	cml::vec3f pos{ 0.0f, 0.0f, 0.0f };

	ALuint source;

	SoundSource (cml::vec3f position, bool loop, float pitch, float gain) : pos (position)
	{
		alGenSources ((ALuint)1, &source);
		// check for errors

		alSourcef (source, AL_PITCH, pitch);
		// check for errors
		alSourcef (source, AL_GAIN, gain);
		// check for errors
		alSource3f (source, AL_POSITION, pos.x, pos.y, pos.z);
		// check for errors
		alSource3f (source, AL_VELOCITY, 0, 0, 0);
		// check for errors
		alSourcei (source, AL_LOOPING, AL_FALSE);
		// check for errors
	}
	~SoundSource () { alDeleteSources (1, &source); }
};

class SoundBuffer
{

	ALuint buffer;
	SoundBuffer ()
	{
		alGenBuffers ((ALuint)1, &buffer);
		// check for errors
	}
	~SoundBuffer () { alDeleteBuffers (1, &buffer); }
};

class SoundDevice
{

	ALCdevice* device;
	ALCcontext* context;

	static void list_audio_devices (const ALCchar* devices)
	{
		const ALCchar *device = devices, *next = devices + 1;
		size_t len = 0;

		fprintf (stdout, "Devices list:\n");
		fprintf (stdout, "----------\n");
		while (device && *device != '\0' && next && *next != '\0')
		{
			fprintf (stdout, "%s\n", device);
			len = strlen (device);
			device += (len + 1);
			next += (len + 2);
		}
		fprintf (stdout, "----------\n");
	}

	SoundDevice ()
	{
		device = alcOpenDevice (NULL);
		if (!device)
		{
			// handle errors
		}

		ALboolean enumeration;
		enumeration = alcIsExtensionPresent (NULL, "ALC_ENUMERATION_EXT");
		if (enumeration == AL_FALSE)
		{
			// enumeration not supported
		}
		else
		{
			// enumeration supported
		}

		list_audio_devices (alcGetString (NULL, ALC_DEVICE_SPECIFIER));

		ALCcontext* context;

		context = alcCreateContext (device, NULL);
		if (!alcMakeContextCurrent (context))
		{
			// failed to make context current
			// test for errors here using alGetError();
		}
	}

	~SoundDevice ()
	{
		device = alcGetContextsDevice (context);
		alcMakeContextCurrent (NULL);
		alcDestroyContext (context);
		alcCloseDevice (device);
	}
};
