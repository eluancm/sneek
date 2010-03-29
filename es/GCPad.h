
enum GCPad
{
	PAD_BUTTON_LEFT	=(1<<16),
	PAD_BUTTON_RIGHT=(1<<17),
	PAD_BUTTON_DOWN	=(1<<18),
	PAD_BUTTON_UP	=(1<<19),

	PAD_BUTTON_A	=(1<<24),
	PAD_BUTTON_B	=(1<<25),
	PAD_BUTTON_X	=(1<<26),
	PAD_BUTTON_Y	=(1<<27),
	PAD_BUTTON_START=(1<<28),
};

typedef struct
{
	union
	{
		struct 
		{
			bool ErrorStatus		:1;
			bool ErrorLatch			:1;
			u32	 Reserved			:1;
			bool Start				:1;

			bool Y					:1;
			bool X					:1;
			bool B					:1;
			bool A					:1;

			u32  AlwaysSet			:1;
			bool R					:1;
			bool L					:1;
			bool Z					:1;

			bool Up					:1;
			bool Down				:1;
			bool Left				:1;
			bool Right				:1;

			s16  StickX				:8;
			s16  StickY				:8;
		};
		u32 Buttons;
	};
	union 
	{
		struct 
		{
			s16  CStickX;
			s16  CStickY;
			s16  LShoulder;
			s16  RShoulder;	
		};
		u32 Sticks;
	};
} GCPadStatus;
