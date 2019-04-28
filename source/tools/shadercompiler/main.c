#include "FileParser.h"

char defaultFile[] = "ShaderFileMockup.asfxdef";

int main(int argc, char* argv[])
{
	char *fileName = defaultFile;
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
	if (strstr(fileName, ".asfxdef") == NULL)
	{
		asDebugLog("ERROR: .asfxdef NOT PROVIDED!\n");
		return;
	}

	/*Load the shader file from disc*/
	char* fileBytes;
	int fileSize;
	{
		FILE *fp;
		fopen_s(&fp, fileName, "rb");
		if (!fp) {
			asDebugLog("ERROR: COULD NOT OPEN FILE!\n");
			return;
		}
		fseek(fp, 0, SEEK_END);
		fileSize = ftell(fp) + 1;
		fseek(fp, 0, SEEK_SET);
		fileBytes = (char*)asMalloc(fileSize);
		fread(fileBytes, 1, fileSize, fp);
		fileBytes[fileSize-1] = '\0';
		fclose(fp);
	}
	
	/*Generate the shader fx*/
	generateShaderFromFile(fileBytes, fileSize);

	asFree(fileBytes);
	getchar();
}