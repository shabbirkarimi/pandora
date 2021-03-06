#include <python2.6/Python.h>
#include "main.h"
#include "crypt.h"
#include <fmodex/fmod.h>
#include <fmodex/fmod_errors.h>
#include <time.h>


static FMOD_SYSTEM* sound_system;
static FMOD_SOUND* music;
static FMOD_CHANNEL* channel = 0;
static float original_frequency = 0;
static float volume = 1.0;


static PyMethodDef pandora_methods[] = {
    {"decrypt",  pandora_decrypt, METH_VARARGS, "Decrypt a string from pandora"},
    {"encrypt",  pandora_encrypt, METH_VARARGS, "Encrypt a string from pandora"},
    {"play",  pandora_playMusic, METH_VARARGS, "Play a song"},
    {"pause",  pandora_pauseMusic, METH_VARARGS, "Pause music"},
    {"resume",  pandora_unpauseMusic, METH_VARARGS, "Resume music"},
    {"stop",  pandora_stopMusic, METH_VARARGS, "Stop music"},
    {"is_playing",  pandora_musicIsPlaying, METH_VARARGS, "See if anything is playing"},
    {"set_speed", pandora_setMusicSpeed, METH_VARARGS, "Set the music speed"},
    {"set_volume", pandora_setVolume, METH_VARARGS, "Set the volume"},
    {"stats",  pandora_getMusicStats, METH_VARARGS, "Get music stats"},
    {NULL, NULL, 0, NULL}
};


void pandora_fmod_errcheck(FMOD_RESULT result) {
    if (result != FMOD_OK) {
        printf("FMOD error! (%d) %s\n", result, FMOD_ErrorString(result));
        exit(-1);
    }
}



static PyObject* pandora_decrypt(PyObject *self, PyObject *args) {
    const char *payload;
    char *xml;

    if (!PyArg_ParseTuple(args, "s", &payload)) return NULL;

    xml = PianoDecryptString(payload);
    return Py_BuildValue("s", xml);
}

static PyObject* pandora_getMusicStats(PyObject *self, PyObject *args) {
	if (!channel) Py_RETURN_NONE;

	unsigned int pos;
	(void)FMOD_Channel_GetPosition(channel, &pos, FMOD_TIMEUNIT_MS);

	unsigned int length;
	(void)FMOD_Sound_GetLength(music, &length, FMOD_TIMEUNIT_MS);
	return Py_BuildValue("(ii)", (int)((float)length / 1000.0f), (int)((float)pos / 1000.0f));
}

static PyObject* pandora_setVolume(PyObject *self, PyObject *args) {
	if (!channel) Py_RETURN_NONE;

	if (!PyArg_ParseTuple(args, "f", &volume)) return NULL;

	FMOD_RESULT res;
	res = FMOD_Channel_SetVolume(channel, volume);
	pandora_fmod_errcheck(res);

	Py_RETURN_NONE;
}

static PyObject* pandora_musicIsPlaying(PyObject *self, PyObject *args) {
	FMOD_BOOL isplaying = 0;
	FMOD_RESULT res;

	if (channel) {
		res = FMOD_Channel_IsPlaying(channel, &isplaying);
		pandora_fmod_errcheck(res);
	}

	if (isplaying) {Py_RETURN_TRUE;}
	else {Py_RETURN_FALSE;}
}

static PyObject* pandora_setMusicSpeed(PyObject *self, PyObject *args) {
	FMOD_RESULT res;

	float new_freq;
	if (!PyArg_ParseTuple(args, "f", &new_freq)) return NULL;

	if (channel) {
		res = FMOD_Channel_SetFrequency(channel, new_freq * original_frequency);
		pandora_fmod_errcheck(res);
	}
	Py_RETURN_NONE;
}

static PyObject* pandora_stopMusic(PyObject *self, PyObject *args) {
	(void)FMOD_Channel_Stop(channel);
	Py_RETURN_NONE;
}

static PyObject* pandora_pauseMusic(PyObject *self, PyObject *args) {
	FMOD_RESULT res;
	FMOD_BOOL isplaying;

	if (channel) {
		res = FMOD_Channel_IsPlaying(channel, &isplaying);
		pandora_fmod_errcheck(res);
		if (isplaying) {
			res = FMOD_Channel_SetPaused(channel, 1);
			pandora_fmod_errcheck(res);
		}
	}
	Py_RETURN_NONE;
}

static PyObject* pandora_unpauseMusic(PyObject *self, PyObject *args) {
	FMOD_RESULT res;
	FMOD_BOOL isplaying;

	if (channel) {
		res = FMOD_Channel_IsPlaying(channel, &isplaying);
		pandora_fmod_errcheck(res);
		if (isplaying) {
			res = FMOD_Channel_SetPaused(channel, 0);
			pandora_fmod_errcheck(res);
		}
	}
	Py_RETURN_NONE;
}


static PyObject* pandora_playMusic(PyObject *self, PyObject *args) {
    const char *song_file;

    if (!PyArg_ParseTuple(args, "s", &song_file)) return NULL;

    FMOD_RESULT res;
    res = FMOD_System_CreateSound(sound_system, song_file, FMOD_SOFTWARE, 0, &music);
    pandora_fmod_errcheck(res);
    res = FMOD_System_PlaySound(sound_system, FMOD_CHANNEL_FREE, music, 0, &channel);
    pandora_fmod_errcheck(res);

    res = FMOD_Channel_GetFrequency(channel, &original_frequency);
    pandora_fmod_errcheck(res);

    res = FMOD_Channel_SetVolume(channel, volume);
    pandora_fmod_errcheck(res);

	unsigned int length;
	res = FMOD_Sound_GetLength(music, &length, FMOD_TIMEUNIT_MS);
	pandora_fmod_errcheck(res);
	return Py_BuildValue("i", (int)((float)length / 1000.0f));
}

static PyObject* pandora_encrypt(PyObject *self, PyObject *args) {
    const char *xml;
    char *payload;

    if (!PyArg_ParseTuple(args, "s", &xml)) return NULL;

    payload = PianoEncryptString(xml);
    return Py_BuildValue("s", payload);
}

static void cleanup() {
    (void)FMOD_Sound_Release(music);
    (void)FMOD_System_Close(sound_system);
    (void)FMOD_System_Release(sound_system);
}

PyMODINIT_FUNC init_pandora(void) {
	FMOD_System_Create(&sound_system);
	FMOD_System_Init(sound_system, 32, FMOD_INIT_NORMAL, NULL);

    (void) Py_InitModule("_pandora", pandora_methods);

    Py_AtExit(cleanup);
}
