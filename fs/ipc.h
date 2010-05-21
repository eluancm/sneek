#ifndef __IPC_H__
#define __IPC_H__	1

#define IPC_EINVAL -1




/* IOCTL vector */
typedef struct iovec {
	void *data;
	u32   len;
} ioctlv;

typedef struct IPCMessage
{
	unsigned int command;			// 0
	unsigned int result;			// 4
	union
	{
		unsigned int fd;			// 8
	};
	union 
	{ 
		struct
	 	{
			char *device;			// 12
			unsigned int mode;		// 16
			unsigned int resultfd;	// 20
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
			unsigned int command;

			unsigned int *buffer_in;
			unsigned int length_in;
			unsigned int *buffer_io;
			unsigned int length_io;
		} ioctl;

		struct
		{
			unsigned int command;

			unsigned int argc_in;
			unsigned int argc_io;
			struct ioctl_vector *argv;
		} ioctlv;
	};
} __attribute__((packed)) IPCMessage; 

#define IOS_OPEN				0x01
#define IOS_CLOSE				0x02
#define IOS_READ				0x03
#define IOS_WRITE				0x04
#define IOS_SEEK				0x05
#define IOS_IOCTL				0x06
#define IOS_IOCTLV				0x07

#endif
