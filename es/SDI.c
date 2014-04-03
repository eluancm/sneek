#include "SDI.h"

u32 *HCR;
u32 *SDStatus;		//0x00110001
u32 SDClock=0;

extern int FFSHandle;

void SD_Ioctl( struct ipcmessage *msg )
{
	u8  *bufin  = (u8*)msg->ioctl.buffer_in;
	//(void) u32 lenin   = msg->ioctl.length_in;
	u8  *bufout = (u8*)msg->ioctl.buffer_io;
	//(void) u32 lenout  = msg->ioctl.length_io;
	s32 ret = -1;

	switch( msg->ioctl.command )
	{
//#ifdef SDI
		case 0x01:	// Write HC Register
		{
			u32 reg = *(u32*)(bufin);
			u32 val = *(u32*)(bufin+16);

			if( (reg==0x2C) && (val==1) )
			{
				HCR[reg] = val | 2;
			} else if( (reg==0x2F) && val )
			{
				HCR[reg] = 0;
			} else {
				HCR[reg] = val;
			}

			ret = 0;
			dbgprintf("SD:SetHCRegister(%02X:%02X):%d\n", reg, HCR[reg], ret );
		} break;
		case 0x02:	// Read HC Register
		{
			*(u32*)(bufout) = HCR[*(u32*)(bufin)];

			ret = 0;
			dbgprintf("SD:GetHCRegister(%02X:%02X):%d\n", *(u32*)(bufin), HCR[*(u32*)(bufin)], ret );
		} break;
		case 0x04:	// sd_reset_card
		{
			*SDStatus |= 0x10000;
			*(u32*)(bufout) = 0x9f620000;

			ret = 0;
			dbgprintf("SD:Reset(%08x):%d\n", *(u32*)(bufout), ret);
		} break;
		case 0x06:
		{
			SDClock = *(u32*)(bufin);
			ret=0;
			dbgprintf("SD:SetClock(%d):%d\n", *(u32*)(bufin), ret );
		} break;
		case 0x07:
		{
			SDCommand *scmd = (SDCommand *)(bufin);
			memset32( bufout, 0, 0x10 );

			switch( scmd->command )
			{
				case 0:
					ret = 0;
					dbgprintf("SD:GoIdleState():%d\n", ret);
				break;
				case 3:
					*(u32*)(bufout) = 0x9f62;
					ret = 0;
					dbgprintf("SD:SendRelAddress():%d\n", ret);
				break;
				case SD_APP_SET_BUS_WIDTH:		// 6	ACMD_SETBUSWIDTH
				{
					*(u32*)(bufout) = 0x920;
					ret=0;
					dbgprintf("SD:SetBusWidth(%d):%d\n", scmd->arg, ret );
				} break;
				case MMC_SELECT_CARD:			// 7
				{
					if( scmd->arg >> 16 )
						*(u32*)(bufout) = 0x700;
					else
						*(u32*)(bufout) = 0x900;

					ret=0;
					dbgprintf("SD:SelectCard(%08x):%d\n", *(u32*)(bufout), ret);
				} break;
				case 8:
				{
					*(u32*)(bufout) = scmd->arg;
					ret=0;
					dbgprintf("SD:SendIFCond(%08x):%d\n", *(u32*)(bufout), ret);
				} break;
				case MMC_SEND_CSD:
				{
					*(u32*)(bufout)		= 0x80168000;
					*(u32*)(bufout+4)	= 0xa9ffffff;
					*(u32*)(bufout+8)	= 0x325b5a83;
					*(u32*)(bufout+12)	= 0x00002e00;
					ret=0;
					dbgprintf("SD:SendCSD():%d\n", ret );
				} break;
				case MMC_ALL_SEND_CID:	// 2
				case 10:				// SEND_CID
				{
					*(u32*)(bufout)		= 0x80114d1c;
					*(u32*)(bufout+4)	= 0x80080000;
					*(u32*)(bufout+8)	= 0x8007b520;
					*(u32*)(bufout+12)	= 0x80080000;			
					ret=0;
					dbgprintf("SD:SendCID():%d\n", ret );
				} break;
				case MMC_SET_BLOCKLEN:	// 16 0x10
				{
					*(u32*)(bufout) = 0x900;
					ret=0;
					dbgprintf("SD:SetBlockLen(%d):%d\n", scmd->arg, ret );
				} break;
				case MMC_APP_CMD:	// 55 0x37
				{
					*(u32*)(bufout) = 0x920;
					ret=0;
					dbgprintf("SD:AppCMD(%08x):%d\n", *(u32*)(bufout), ret);
				} break;
				case SDHC_CAPABILITIES:
				{
					//0x01E130B0
					if( (*SDStatus)&0x10000 )
						*(u32*)(bufout) = (1 << 7) | (63 << 8);
					else
						*(u32*)(bufout) = 0;

					ret=0;
					dbgprintf("SD:GetCapabilities():%d\n", ret );
				} break;
				case 0x41:
				{
					*SDStatus &= ~0x10000;
					ret=0;
					dbgprintf("SD:Unmount(%02X):%d\n", scmd->command, ret );
				} break;
				case 4:
				{
					ret=0;
					dbgprintf("SD:Command(%02X):%d\n", scmd->command, ret );
				} break;
				case 12:	// STOP_TRANSMISSION
				{
					dbgprintf("SD:StopTransmission()\n");
				} break;
				case SDHC_POWER_CTL:
				{
					*(u32*)(bufout) = 0x80ff8000;
					ret=0;
					dbgprintf("SD:SendOPCond(%04X):%d\n", *(u32*)(bufout), ret);
				} break;
				case 0x19:	// CMD25 WRITE_MULTIPLE_BLOCK
				{
					vector *vec = (vector *)malloca( sizeof(vector), 0x40 );

					vec[0].data = (u32)bufin;
					vec[0].len = sizeof(SDCommand);

					ret = IOS_Ioctlv( FFSHandle, 0x21, 1, 0, vec );

					if( ret < 0 )
					{
						SDCommand *scmd = (SDCommand *)bufin;
						dbgprintf("SD:WriteMultipleBlock( 0x%p, 0x%x, 0x%x):%d\n", scmd->addr, scmd->arg, scmd->blocks, ret );
					}
					free( vec );

				} break;
				default:
				{
					//dbgprintf("Command:%08X\n", *(u32*)(bufin) );
					//dbgprintf("CMDType:%08X\n", *(u32*)(bufin+4) );
					//dbgprintf("ResType:%08X\n", *(u32*)(bufin+8) );
					//dbgprintf("Argumen:%08X\n", *(u32*)(bufin+0x0C) );
					//dbgprintf("BlockCn:%08X\n", *(u32*)(bufin+0x10) );
					//dbgprintf("SectorS:%08X\n", *(u32*)(bufin+0x14) );
					//dbgprintf("Buffer :%08X\n", *(u32*)(bufin+0x18) );
					//dbgprintf("unkown :%08X\n", *(u32*)(bufin+0x1C) );
					//dbgprintf("unknown:%08X\n", *(u32*)(bufin+0x20) );

					//dbgprintf("Unhandled command!\n");
				} break;
			}

			//dbgprintf("SD:Command(0x%02X):%d\n", *(u32*)(bufin), ret );
		} break;
		case 0x0B:	// sd_get_status
		{
			*(u32*)(bufout) = *SDStatus;
			ret = 0;
		//	dbgprintf("SD:GetStatus(%08X):%d\n", *(u32*)(bufout), ret);
		} break;
		case 0x0C:	//OCRegister
		{
			*(u32*)(bufout) = 0x80ff8000;
			ret = 0;
		//	dbgprintf("SD:GetOCRegister(%08X):%d\n", *(u32*)(bufout), ret);
		} break;
//#endif
		default:
			ret = -1;
		//	dbgprintf("SD:IOS_Ioctl( %d 0x%x 0x%p 0x%x 0x%p 0x%x )\n", msg->fd, msg->ioctl.command, bufin, lenin, bufout, lenout);
			break;
	}

	mqueue_ack( (void *)msg, ret);
}
void SD_Ioctlv( struct ipcmessage *msg )
{
	//u32 InCount		= msg->ioctlv.argc_in;
	//u32 OutCount	= msg->ioctlv.argc_io;
	vector *v		= (vector*)(msg->ioctlv.argv);
	s32 ret=-1017;
	//u32 i;

	switch(msg->ioctl.command)
	{
//#ifdef SDI
		case 0x07:
		{
			ret = IOS_Ioctlv( FFSHandle, 0x20, msg->ioctlv.argc_in, msg->ioctlv.argc_io, msg->ioctlv.argv );
			memset32( (u32*)(v[2].data), 0, v[2].len );

			if( ret < 0 )
			{
				//SDCommand *scmd = (SDCommand *)(v[0].data);
				//dbgprintf("SD:ReadMultipleBlocks( 0x%p, 0x%x, 0x%x):%d\n", scmd->addr, scmd->arg, scmd->blocks, ret );

				//dbgprintf("cmd    :%08X\n", scmd->command );
				//dbgprintf("type   :%08X\n", scmd->type );
				//dbgprintf("resp   :%08X\n", scmd->rep );
				//dbgprintf("arg    :%08X\n", scmd->arg );
				//dbgprintf("blocks :%08X\n", scmd->blocks );
				//dbgprintf("bsize  :%08X\n", scmd->bsize );
				//dbgprintf("addr   :%08X\n", scmd->addr );
				//dbgprintf("isDMA  :%08X\n", scmd->isDMA );
			}
		} break;
//#endif
		default:
			//for( i=0; i<InCount+OutCount; ++i)
			//{
			//	dbgprintf("data:%p len:%d(0x%X)\n", v[i].data, v[i].len, v[i].len );
			//}
			//dbgprintf("SD:IOS_Ioctlv( %d 0x%x %d %d 0x%p )\n", msg->fd, msg->ioctlv.command, msg->ioctlv.argc_in, msg->ioctlv.argc_io, msg->ioctlv.argv);
			//while(1);
		break;
	}

	mqueue_ack( (void *)msg, ret);

}