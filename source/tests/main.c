#include "engine/entry.h"

#include "stdlib.h"

int main(int argc, char* argv[])
{
	atexit(asShutdown);
	asIgnite(argc, argv);
	return asEnterLoop();
}