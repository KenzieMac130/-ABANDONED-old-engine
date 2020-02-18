#pragma once

#ifdef __cplusplus 
extern "C" { 
#endif

#include "asCommon.h"

/**Meta type stack definition
* @warning DO NOT MANUALLY MANUPULATE! USE HELPER MACROS
*/
#define AS_STACK(_Type) struct {\
	int max;\
	int count;\
	void(*fpDestructor)(void*); /*Destructor passes pointer to the data to be collected*/\
	_Type* pData;\
}

/**Meta Stack initialize with destructor function*/
#define AS_STACK_INIT_WITH_DESTRUCTOR(_Stack, _Destructor, _Amount)\
{\
	_Stack.pData = asMalloc(_Amount * sizeof(_Stack.pData[0]));\
	ASASSERT(_Stack.pData);\
	memset(_Stack.pData, 0, _Amount * sizeof(_Stack.pData[0]));\
	_Stack.count = 0;\
	_Stack.max = _Amount;\
	_Stack.fpDestructor = _Destructor;\
}\

/**Meta Stack initialize*/
#define AS_STACK_INIT(_Stack, _Amount)\
	AS_STACK_INIT_WITH_DESTRUCTOR(_Stack, NULL, _Amount)

/**Meta Stack free*/
#define AS_STACK_FREE(_Stack)\
{\
	asFree(_Stack.pData);\
	_Stack.pData = NULL;\
	_Stack.count = 0;\
	_Stack.max = 0;\
	_Stack.fpDestructor = NULL;\
}\

/**Meta Stack push*/
#define AS_STACK_PUSH(_Stack)\
{\
	_Stack.count++;\
	if(_Stack.count > _Stack.max){\
		_Stack.max += 32;\
		_Stack.pData = asRealloc(_Stack.pData, _Stack.max * sizeof(_Stack.pData[0]));\
		ASASSERT(_Stack.pData);\
		memset(&_Stack.pData[_Stack.count - 1], 0, (_Stack.max - _Stack.count) * sizeof(_Stack.pData[0]));\
	}\
}\

/**Meta Stack length*/
#define AS_STACK_LENGTH(_Stack) _Stack.count

/**Meta Stack capacity*/
#define AS_STACK_CAPACITY(_Stack) _Stack.max

/**Meta Stack empty*/
#define AS_STACK_IS_EMPTY(_Stack) (_Stack.count > 0 ? 0 : 1)

/**Meta Stack current*/
#define AS_STACK_AT_INDEX(_Stack, _Index) _Stack.pData[_Index]

/**Meta Stack upper index*/
#define AS_STACK_TOP_INDEX(_Stack) _Stack.count-1

/**Meta Stack upper entry*/
#define AS_STACK_TOP_ENTRY(_Stack) AS_STACK_AT_INDEX(_Stack, AS_STACK_TOP_INDEX(_Stack))

/**Meta Stack pop*/
#define AS_STACK_POP(_Stack)\
{\
	ASASSERT(_Stack.pData);\
	if(_Stack.count > 0){\
		if (_Stack.fpDestructor) {\
				_Stack.fpDestructor((void*)&AS_STACK_TOP_ENTRY(_Stack));\
		}\
		memset(&AS_STACK_TOP_ENTRY(_Stack), 0, sizeof(_Stack.pData[0])); \
		_Stack.count--; \
	}\
}\

/**Meta Stack pop until meets size*/
#define AS_STACK_DOWNSIZE_TO(_Stack, _Target)\
{\
	while(_Stack.count > _Target && _Stack.count > 0){\
		AS_STACK_POP(_Stack);\
	}\
}\

/**Meta Stack clear all*/
#define AS_STACK_CLEAR(_Stack) AS_STACK_DOWNSIZE_TO(_Stack, 0)

#ifdef __cplusplus
}
#endif