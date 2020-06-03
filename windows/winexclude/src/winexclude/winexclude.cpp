#include "stdafx.h"
#include "winexclude.h"
#include <stdexcept>
#include <libcommon/error.h>

namespace
{

MullvadLogSink g_logSink = nullptr;
void *g_logSinkContext = nullptr;

} // anonymous namespace

extern "C"
WINEXCLUDE_LINKAGE
bool
WINEXCLUDE_API
WinExclude_Initialize(
	MullvadLogSink logSink,
	void *logSinkContext
)
{
	g_logSink = logSink;
	g_logSinkContext = logSinkContext;

	return true;
}

extern "C"
WINEXCLUDE_LINKAGE
bool
WINEXCLUDE_API
WinExclude_Deinitialize(
)
{
	return true;
}

extern "C"
WINEXCLUDE_LINKAGE
WINEXCLUDE_UPDATE_STATUS
WINEXCLUDE_API
WinExclude_SetAppPaths(
	const wchar_t **paths
)
{
	if (nullptr == paths)
	{
		THROW_ERROR("Invalid argument: paths");
		return WINEXCLUDE_UPDATE_STATUS_INVALID_ARGUMENT;
	}

	/*
	for (const wchar_t **pathPtr = paths; nullptr != *pathPtr; pathPtr++)
	{
		const wchar_t *path = *pathPtr;
	}
	*/

	return WINEXCLUDE_UPDATE_STATUS_SUCCESS;
}
