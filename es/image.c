#include "image.h"

typedef struct{
	u32 size;
	char tID[4];
	char data[];
}PNGBlock;//followed with 4 byte CRC


void DrawImage(u32 frameBuffer, u32 x, u32 y, ImageStruct* Image){
	u32* fb = (u32*) frameBuffer;
	u32 i, j;

	u32 halfWidth = Image->width / 2;

	for (j = 0; j < Image->height && y + j < 440; j++){
		for (i = 0; i < halfWidth; i++){
			fb[(x+i)+(y+j)*320] = Image->imageData[i + j * halfWidth];
		}
	}
}

/*ImageStruct* LoadPNG(unsigned char* rawData, u32 size){
	u32 pos = 8;
	u8 endFound = 0;
	do{
	} while (endFound != 0 && pos < size);
	if (endFound != 0)
		return NULL;
}*/

u32 RGBtoYCbYCr(u8 r1, u8 g1, u8 b1, u8 r2, u8 g2, u8 b2){
	  u32 y1, cb1, cr1, y2, cb2, cr2, cb, cr;

	  y1 = (299 * r1 + 587 * g1 + 114 * b1) / 1000;
	  cb1 = (-16874 * r1 - 33126 * g1 + 50000 * b1 + 12800000) / 100000;
	  cr1 = (50000 * r1 - 41869 * g1 - 8131 * b1 + 12800000) / 100000;
 
	  y2 = (299 * r2 + 587 * g2 + 114 * b2) / 1000;
	  cb2 = (-16874 * r2 - 33126 * g2 + 50000 * b2 + 12800000) / 100000;
	  cr2 = (50000 * r2 - 41869 * g2 - 8131 * b2 + 12800000) / 100000;
 
	  cb = (cb1 + cb2) >> 1;
	  cr = (cr1 + cr2) >> 1;
	return (y1 << 24) | (cb << 16) | (y2 << 8) | cr;
}

u32 flipIntValue(u32 value){
	return ((value & 0xFF) << 24) | ((value & 0xFF00) << 8) | ((value & 0xFF0000) >> 8) | ((value & 0xFF000000) >> 24);
}

u16 flipShortValue(u16 value){
	return ((value & 0xFF) << 8) | ((value & 0xFF00) >> 8);
}

