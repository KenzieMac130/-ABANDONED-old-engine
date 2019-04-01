#include "include/entry.h"

#include "stdlib.h"
#include "stdio.h"
#include "astrengineConfig.h"

#include <stdbool.h>

bool loopContinue = true;

void asShutdown(void)
{
	printf("astrengine Quit...");
}

int asIgnite(int argc, char *argv[])
{
	printf("astrengine %d.%d\n", ASTRENGINE_VERSION_MAJOR, ASTRENGINE_VERSION_MINOR);
	return 0; 
}

int asLoopSingleShot()
{
	printf("PING\n");
	return 0;
}

int asEnterLoop()
{
	int result;
	while (loopContinue)
	{
		result = asLoopSingleShot();
		if (result)
			return result;
	}
	return 0;
}