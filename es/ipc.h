#ifndef __IPC_H__
#define __IPC_H__	1

#define IPC_EINVAL -1

struct ioctl_vector {
	void *data;
	unsigned int len;
} __attribute__((packed));


/* IOCTL vector */
typedef struct iovec {
	void *data;
	u32   len;
} ioctlv;

struct ipcmessage
{
	unsigned int command;			// 0
	unsigned int result;			// 4
	union
	{
		unsigned int fd;			// 8
		u32 req_cmd;
	};
	union 
	{ 
		struct
	 	{
			char *device;			// 12
			unsigned int mode;		// 16
		} open;
	
		struct 
		{
			void *data;
			unsigned int length;
		} read, write;
		
		struct 
	 	{
			int offset;
			int origin;
		} seek;
		
		struct 
	 	{
			unsigned int command;		//12

			unsigned int *buffer_in;	//16
			unsigned int length_in;		//20
			unsigned int *buffer_io;	//24
			unsigned int length_io;		//28
		} ioctl;

		struct
		{
			unsigned int command;	//12

			unsigned int argc_in;	//16
			unsigned int argc_io;	//20
			struct ioctl_vector *argv;	//24
		} ioctlv;
		u32 args[5];	
	};

	ipccallback cb;		//32
	void *usrdata;		//36
	u32 relnch;			//40
	u32	syncqueue;		//44
	u32 magic;			//48 - used to avoid spurious responses, like from zelda.
	u8 pad1[12];		//52 - 60

} __attribute__((packed)) ipcmessage; 

#define IOS_OPEN				0x01
#define IOS_CLOSE				0x02
#define IOS_READ				0x03
#define IOS_WRITE				0x04
#define IOS_SEEK				0x05
#define IOS_IOCTL				0x06
#define IOS_IOCTLV				0x07
#endif
