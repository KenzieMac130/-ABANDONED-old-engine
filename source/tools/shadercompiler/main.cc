#include "ShaderGen.h"

#include <omp.h>
#include <iostream>

#define CUTE_FILES_IMPLEMENTATION
#include "cute/cute_files.h"

#include "GLSLGenerator.h"
#include "SPIRVCompiler.h"

int main(int argc, char* argv[])
{
	asCfgFile_t* cfg = asCfgLoad("shaderGen.cfg");

	const char *fileName = asCfgGetString(cfg, "DefaultFile", "ShaderFileMockup.asfxdef");
	/*No file provided*/
	if (argc <= 1)
	{
		asDebugLog("ERROR: NO FILE PROVIDED!\n");
	}
	else
	{
		fileName = argv[1];
	}
	/*Not a valid .asfx file*/
	if (!strstr(fileName, ".asfxdef"))
	{
		asDebugLog("ERROR: .asfxdef NOT PROVIDED!\n");
		return 0;
	}

	const char* includePath = asCfgGetString(cfg, "IncludePath", "includes/");
	const char* templatePath = asCfgGetString(cfg, "TemplatePath", "templates");

	/*Load template files*/
	std::vector<cGlslGenContext> glslTemplates;
	std::vector<asHash32_t> glslTemplateNameHashes;
	glslTemplates.reserve(16);
	glslTemplateNameHashes.reserve(16);
	/*Load Templates*/
	{
		cf_dir_t dir;
		if (!cf_file_exists(templatePath))
		{
			asDebugLog("ERROR: INVALID PATH: %s", templatePath);
			return -3;
		}
		cf_dir_open(&dir, templatePath);
		while (dir.has_next)
		{
			asShaderStage stage;
			cf_file_t file;
			cf_read_file(&dir, &file);
			if (file.is_dir)
			{
				cf_dir_next(&dir);
				continue;
			}
			if (strcmp(file.ext, ".vert") == 0) {
				stage = AS_SHADERSTAGE_VERTEX;
			}
			else if (strcmp(file.ext, ".frag") == 0) {
				stage = AS_SHADERSTAGE_FRAGMENT;
			}
			else if (strcmp(file.ext, ".comp") == 0) {
				stage = AS_SHADERSTAGE_COMPUTE;
			}
			else if (strcmp(file.ext, ".tctrl") == 0) {
				stage = AS_SHADERSTAGE_TESS_CONTROL;
			}
			else if (strcmp(file.ext, ".teval") == 0) {
				stage = AS_SHADERSTAGE_TESS_EVALUATION;
			}
			else if (strcmp(file.ext, ".geo") == 0) {
				stage = AS_SHADERSTAGE_GEOMETRY;
			}
			else {
				continue;
			}
			asDebugLog("Found Template: %s\n", file.name);
			glslTemplateNameHashes.push_back(asHashBytes32_xxHash(file.name, strlen(file.name)));
			cGlslGenContext glslTemp = cGlslGenContext(NULL);
			glslTemp.stage = stage;
			glslTemp.loadGLSLFromFile(file.path);
			glslTemplates.push_back(glslTemp);
			cf_dir_next(&dir);
		}
		cf_dir_close(&dir);
	}
	
	/*Generate the shader fx*/
	asTimer_t finalTimer = asTimerStart();
#pragma omp parallel
	{
		int threadNumber = omp_get_thread_num();
		std::string debugString;
		debugString.reserve(1024);
		std::ostringstream debugOut(debugString);

		/*For each file to process*/
#pragma omp for
		for (int i = 0; i < 1; i++)
		{
			asTimer_t perfTimer = asTimerStart();;
			/*Load the shader file from disc*/
			char* fileBytes = NULL;
			int fileSize = 0;
			FILE *fp = NULL;
			fopen_s(&fp, fileName, "rb");
			if (!fp) {
				debugOut << "ERROR: COULD NOT OPEN FILE!\n";
			}
			else /*Process*/
			{
				debugOut << "Opened: " << fileName << "\n";
				fseek(fp, 0, SEEK_END);
				fileSize = ftell(fp) + 1;
				fseek(fp, 0, SEEK_SET);
				fileBytes = (char*)asMalloc(fileSize);
				fread(fileBytes, 1, fileSize, fp);
				fileBytes[fileSize - 1] = '\0';
				fclose(fp);
				debugOut << "Opened File (microseconds): " <<
					asTimerMicroseconds(perfTimer, asTimerTicksElapsed(perfTimer))
					<< "\n";

				/*Generate shader fx*/
				cFxContext fx = cFxContext(NULL);
				ShaderGenerator::parseShaderFx(fx, fileBytes, fileSize);
				debugOut << "Generated FX (microseconds): " <<
					asTimerMicroseconds(perfTimer, asTimerTicksElapsed(perfTimer))
					<< "\n";

				/*Create GLSL from fx/template*/
				const size_t templateCount = glslTemplates.size();
				std::vector<cGlslGenContext> glslGen;
				glslGen.resize(templateCount, cGlslGenContext(&debugOut));
				for (int tmpl = 0; tmpl < templateCount; tmpl++)
				{
					glslGen[tmpl].generateGLSLFxFromTemplate(&fx, &glslTemplates[tmpl]);
				}
				debugOut << "Generated GLSL (microseconds): " <<
					asTimerMicroseconds(perfTimer, asTimerTicksElapsed(perfTimer))
					<< "\n";

				/*Generate SPIRV*/
				for (int tmpl = 0; tmpl < templateCount; tmpl++)
				{
					cSpirvGenContext spirv = cSpirvGenContext(&debugOut, includePath);
					spirv.genSpirvFromGlsl(&glslGen[tmpl]);
					fx.fxAssemblerAddNativeShaderCode(glslTemplateNameHashes[tmpl], glslTemplates[tmpl].stage, 
						(const char*)spirv.GetBytes(), spirv.GetLength());
				}
				debugOut << "Generated SPIRV (microseconds): " <<
					asTimerMicroseconds(perfTimer, asTimerTicksElapsed(perfTimer))
					<< "\n";

				/*Save shader fx file*/
				std::string outName = fileName;
				outName.replace(outName.begin() + outName.find_last_of('.'), outName.end(), ".asfx");
				asShaderFxDesc_SaveToFile(outName.c_str(), &fx.desc, AS_GFXAPI_VULKAN, 1);
			}
			/*Cleanup*/
			asFree(fileBytes);
		}

		asDebugLog(debugOut.str().c_str());
	}
	asDebugLog("FINISHED IN %f SECONDS", (float)asTimerMicroseconds(finalTimer, asTimerTicksElapsed(finalTimer)) / 1000000);

	asCfgFree(cfg);
	getchar();
}