ImageStruct* LoadBMP(s32 fd, u32 size){
	if (size < sizeof(bmpfile_magic) + sizeof(bmpfile_header) + sizeof(bmp_dib_v3_header_t))
		return NULL;
	IOS_Seek(fd,sizeof(bmpfile_magic),SEEK_SET);
	bmpfile_header* bmpHeader = (bmpfile_header*) malloca(sizeof(bmpfile_header),32);
	IOS_Read(fd,bmpHeader,sizeof(bmpfile_header));
	bmp_dib_v3_header_t* dibHeader = (bmp_dib_v3_header_t*) malloca(sizeof(bmp_dib_v3_header_t),32);
	IOS_Read(fd,dibHeader,sizeof(bmp_dib_v3_header_t));
	//correct int byte arrangement
	bmpHeader->bmp_offset = flipIntValue(bmpHeader->bmp_offset);
	dibHeader->header_sz = flipIntValue(dibHeader->header_sz);
	dibHeader->nplanes = flipShortValue(dibHeader->nplanes);
	dibHeader->bitspp = flipShortValue(dibHeader->bitspp);
	dibHeader->compress_type = flipIntValue(dibHeader->compress_type);
	dibHeader->width = flipIntValue(dibHeader->width);
	dibHeader->height = flipIntValue(dibHeader->height);
	if (dibHeader->header_sz != sizeof(bmp_dib_v3_header_t)){
		free(bmpHeader);
		free(dibHeader);
		return NULL; //wrong header type
	}
	if (dibHeader->nplanes != 1){
		free(bmpHeader);
		free(dibHeader);
		return NULL; //must only use 1 plane
	}
	if (dibHeader->compress_type != BI_RGB){
		free(bmpHeader);
		free(dibHeader);
		return NULL; //must be uncompressed
	}
	s32 width, height, realWidth;
	u32 imageDataPos = bmpHeader->bmp_offset;
	free(bmpHeader);
	width = dibHeader->width;
	realWidth = width;
	if (width % 2)
		width += 1;
	u32 halfWidth = width / 2;
	height = dibHeader->height;
	u32 bitspp = dibHeader->bitspp;
	free(dibHeader);
	u32 constructedSize = sizeof(ImageStruct) + halfWidth * height * sizeof(u32);
	if (constructedSize > 90000){
		return NULL;
	}
	ImageStruct* image = (ImageStruct*)malloca(constructedSize,32);
	image->width = width;
	image->height = height;
	IOS_Seek(fd,imageDataPos,SEEK_SET);
	switch (bitspp){
		case 1:
		{
			IOS_Seek(fd,sizeof(bmpfile_magic) + sizeof(bmpfile_header) + sizeof(bmp_dib_v3_header_t),SEEK_SET);
			u8* colorTable = (u8*)malloca(2*3,32);
			s32 x, y, i;
			u32 iy, ix;
			u32 bytesToSkip = (width / 8) % 4;
			iy = height - 1;
			for (x = 0; x < 2; x++){
				IOS_Read(fd,&colorTable[x*3],3);
				IOS_Seek(fd,1,SEEK_CUR);
			}
			IOS_Seek(fd,imageDataPos,SEEK_SET);
			u8* pixelData = (u8*)malloca(1,32);
			for (y = 0; y < height; y++){
				ix = 0;
				for (x = 0; x < width; x += 8){
					IOS_Read(fd,&pixelData[0],1);
					for (i = 8; i > 0 && x + 8 - i < realWidth; i -= 2){
						u8 p1, p2;
						p1 = (pixelData[0] >> (i - 1)) & 1;
						if (x + 7 - i < realWidth)
							p2 = (pixelData[0] >> (i - 2)) & 1;
						else
							p2 = p1;
						image->imageData[iy * halfWidth + ix] = RGBtoYCbYCr(colorTable[p1 * 3 + 2],colorTable[p1 * 3 + 1],colorTable[p1 * 3 + 0],colorTable[p2 * 3 + 2],colorTable[p2 * 3 + 1],colorTable[p2 * 3 + 0]);
						ix++;
					}
				}
				IOS_Seek(fd,bytesToSkip,SEEK_CUR);
				iy--;
			}
			free(pixelData);
			free(colorTable);
		} break;
		case 4:
		{
			IOS_Seek(fd,sizeof(bmpfile_magic) + sizeof(bmpfile_header) + sizeof(bmp_dib_v3_header_t),SEEK_SET);
			u8* colorTable = (u8*)malloca(16*3,32);
			s32 x, y;
			u32 iy, ix;
			u32 bytesToSkip = (width / 2) % 4;
			iy = height - 1;
			for (x = 0; x < 16; x++){
				IOS_Read(fd,&colorTable[x*3],3);
				IOS_Seek(fd,1,SEEK_CUR);
			}
			IOS_Seek(fd,imageDataPos,SEEK_SET);
			u8* pixelData = (u8*)malloca(1,32);
			for (y = 0; y < height; y++){
				ix = 0;
				for (x = 0; x < width; x += 2){
					IOS_Read(fd,&pixelData[0],1);
					u16 p1, p2;
					p1 = (pixelData[0] >> 4) & 0xF;
					if (x + 1 < realWidth)
						p2 = pixelData[0] &0xF;
					else
						p2 = p1;
					image->imageData[iy * halfWidth + ix] = RGBtoYCbYCr(colorTable[p1 * 3 + 2],colorTable[p1 * 3 + 1],colorTable[p1 * 3 + 0],colorTable[p2 * 3 + 2],colorTable[p2 * 3 + 1],colorTable[p2 * 3 + 0]);
					ix++;
				}
				IOS_Seek(fd,bytesToSkip,SEEK_CUR);
				iy--;
			}
			free(pixelData);
			free(colorTable);
		} break;
		case 8:
		{
			IOS_Seek(fd,sizeof(bmpfile_magic) + sizeof(bmpfile_header) + sizeof(bmp_dib_v3_header_t),SEEK_SET);
			u8* colorTable = (u8*)malloca(256*3,32);
			s32 x, y;
			u32 iy, ix;
			u32 bytesToSkip = width % 4;
			iy = height - 1;
			for (x = 0; x < 256; x++){
				IOS_Read(fd,&colorTable[x*3],3);
				IOS_Seek(fd,1,SEEK_CUR);
			}
			IOS_Seek(fd,imageDataPos,SEEK_SET);
			u8* pixelData = (u8*)malloca(2,32);
			for (y = 0; y < height; y++){
				ix = 0;
				for (x = 0; x < width; x += 2){
					if (x + 1 < realWidth)
						IOS_Read(fd,&pixelData[0],2);
					else{
						IOS_Read(fd,&pixelData[0],1);
						pixelData[1] = pixelData[0];
					}
					image->imageData[iy * halfWidth + ix] = RGBtoYCbYCr(colorTable[pixelData[0] * 3 + 2],colorTable[pixelData[0] * 3 + 1],colorTable[pixelData[0] * 3 + 0],colorTable[pixelData[1] * 3 + 2],colorTable[pixelData[1] * 3 + 1],colorTable[pixelData[1] * 3 + 0]);
					ix++;
				}
				IOS_Seek(fd,bytesToSkip,SEEK_CUR);
				iy--;
			}
			free(pixelData);
			free(colorTable);
		} break;
		case 24:
		{
			s32 x, y;
			u32 iy, ix;
			u32 bytesToSkip = (width * 3) % 4;
			iy = height - 1;
			u8* pixelData = (u8*)malloca(6,32);
			for (y = 0; y < height; y++){
				ix = 0;
				for (x = 0; x < width; x += 2){
					if (x + 1 < realWidth)
						IOS_Read(fd,&pixelData[0],6);
					else{
						IOS_Read(fd,&pixelData[0],3);
						memcpy(&pixelData[3],&pixelData[0],3);
					}
					image->imageData[iy * halfWidth + ix] = RGBtoYCbYCr(pixelData[2],pixelData[1],pixelData[0],pixelData[5],pixelData[4],pixelData[3]);
					ix++;
				}
				IOS_Seek(fd,bytesToSkip,SEEK_CUR);
				iy--;
			}
			free(pixelData);
		} break;
		case 32:
		{
			s32 x, y;
			u32 iy, ix;
			iy = height - 1;
			u8* pixelData = (u8*)malloca(8,32);
			for (y = 0; y < height; y++){
				ix = 0;
				for (x = 0; x < width; x += 2){
					if (x + 1 < realWidth)
						IOS_Read(fd,&pixelData[0],8);
					else{
						IOS_Read(fd,&pixelData[0],4);
						memcpy(&pixelData[3],&pixelData[0],3);
					}
					image->imageData[iy * halfWidth + ix] = RGBtoYCbYCr(pixelData[2],pixelData[1],pixelData[0],pixelData[6],pixelData[5],pixelData[4]);
					ix++;
				}
				iy--;
			}
			free(pixelData);
		} break;
		default: //unsupported colordepth
		{
			free(image);
			image = NULL;
		} break;
	}
	return image;
}

