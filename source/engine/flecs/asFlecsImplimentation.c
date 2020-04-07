#define FLECS_IMPL 1
#include "asFlecsImplimentation.h"
#include "../engineCore/asEntry.h"

bool flecsEnabled;
ecs_world_t* ecsWorld;

/*Implimentations*/
void ecs_os_log_ASTRENGINE(const char* fmt, va_list args)
{
	_asDebugLoggerLogArgs(AS_DEBUGLOG_MESSAGE, fmt, args);
}

void ecs_os_log_warning_ASTRENGINE(const char* fmt, va_list args)
{
	_asDebugLoggerLogArgs(AS_DEBUGLOG_WARNING, fmt, args);
}

void ecs_os_log_error_ASTRENGINE(const char* fmt, va_list args)
{
	_asDebugLoggerLogArgs(AS_DEBUGLOG_ERROR, fmt, args);
}

void as_os_abort_ASTRENGINE(void)
{
	asFatalError("Entity Component System Crash!\nCheck log file for details...");
}

ASEXPORT asResults asInitFlecs(int argc, char** argv)
{
	ecsWorld = ecs_init_w_args(argc, argv);
	flecsEnabled = true;

	ecs_os_api_t os_api = ecs_os_api;
	os_api.log = ecs_os_log_ASTRENGINE;
	os_api.log_debug = ecs_os_log_ASTRENGINE;
	os_api.log_error = ecs_os_log_error_ASTRENGINE;
	os_api.log_warning = ecs_os_log_warning_ASTRENGINE;
	os_api.abort = as_os_abort_ASTRENGINE;
	ecs_os_set_api(&os_api);

	return AS_SUCCESS;
}

ASEXPORT asResults asShutdownFlecs()
{
	if (flecsEnabled) { ecs_fini(ecsWorld); }
	return AS_SUCCESS;
}

ASEXPORT asResults asUpdateFlecs(float deltaTime)
{
	if (flecsEnabled) { 
		if (ecs_progress(ecsWorld, deltaTime) == false)
		{
			asExitLoop();
		}
	}
	return AS_SUCCESS;
}

ASEXPORT ecs_world_t* asEcsGetWorldPtr()
{
	return ecsWorld;
}

ASEXPORT bool asIsFlecsEnabled()
{
	return flecsEnabled;
}