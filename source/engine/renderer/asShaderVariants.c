#include "asRenderFx.h"

/*Shader Backend Dependencies*/
#if ASTRENGINE_NUKLEAR
#include "engine/nuklear/asNuklearImplimentation.h"
#endif
#if ASTRENGINE_DEARIMGUI
#include "engine/cimgui/asDearImGuiImplimentation.h"
#endif

#include "engine/renderer/asSceneRenderer.h"

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
				{{"TYPE_GUI","1"}}
			},
			{ /*Fragment*/
				"standard",
				"main",
				AS_SHADERSTAGE_FRAGMENT,
				AS_QUALITY_LOW,
				1, /*Macros*/
				{{"TYPE_GUI","1"}}
			},
		}
	},
	{ /*3D Scene*/
		.name = "Scene",
		.pipelineCount = 1,
		.pipelines = { /*Simplified Rendering Pipeline*/
			"basic",
			AS_PIPELINETYPE_GRAPHICS,
			_asFillGfxPipeline_Scene, /*Callback Function*/
			NULL, /*Callback Data*/
			2, { /*Code Path Mappings*/
				0, 1
			}
		},
		.codePathCount = 2,
		.codePaths = {
			{ /*Vertex*/
				"basic",
				"main",
				AS_SHADERSTAGE_VERTEX,
				AS_QUALITY_LOW,
				2, /*Macros*/
				{
					{"RENDER_SIMPLIFIED","1"},
					{"TYPE_SCENE","1"}
				}
			},
			{ /*Fragment*/
				"basic",
				"main",
				AS_SHADERSTAGE_FRAGMENT,
				AS_QUALITY_LOW,
				2, /*Macros*/
				{
					{"RENDER_SIMPLIFIED","1"},
					{"TYPE_SCENE","1"}
				}
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
