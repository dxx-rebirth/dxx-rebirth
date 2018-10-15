/*
 * This file is part of the DXX-Rebirth project <https://www.dxx-rebirth.com/>.
 * It is copyright by its individual contributors, as recorded in the
 * project's Git history.  See COPYING.txt at the top level for license
 * terms and a link to the Git history.
 */
/*
 * This is a dynamic interface to libADLMIDI, the OPL3 synthesizer library.
 */

#include "adlmidi_dynamic.h"
#include "console.h"
#if !defined(_WIN32)
#include <dlfcn.h>
#else
#include <windows.h>
#endif

#if defined(_WIN32)
enum
{
	RTLD_LAZY = 1, RTLD_NOW = 2
};

void *dlopen(const char *filename, int)
{
	return LoadLibraryA(filename);
}

void dlclose(void *handle)
{
	FreeLibrary(reinterpret_cast<HMODULE>(handle));
}

void *dlsym(void *handle, const char *symbol)
{
	return reinterpret_cast<void *>(
		GetProcAddress(reinterpret_cast<HMODULE>(handle), symbol));
}
#endif

static ADL_MIDIPlayer *adl_init_failure(long)
{
	return nullptr;
}

template <class F>
static bool load_function(void *handle, const char *name, F *&fptr)
{
	fptr = reinterpret_cast<F *>(dlsym(handle, name));
	if (!fptr)
		con_printf(CON_NORMAL, "ADLMIDI: failed to load the dynamic function \"%s\"", name);
	return fptr != nullptr;
}

static ADL_MIDIPlayer *adl_init_first_call(long sample_rate)
{
#if defined(_WIN32)
	const char *library_name = "libADLMIDI.dll";
#elif defined(__APPLE__)
	const char *library_name = "libADLMIDI.dylib";
#else
	const char *library_name = "libADLMIDI.so";
#endif
	const auto handle = dlopen(library_name, RTLD_NOW);
	if (!handle ||
	    !load_function(handle, "adl_init", adl_init) ||
	    !load_function(handle, "adl_close", adl_close) ||
	    !load_function(handle, "adl_switchEmulator", adl_switchEmulator) ||
	    !load_function(handle, "adl_setNumChips", adl_setNumChips) ||
	    !load_function(handle, "adl_setBank", adl_setBank) ||
	    !load_function(handle, "adl_setSoftPanEnabled", adl_setSoftPanEnabled) ||
	    !load_function(handle, "adl_setLoopEnabled", adl_setLoopEnabled) ||
	    !load_function(handle, "adl_openData", adl_openData) ||
	    !load_function(handle, "adl_openFile", adl_openFile) ||
	    !load_function(handle, "adl_playFormat", adl_playFormat))
	{
		adl_init = &adl_init_failure;
		if (handle)
			dlclose(handle);
		else
			con_printf(CON_NORMAL, "ADLMIDI: failed to load the dynamic library \"%s\"", library_name);
	}
	else
		con_printf(CON_NORMAL, "ADLMIDI: loaded the dynamic OPL3 synthesizer");
	return adl_init(sample_rate);
}

ADL_MIDIPlayer *(*adl_init)(long sample_rate) = &adl_init_first_call;
void (*adl_close)(ADL_MIDIPlayer *device) = nullptr;
int (*adl_switchEmulator)(ADL_MIDIPlayer *device, int emulator) = nullptr;
int (*adl_setNumChips)(ADL_MIDIPlayer *device, int numChips) = nullptr;
int (*adl_setBank)(ADL_MIDIPlayer *device, int bank) = nullptr;
void (*adl_setSoftPanEnabled)(ADL_MIDIPlayer *device, int softPanEn) = nullptr;
void (*adl_setLoopEnabled)(ADL_MIDIPlayer *device, int loopEn) = nullptr;
int (*adl_openData)(ADL_MIDIPlayer *device, const void *mem, unsigned long size) = nullptr;
int (*adl_openFile)(ADL_MIDIPlayer *device, const char *filePath) = nullptr;
int (*adl_playFormat)(ADL_MIDIPlayer *device, int sampleCount, uint8_t *left, uint8_t *right, const ADLMIDI_AudioFormat *format) = nullptr;
