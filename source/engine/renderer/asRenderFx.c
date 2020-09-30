#include "asRenderFx.h"

#if ASTRENGINE_VK
#include "vulkan/asVulkanBackend.h"
#endif

ASEXPORT asResults asCreateShaderFx(asBinReader* pAsbin, asShaderFx* pShaderfx, asQualityLevel quality)
{
	/*Get Variant Data*/
	const char* variantName;
	asBinReaderGetSection(pAsbin, (asBinSectionIdentifier) { "VARIANT", 0 }, &variantName, NULL);
	asShaderTypeRegistration* pShaderType = asShaderFindTypeRegistrationByName(variantName);
	pShaderfx->registration = pShaderType;
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
#if ASTRENGINE_VK
		int stageCount = 0;
		VkShaderModule shaderModules[AS_SHADER_MAX_CODEPATHS];
		VkPipelineShaderStageCreateInfo stageCreateInfos[AS_SHADER_MAX_CODEPATHS];
		memset(shaderModules, 0, sizeof(VkShaderModule) * AS_SHADER_MAX_CODEPATHS);
		memset(stageCreateInfos, 0, sizeof(VkPipelineShaderStageCreateInfo) * AS_SHADER_MAX_CODEPATHS);
#endif
		/*Find each associated section*/
		asHash32_t groupHash = asHashBytes32_xxHash(pShaderType->pipelines[i].name, strlen(pShaderType->pipelines[i].name));
		for (int j = 0; j < pShaderType->pipelines[i].codePathIdxCount; j++)
		{
			int32_t codePathIndex = pShaderType->pipelines[i].codePathIdxs[j];
			asShaderFxProgramDesc* pDesc = NULL;
			int minQuality = -1;
			for (int k = 0; k < codeSectionCount; k++) /*Search for Shader to Add*/
			{
				/*Correct Section Found*/
				if (pCodeSections[k].defGroupNameHash == groupHash &&
					pCodeSections[k].stage == pShaderType->codePaths[codePathIndex].stage &&
					pCodeSections[k].quality > minQuality &&
					pCodeSections[k].quality <= quality)
				{
					pDesc = &pCodeSections[k];
					minQuality = pCodeSections[k].quality;
				}
			}
			if (!pDesc) { return AS_FAILURE_UNKNOWN_FORMAT; }
			stageCount++;
#if ASTRENGINE_VK
			/*Don't Create Redundant Copies*/
			if (shaderModules[codePathIndex] != 0) { continue; }
			/*Add Shader to shaderModules*/
			size_t codeSize;
			int32_t* pSpirv;
			asBinReaderGetSection(pAsbin, (asBinSectionIdentifier) { "SPIR-V", pDesc->programSection }, &pSpirv, &codeSize);

			VkShaderModuleCreateInfo createInfo = (VkShaderModuleCreateInfo){ VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO };
			createInfo.codeSize = codeSize;
			createInfo.pCode = pSpirv;
			vkCreateShaderModule(asVkDevice, &createInfo, AS_VK_MEMCB, &shaderModules[codePathIndex]);

			/*Describe Stage*/
			stageCreateInfos[codePathIndex] = (VkPipelineShaderStageCreateInfo){ VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO };
			stageCreateInfos[codePathIndex].module = shaderModules[codePathIndex];
			stageCreateInfos[codePathIndex].pName = pShaderType->codePaths[codePathIndex].entry;
			stageCreateInfos[codePathIndex].stage = asVkConvertToNativeStage(pShaderType->codePaths[codePathIndex].stage);
			stageCreateInfos[codePathIndex].pSpecializationInfo = NULL; /*Pipeline Specialization is not implimented*/
#endif
		}

#if ASTRENGINE_VK
		if (pShaderType->pipelines[i].type == AS_PIPELINETYPE_GRAPHICS)
		{
			VkGraphicsPipelineCreateInfo gfxPipelineInfo = (VkGraphicsPipelineCreateInfo){ VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO };
			gfxPipelineInfo.pStages = stageCreateInfos;
			gfxPipelineInfo.stageCount = stageCount;

			/*Call Pipleine Creation Callback*/
			pShaderType->pipelines[i].fpCreatePipelineCallback(
				pAsbin,
				AS_GFXAPI_VULKAN,
				AS_PIPELINETYPE_GRAPHICS,
				&gfxPipelineInfo,
				pShaderType->pipelines[i].name,
				&pShaderfx->pipelines[i],
				pShaderType->pipelines[i].pUserData);
		}
		else if (pShaderType->pipelines[i].type == AS_PIPELINETYPE_COMPUTE)
		{
			VkComputePipelineCreateInfo computePipelineInfo = (VkComputePipelineCreateInfo){ VK_STRUCTURE_TYPE_COMPUTE_PIPELINE_CREATE_INFO };
			computePipelineInfo.stage = stageCreateInfos[0];

			/*Call Pipleine Creation Callback*/
			pShaderType->pipelines[i].fpCreatePipelineCallback(
				pAsbin,
				AS_GFXAPI_VULKAN,
				AS_PIPELINETYPE_COMPUTE,
				&computePipelineInfo,
				pShaderType->pipelines[i].name,
				&pShaderfx->pipelines[i],
				pShaderType->pipelines[i].pUserData);
		}
		else { return AS_FAILURE_UNKNOWN_FORMAT; }

		/*Free Shader Modules*/
		for (int j = 0; j < pShaderType->codePathCount; j++) { vkDestroyShaderModule(asVkDevice, shaderModules[j], AS_VK_MEMCB); }
#endif
	}
	return AS_SUCCESS;
}

ASEXPORT void asFreeShaderFx(asShaderFx* pShaderfx)
{
	for (int i = 0; i < pShaderfx->registration->pipelineCount; i++)
	{
#if ASTRENGINE_VK
		if (pShaderfx->pipelines[i] != VK_NULL_HANDLE) {
			vkDestroyPipeline(asVkDevice, (VkPipeline)pShaderfx->pipelines[i], AS_VK_MEMCB);
			pShaderfx->pipelines[i] = VK_NULL_HANDLE;
		}
#endif
	}
}
