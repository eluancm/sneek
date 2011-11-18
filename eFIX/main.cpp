#define _CRT_SECURE_NO_WARNINGS

#include <stdio.h>
#include <string.h>
#include "ELF.h"

unsigned int s32( unsigned int i )
{
	return (i>>24) | (i<<24) | ((i>>8)&0xFF00) | ((i<<8)&0xFF0000);
}
unsigned short s16( unsigned short s )
{
	return (s>>8) | (s<<8);
}

typedef unsigned int u32;

struct IOSHeaderInfo
{
	u32 Unknown;
	u32 EntryCount;
	u32 ModuleID;
	u32 Unknown2;
	u32 ThreadArg;
	u32 Unknown3;
	u32 Entrypoint;
	u32 Unknown4;
	u32 ThreadPrio;
	u32 Unknown5;
	u32 ThreadStackSize;
	u32 Unknown6;
	u32 Unknown7;
};

int main( int argc, char *argv[] )
{
	if( argc != 2 )
	{
		printf("eFix v 0.1 by crediar\n");
		printf("Usage:\n");
		printf("\teFix.exe DIModule.elf\n\n");
		return -1;
	}

	FILE *in = fopen(argv[1], "rb" );
	if( in == NULL )
	{
		printf("Could not open \"%s\"\n", argv[1] );
		perror("");
		return -1;
	}

	fseek( in, 0, SEEK_END);

	unsigned int size = ftell(in);

	char *buf = new char[size];

	fseek( in, 0, 0 );
	fread( buf, 1, size, in );
	fclose( in );
	
	Elf32_Ehdr *inhdr = (Elf32_Ehdr*)buf;

	if( inhdr->e_ident[EI_MAG0] != 0x7F ||
		inhdr->e_ident[EI_MAG1] != 'E' ||
		inhdr->e_ident[EI_MAG2] != 'L' ||
		inhdr->e_ident[EI_MAG3] != 'F' )
	{
		printf("\"%s\" is not a valid ELF!\n", argv[1] );
		return -2;
	}


	//Prepare new ELF header, output will have SIX program headers

#define PHCOUNT	6

	Elf32_Ehdr OEHdr;
	Elf32_Ehdr *IEHdr = (Elf32_Ehdr *)buf;

	memset( &OEHdr, 0, sizeof(Elf32_Ehdr) );
	
	OEHdr.e_ident[EI_MAG0]	= 0x7F;
	OEHdr.e_ident[EI_MAG1]	= 'E';
	OEHdr.e_ident[EI_MAG2]	= 'L';
	OEHdr.e_ident[EI_MAG3]	= 'F';
	
	OEHdr.e_ident[EI_CLASS]	 = 0x01;
	OEHdr.e_ident[EI_DATA]	 = 0x02;
	OEHdr.e_ident[EI_VERSION]= 0x01;
	OEHdr.e_ident[EI_PAD]	 = 0x61;
	OEHdr.e_ident[8]		 = 0x01;

	OEHdr.e_type		= s16(0x0002);
	OEHdr.e_machine		= s16(0x0028);
	OEHdr.e_version		= s32(0x0001);
	OEHdr.e_entry		= s32(0x20200000);
	
	OEHdr.e_flags		= s32(0x00000606);
	OEHdr.e_ehsize		= s16(sizeof(Elf32_Ehdr));
	
	OEHdr.e_phentsize	= s16(sizeof(Elf32_Phdr));
	OEHdr.e_phoff		= s32(sizeof(Elf32_Ehdr));
	OEHdr.e_phnum		= s16(PHCOUNT);

	Elf32_Phdr OPHdr[PHCOUNT];
	memset( &OPHdr, 0, sizeof(Elf32_Phdr) * PHCOUNT );

//Special Headers
	OPHdr[0].p_type		= s32(6);
	OPHdr[0].p_offset	= s32(0x34);
	OPHdr[0].p_vaddr	= 0;
	OPHdr[0].p_paddr	= 0;
	OPHdr[0].p_flags	= s32(0xF00000);
	OPHdr[0].p_filesz	= s32(0xC0);
	OPHdr[0].p_memsz	= s32(0xC0);
	OPHdr[0].p_align	= s32(4);

	OPHdr[1].p_type		= s32(4);
	OPHdr[1].p_offset	= s32(0xF4);
	OPHdr[1].p_vaddr	= s32(0xC0);
	OPHdr[1].p_paddr	= s32(0xC0);
	OPHdr[1].p_filesz	= s32(0x34);
	OPHdr[1].p_memsz	= s32(0x34);
	OPHdr[1].p_flags	= s32(0xF00000);
	OPHdr[1].p_align	= s32(4);

	OPHdr[2].p_type		= s32(1);
	OPHdr[2].p_offset	= s32(0x34);
	OPHdr[2].p_vaddr	= 0;
	OPHdr[2].p_paddr	= 0;
	OPHdr[2].p_filesz	= s32(0xF4);
	OPHdr[2].p_memsz	= s32(0xF4);
	OPHdr[2].p_flags	= s32(0xF00000);
	OPHdr[2].p_align	= s32(4);
	
	FILE *out = fopen("di.bin", "wb");
//Real headers

	fwrite( &OEHdr, sizeof(Elf32_Ehdr), 1, out );

	fseek( out, 0x128, SEEK_SET );

	for( int i=0; i < 3; ++i )
	{
		Elf32_Phdr *IPHdr = (Elf32_Phdr *) ( buf + s32(IEHdr->e_phoff) + sizeof(Elf32_Phdr) * i );
		
		printf("Type:%X Off:%06X VAdr:%08X PAdr:%08X FSz:%05X MSz:%05X Flags:%02X A:%d\n", s32(IPHdr->p_type), s32(IPHdr->p_offset), s32(IPHdr->p_vaddr), s32(IPHdr->p_paddr), s32(IPHdr->p_filesz), s32(IPHdr->p_memsz), s32(IPHdr->p_flags), s32(IPHdr->p_align) );
		

		if( s32(IPHdr->p_vaddr) == 0x20209000 )
			IPHdr->p_memsz = s32(0x02BDC4);

		memcpy( OPHdr + i + 3, IPHdr, sizeof( Elf32_Phdr ) );
		
		OPHdr[i+3].p_flags |= s32(0x300000);
		OPHdr[i+3].p_offset = s32(ftell(out));

		fwrite( buf + s32(IPHdr->p_offset), sizeof(char), s32(IPHdr->p_filesz), out );
	}

 	fseek( out, s32(OEHdr.e_phoff), SEEK_SET );
	fwrite( OPHdr, sizeof( Elf32_Phdr ), s16(OEHdr.e_phnum), out );

	IOSHeaderInfo IOSHdr;

	memset( &IOSHdr, 0, sizeof(IOSHeaderInfo) );

	IOSHdr.Unknown			= s32(0);
	IOSHdr.EntryCount		= s32(0x28);
	IOSHdr.ModuleID			= s32(6);
	IOSHdr.Unknown2			= s32(0x0B);
	IOSHdr.ThreadArg		= s32(3);
	IOSHdr.Unknown3			= s32(0x09);
	IOSHdr.Entrypoint		= s32(0x20200000);
	IOSHdr.Unknown4			= s32(0x7D);
	IOSHdr.ThreadPrio		= s32(0x54);
	IOSHdr.Unknown5			= s32(0x7E);
	IOSHdr.ThreadStackSize	= s32(0x8000);
	IOSHdr.Unknown6			= s32(0x7F);
	IOSHdr.Unknown7			= s32(0x20229000);

	fseek( out, 0xF4, SEEK_SET );
	fwrite( &IOSHdr, sizeof( IOSHeaderInfo ), 1, out ); 

	fclose(out);
	delete buf;

	return 0;
}