#include "syscalls.h"
#include "string.h"
#include "alloc.h"
#include "bmp.h"
#include "NAND.h"

typedef struct{
	u32 width;
	u32 height;
	u32 imageData[];
}ImageStruct;

void DrawImage(u32 frameBuffer, u32 x, u32 y, ImageStruct* Image);
ImageStruct* LoadImage(char* str);