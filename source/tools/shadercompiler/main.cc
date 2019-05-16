#include "ShaderGen.h"

/*Todo: Yeahhh this was better suited for C++ in hindsight, when maintnence is needed go ahead and port it*/
/*Todo: Multiple Files using Multiple Threads (will need to resolve debug logging)*/

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

	/*Load the shader file from disc*/
	char* fileBytes;
	int fileSize;
	{
		FILE *fp;
		fopen_s(&fp, fileName, "rb");
		if (!fp) {
			asDebugLog("ERROR: COULD NOT OPEN FILE!\n");
			return 0;
		}
		asDebugLog("Opened: %s\n", fileName);
		fseek(fp, 0, SEEK_END);
		fileSize = ftell(fp) + 1;
		fseek(fp, 0, SEEK_SET);
		fileBytes = (char*)asMalloc(fileSize);
		fread(fileBytes, 1, fileSize, fp);
		fileBytes[fileSize-1] = '\0';
		fclose(fp);
	}
	
	/*Generate the shader fx*/
	shaderGenResult_t result = generateShaderFxFromTemplates(fileBytes, fileSize,
			asCfgGetString(cfg, "TemplatePath", "templates"),
			asCfgGetString(cfg, "IncludePath", "includes/"));

	asFree(fileBytes);
	asCfgFree(cfg);
	getchar();
}