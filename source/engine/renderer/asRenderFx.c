#include "asRenderFx.h"

#include "asShaderVariants.h"

#if ASTRENGINE_VK
#include "vulkan/asVulkanBackend.h"
#endif

ASEXPORT asResults asCreateShaderFx(asBinReader* pAsbin, asShaderFx* pShaderfx, asQualityLevel quality)
{
	/*Get Variant Data*/
	const char* variantName;
	asBinReaderGetSection(pAsbin, (asBinSectionIdentifier) { "VARIANT", 0 }, &variantName, NULL);
	asShaderTypeRegistration* pShaderType = asShaderFindTypeRegistrationByName(variantName);
	if (!pShaderType) { return AS_FAILURE_DATA_DOES_NOT_EXIST; }

	/*Retrieve Code Sections*/
	size_t codeSectionSize;
	size_t codeSectionCount;
	asShaderFxProgramDesc* pCodeSections;
	asBinReaderGetSection(pAsbin, (asBinSectionIdentifier) { "CODESECT", 0 }, & pCodeSections, &codeSectionSize);
	codeSectionCount = codeSectionSize / sizeof(asShaderFxProgramDesc);

	/*Create Each Pipeline*/
	for (int i = 0; i < pShaderType->pipelineCount; i++)
	{
		if (pShaderType->pipelines[i].type == AS_PIPELINETYPE_GRAPHICS)
		{
#if ASTRENGINE_VK
			int currentCreateInfo = 0;
			VkShaderModule shaderModules[AS_SHADER_MAX_CODEPATHS];
			VkGraphicsPipelineCreateInfo gfxPipelineInfo;
			gfxPipelineInfo.pStages = shaderModules;
#endif
			/*Find each associated section*/
			asHash32_t groupHash = asHashBytes32_xxHash(pShaderType->pipelines[i].name, strlen(pShaderType->pipelines[i].name));
			for (int j = 0; j < pShaderType->pipelines[i].codePathCount; j++)
			{
				asShaderFxProgramDesc* pDesc = NULL;
				int minQuality = -1;
				for (int k = 0; k < codeSectionCount; k++) /*Search for Shader to Add*/
				{
					/*Correct Section Found*/
					if (pCodeSections[k].defGroupNameHash == groupHash &&
						pCodeSections[k].stage == pShaderType->pipelines[i].codePaths[j].stage &&
						pCodeSections[k].quality > minQuality &&
						pCodeSections[k].quality <= quality){
						pDesc = &pCodeSections[k];
						minQuality = pCodeSections[k].quality;
					}
				}
				if (!pDesc) { return AS_FAILURE_UNKNOWN_FORMAT; }

#if ASTRENGINE_VK
				/*Add Shader to shaderModules*/
				size_t codeSize;
				int32_t* pSpirv;
				asBinReaderGetSection(pAsbin, (asBinSectionIdentifier) { "SPIR-V", pDesc->programSection }, &pSpirv, &codeSize);
				
				vkCreateShaderModule(asVkDevice, ) /*Todo*/
				//pShaderType->pipelines[i].codePaths[j].entry
#endif
			}
			gfxPipelineInfo.stageCount = currentCreateInfo;

			/*Call Pipleine Creation Callback*/

			/*Free Shader Modules*/
		}
		else if (pShaderType->pipelines[i].type == AS_PIPELINETYPE_COMPUTE)
		{
			/*Todo: Compute*/
		}
	}

	return AS_SUCCESS;
}

ASEXPORT void asFreeShaderFx(asShaderFx* pShaderfx)
{

}