ImageStruct* LoadRaw(s32 fd, u32 size){
	if (size < sizeof(ImageStruct))
		return NULL;
	if (size > 90000)
		return NULL;
	ImageStruct* image = (ImageStruct*)malloca(size,32);
	IOS_Seek(fd,0,SEEK_SET);
	IOS_Read(fd,image,size);
	return image;
}

ImageStruct* LoadImage(char* str){
	dbgprintf("Loading img: %s",str);
	u32 size = 0;
	u8 isRaw = 0;
	ImageStruct* image = NULL;
	s32 fd = IOS_Open(str,1);
	if (fd < 0)
		return NULL;
	size = IOS_Seek( fd, 0, SEEK_END );
	if (size < 2){
		IOS_Close(fd);
		return NULL;
	}
	IOS_Seek( fd, 0, SEEK_SET);
	u8* fileData = (u8*) malloca(2,32);
	memset32(fileData,0,2);
	IOS_Read( fd, fileData, 2);
	//if (size > 8 && strncmp((char*) fileData,"\x89PNG\r\n\x1A\n") == 0)
	//	image = LoadPNG(fileData,size);
	char* extension = strchr(str,'.');
	if (extension != NULL && strcmp(&extension[1],"raw") == 0){
		image = LoadRaw(fd,size);
		isRaw = 1;
	}
	else if (strncmp((char*) fileData,"BM",2) == 0){
		image = LoadBMP(fd,size);
	}
	IOS_Close( fd );
	free(fileData);
	if (image == NULL){
		dbgprintf("Failed to load: %s",str);
	}
	else if (isRaw == 0){
		char* newFile = (char*)malloca(128,32);
		strcpy(newFile,str);
		char* periodPlace = strchr(newFile,'.');
		if (periodPlace != NULL)
			strcpy(&periodPlace[1],"raw");
		else
			strcpy(&newFile[strlen(newFile)],".raw");
		NANDWriteFileSafe( newFile,image,sizeof(ImageStruct) + image->height * image->width / 2 * sizeof(u32));
		free(newFile);
	}
	return image;
}
