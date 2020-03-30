#include "asRenderFx.h"

/*Shader Backend Dependencies*/
#if ASTRENGINE_NUKLEAR
#include "engine/nuklear/asNuklearImplimentation.h"
#endif
#if ASTRENGINE_DEARIMGUI
#include "engine/cimgui/asDearImGuiImplimentation.h"
#endif

asShaderTypeRegistration shaderTypes[] =
{
	{ /*2D GUI*/
		.name = "Gui2D",
		.pipelineCount = 1,
		.pipelines = { /*Standard Pipeline*/
			"standard",
			AS_PIPELINETYPE_GRAPHICS,
			#if ASTRENGINE_NUKLEAR
			_asFillGfxPipeline_Nuklear, /*Callback Function*/
			#elif ASTRENGINE_DEARIMGUI
			_asFillGfxPipeline_DearImgui,
			#else
			NULL,
			#endif
			NULL, /*Callback Data*/
			2, { /*Code Path Mappings*/
				0, 1
			}
		},
		.codePathCount = 2,
		.codePaths = {
			{ /*Vertex*/
				"standard",
				"main",
				AS_SHADERSTAGE_VERTEX,
				AS_QUALITY_LOW,
				1, /*Macros*/
				{{"NUKLEAR","1"}}
			},
			{ /*Fragment*/
				"standard",
				"main",
				AS_SHADERSTAGE_FRAGMENT,
				AS_QUALITY_LOW,
				1, /*Macros*/
				{{"NUKLEAR","1"}}
			},
		}
	},
};

ASEXPORT const asShaderTypeRegistration* asShaderFindTypeRegistrationByName(const char* name)
{
	for (size_t i = 0; i < ASARRAYLEN(shaderTypes); i++)
	{
		if (asIsStringEqual(shaderTypes[i].name, name))
		{
			return &shaderTypes[i];
		}
	}
	return NULL;
}
