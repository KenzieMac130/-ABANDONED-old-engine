#include "asReflectImpliment.h"

ASEXPORT int asReflectContainerMemberCount(const asReflectContainer* pContainer)
{
	/*Size Specified*/
	if (pContainer->entryCount)
	{
		return pContainer->entryCount;
	}

	/*Null Terminated*/
	int i = 0;
	const asReflectEntry* pCurrent = pContainer->data;
	while (pCurrent->dataSize)
	{
		pCurrent++;
		i++;
	}
	return i;
}