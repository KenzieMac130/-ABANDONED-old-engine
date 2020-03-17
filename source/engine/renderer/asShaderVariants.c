#include "asShaderVariants.h"

#include "engine/nuklear/asNuklearImplimentation.h"

asShaderTypeRegistration shaderTypes[] =
{
	{ /*2D GUI*/
		.name = "Gui2D",
		.pipelineCount = 1,
		.pipelines = { /*Standard Pipeline*/
			"standard",
			AS_PIPELINETYPE_GRAPHICS,
			_asFillGfxPipeline_Nuklear, /*Callback Function*/
			NULL, /*Callback Data*/
			2, {
				{ /*Vertex*/
					"main",
					AS_SHADERSTAGE_VERTEX,
					AS_QUALITY_LOW,
					1, /*Macros*/
					{{"NUKLEAR","1"}}
				},
				{ /*Fragment*/
					"main",
					AS_SHADERSTAGE_FRAGMENT,
					AS_QUALITY_LOW,
					1, /*Macros*/
					{{"NUKLEAR","1"}}
				},
			}
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
