#pragma once

#include <libshared/logging/logsink.h>

#ifdef WINEXCLUDE_EXPORTS
#define WINEXCLUDE_LINKAGE __declspec(dllexport)
#else
#define WINEXCLUDE_LINKAGE __declspec(dllimport)
#endif

#define WINEXCLUDE_API __stdcall

extern "C"
WINEXCLUDE_LINKAGE
bool
WINEXCLUDE_API
WinExclude_Initialize(
	MullvadLogSink logSink,
	void *logSinkContext
);

extern "C"
WINEXCLUDE_LINKAGE
bool
WINEXCLUDE_API
WinExclude_Deinitialize(
);

enum WINEXCLUDE_UPDATE_STATUS
{
	WINEXCLUDE_UPDATE_STATUS_SUCCESS = 0,

	// One or more paths were not found
	WINEXCLUDE_UPDATE_STATUS_NOT_FOUND = 1,

	WINEXCLUDE_UPDATE_STATUS_INVALID_ARGUMENT = 2,
};

extern "C"
WINEXCLUDE_LINKAGE
WINEXCLUDE_UPDATE_STATUS
WINEXCLUDE_API
WinExclude_SetAppPaths(
	const wchar_t **paths
);
