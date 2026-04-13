#include <REGX52.H>
#include <INTRINS.H>
#define  LCD_DATA   P0
sbit LCD1602_RS = P2^6;
sbit LCD1602_EN = P2^7;
sbit LCD1602_WR = P2^5;


void LCD1602_Delay1ms(void)	//@11.0592MHz
{
	unsigned char data i, j;

	_nop_();
	i = 2;
	j = 199;
	do
	{
		while (--j);
	} while (--i);
}


void LCD1602_WriteCmd(unsigned char Cmd)
{
	LCD1602_RS = 0;
	LCD1602_WR = 0;
	LCD1602_EN = 0;
	LCD_DATA = Cmd;
	LCD1602_EN = 1;
	LCD1602_Delay1ms();
	LCD1602_EN = 0;
	LCD1602_Delay1ms();
}

void LCD1602_WriteData(unsigned char Data)
{
	LCD1602_RS = 1;
	LCD1602_WR = 0;
	LCD1602_EN = 0;
	LCD_DATA = Data;
	LCD1602_EN = 1;
	LCD1602_Delay1ms();
	LCD1602_EN = 0;	
	LCD1602_Delay1ms();
}

void LCD1602_Init()
{
	LCD1602_WriteCmd(0x38);
	LCD1602_WriteCmd(0x0C);
	LCD1602_WriteCmd(0x06);
	LCD1602_WriteCmd(0x01);
}

// 用于设置位置，在哪里显示。
void Set_Position(unsigned char Row, unsigned char Col)
{
	if (Row == 1)
	{
		LCD1602_WriteCmd(0x80 | (Col - 1));  // 映射的位置的数据最高位固定为1，所以真实的地址应该要再或上0x80
	}
	else
	{
		LCD1602_WriteCmd((0x80 | (Col - 1)) + 0x40);
	}
}

// 在特定位置，显示 单个字节
void LCD_ShowChar(unsigned char Rows, unsigned char Cols, unsigned char Char)
{
	Set_Position(Rows, Cols);
	LCD1602_WriteData(Char);
	
}

// 在特定位置，显示 一个字符串
void LCD_ShowString(unsigned char Rows, unsigned char Cols, unsigned char String[])
{
	unsigned char i;
	Set_Position(Rows, Cols);
	for (i = 0; String[i] != '\0'; i++)
	{
		LCD1602_WriteData(String[i]);
	}
}



//123 / 100 % 10  = 1
//123 / 10  % 10  = 2
//123 / 1   % 10  = 3
// 用于计算某个数字的次方结果 
unsigned int SetNub(unsigned char Nub, Len)
{
	unsigned int NubValue = 1;
	unsigned char i;
	for (i = 0; i < (Len - 1); i++)
	{
		NubValue *= Nub;
	}
	return NubValue;
}

// 在特定位置，显示一个有符号数字(0 ~ 65535)
void LCD_ShowNub(unsigned char Rows, unsigned char Cols, unsigned int Nub, unsigned char Len)
{
	unsigned char i;
	Set_Position(Rows, Cols);
	for (i = Len; i > 0; i--)
	{
		LCD1602_WriteData(((Nub / SetNub(10, i) % 10) + 0x30)); // 加上0x30是将数字转换成字符。例如数字0的十六进制ASCLL码为0x00，但是字符0的十六进制ASCLL码为0x30，所以直接加0x30就可以完成转换了。
	}
}

// 在特定位置，显示一个有符号数字(-32768 ~ 32767)
void LCD_ShowSignedNub(unsigned char Rows, unsigned char Cols, int Nub, unsigned char Len)
{
	unsigned char i;
	Set_Position(Rows, Cols);
	if (Nub >= 0)
	{
		LCD1602_WriteData('+');
		for (i = Len; i > 0; i--)
		{
			LCD1602_WriteData(((Nub / SetNub(10, i) % 10) + 0x30)); // 加上0x30是将数字转换成字符。例如数字0的十六进制ASCLL码为0x00，但是字符0的十六进制ASCLL码为0x30，所以直接加0x30就可以完成转换了。
		}
	}
	else
	{
		LCD1602_WriteData('-');
		for (i = Len; i > 0; i--)
		{
			LCD1602_WriteData(((-(Nub) / SetNub(10, i) % 10) + 0x30)); // 加上0x30是将数字转换成字符。例如数字0的十六进制ASCLL码为0x00，但是字符0的十六进制ASCLL码为0x30，所以直接加0x30就可以完成转换了。
		}
	}
}

// 在特定位置，显示一个十六进制数 (0 ~ 0xFFFF)
void LCD_ShowHex(unsigned char Rows, unsigned char Cols, unsigned int Hex, unsigned char Len)
{
	unsigned char i, HexNub;
	Set_Position(Rows, Cols);
	
	for (i = Len; i > 0; i--)
	{
		HexNub = (Hex / SetNub(16, i)) % 16;
		if (HexNub < 10)  LCD1602_WriteData(HexNub + 0x30);  
		else LCD1602_WriteData(HexNub + 55);
	}
		
}

// 在特定位置，显示一个二进制数(0 ~ 1111 1111 1111 1111)
void LCD_ShowBin(unsigned char Rows, unsigned char Cols, unsigned int Bin, unsigned char Len)
{
	unsigned char i, BinNub;
	Set_Position(Rows, Cols);
	
	for (i = Len; i > 0; i--)
	{
		BinNub = (Bin / SetNub(2, i)) % 2;
		LCD1602_WriteData(BinNub + 0x30);
	}
		
}