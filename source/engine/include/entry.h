#pragma once
/**
* @file
* @brief Entry point functionality
*/

/**
* @brief Starts up astrengine
*/
int asIgnite(int argc, char *argv[]);
/**
* @brief Enters the main engine loop
*/
int asEnterLoop();
/**
* @brief Single shot the engine loop (useful for integrating into editors)
*/
int asLoopSingleShot();
/**
* @brief Shutdown the engine
*/
void asShutdown(void);