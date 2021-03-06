#include "lcd_12864.h"

static unsigned char ascii_table_8x16[];

//#pragma optimize=none
//Transfer_command(0xa7);		//reverse



void LCD_GPIO_Configuration(void)
{
    GPIO_InitTypeDef GPIO_InitStructure;

    GPIO_InitStructure.GPIO_Pin = GPIO_Pin_1|GPIO_Pin_2|GPIO_Pin_3|GPIO_Pin_4|GPIO_Pin_5|GPIO_Pin_6;
    GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;
    GPIO_InitStructure.GPIO_Mode = GPIO_Mode_Out_PP;
    GPIO_Init(GPIOE, &GPIO_InitStructure);

    GPIO_InitStructure.GPIO_Pin = GPIO_Pin_9;
    GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;
    GPIO_InitStructure.GPIO_Mode = GPIO_Mode_IN_FLOATING;
    GPIO_Init(GPIOB, &GPIO_InitStructure);

    GPIO_InitStructure.GPIO_Pin = (GPIO_Pin_6 | GPIO_Pin_7 | GPIO_Pin_8 );
    GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;
    GPIO_InitStructure.GPIO_Mode = GPIO_Mode_Out_PP;
    GPIO_Init(GPIOB, &GPIO_InitStructure);
}



void lcd_sleep()
{
    Transfer_command(0xac);		/*Static indicator OFF*/
    Transfer_command(0xae);		/*Display OFF*/
    Transfer_command(0xa5);		/*Display all point ON*/
}



void lcd_sleep_cancel()
{
    Transfer_command(0xa4);		/*Display all point OFF*/
    Transfer_command(0xaf);		/*Display ON*/
    Transfer_command(0xad);		/*Static indicator ON*/
}



void display_hex(const uint8 *data,uint8 count,uint8 y,uint8 x)
{
    uint8 digit;
    const unsigned char *fontbuf;
    int i=0;

    if(x+count*16>=128)
        count=(128-x)/16;


    for(; i<count;)
    {
        digit=data[i]>>4;

        if(digit<0xA)
            fontbuf=ascii_table_8x16+(digit+ 0x10)*16;
        else
            fontbuf=ascii_table_8x16+(digit+ 0x17)*16;

        display_graphic_8x16(y,x,fontbuf);
        x+=8;

        digit=data[i]&0x0F;

        if(digit<0xA)
            fontbuf=ascii_table_8x16+(digit+ 0x10)*16;
        else
            fontbuf=ascii_table_8x16+(digit+ 0x17)*16;

        display_graphic_8x16(y,x,fontbuf);
        i+=1;
        x+=8;
    }
}

uint8 display_int(int32 num_,uint8 y,uint8 x,uint8 pt)
{
    uint8 dp[15]="";
    uint8 i,count,start=0;
    int32 dig=num_;

    if(num_<0)
    {
        dp[0]='-';
        start=1;
        dig=-num_;
    }

    if(dig<10L)
        count=1;
    else if(dig<100L)
        count=2;
    else if(dig<1000L)
        count=3;
    else if(dig<10000L)
        count=4;
    else if(dig<100000L)
        count=5;
    else if(dig<1000000L)
        count=6;
    else if(dig<10000000L)
        count=7;
    else if(dig<100000000L)
        count=8;
    else if(dig<1000000000L)
        count=9;
    else
        count=10;

    if(pt==0)
    {
        dp[start]='0';
        dp[start+1]='.';
        start+=2;
    }
    else if(pt>=count)
        pt=0;

    if(pt==0)
    {
        for(i=0; i<count; i++)
        {
            dp[count-i+start-1]=(dig%10)+'0';
            dig/=10;
        }
    }
    else
    {
        //count;
        for(i=0; i<count-pt; i++)
        {
            dp[count-i+start]=(dig%10)+'0';
            dig/=10;
        }

        dp[pt+start]='.';

        for(; i<count; i++)
        {
            dp[count-i+start-1]=(dig%10)+'0';
            dig/=10;
        }

        start++;
    }

    display_ascii(dp,start+i,y,x);
    return start+i;
}



void display_ascii(const uint8 *text,uint8 count,uint8 y,uint8 x)
{
    uint8 *dp;

    //const unsigned char *fontbuf;
    uint8 i,j;
    uint8 page_address;
    uint8 column_address_L,column_address_H;

    if(x+count*8>=128)
        count=(128-x)/8;

    page_address = 0xB0+y;
    column_address_L =x&0x0f;
    column_address_H =((x>>4)&0x0f)+0x10;
    Rom_CS_1;
    cs1_0;
    Transfer_command(page_address);     /*设置页地址*/
    Transfer_command(column_address_H); /*设置列地址的高 4 位*/
    Transfer_command(column_address_L); /*设置列地址的低 4 位*/

    for (j=0; j<count; j++)
    {
        dp=ascii_table_8x16+(text[j]- 0x20)*16;
        for (i=0; i<8; i++)
        {
            Transfer_data(*(dp+i)); //写数据到 LCD,每写完一个 8 位的数据后列地址自动加 1
        }
    }

    Transfer_command(page_address+1);   /*设置页地址*/
    Transfer_command(column_address_H); /*设置列地址的高 4 位*/
    Transfer_command(column_address_L); /*设置列地址的低 4 位*/

    for (j=0; j<count; j++)
    {
        dp=ascii_table_8x16+(text[j]- 0x20)*16+8;

        for (i=0; i<8; i++)
        {
            Transfer_data(*(dp+i));     //写数据到 LCD,每写完一个 8 位的数据后列地址自动加 1
        }
    }
    cs1_1;
}



void display_GB2312_char(uint8 y,uint8 x,uint8 code_h,uint8 code_l)
{
    uint32  fontaddr=0;
    uint8   addrHigh,addrMid,addrLow;
    uint8   fontbuf[32];

    if(((code_h>=0xb0) &&(code_h<0xf7))&&(code_l>=0xa1))
    {
        fontaddr =  (code_h- 0xb0)*94;
        fontaddr += (code_l-0xa1)+846;
        fontaddr =  (fontaddr<<5);
        addrHigh =  (fontaddr>>16)&0xff;                                /*地址的高 8 位,共 24 位*/
        addrMid  =  (fontaddr>>8)&0xff;                                 /*地址的中 8 位,共 24 位*/
        addrLow  =  fontaddr&0xff;                                      /*地址的低 8 位,共 24 位*/
        get_n_bytes_data_from_ROM(addrHigh,addrMid,addrLow,fontbuf,32 );/*取 32 个字节的数据，存到"fontbuf[32]"*/
        display_graphic_16x16(y,x,fontbuf);                             /*显示汉字到 LCD 上，y 为页地址，x 为列地址，
		                                                                  fontbuf[]为数据*/
    }
    else if(((code_h>0xA0) &&(code_h<=0xA9))&&(code_l>=0xA1))
    {
        unsigned char fontbuf[32];
        fontaddr = (code_h- 0xA1)*94;
        fontaddr = code_l- 0xA1;
        fontaddr = fontaddr<<5;
        addrHigh = (fontaddr>>16)&0xff;
        addrMid  = (fontaddr>>8)&0xff;
        addrLow  = fontaddr&0xff;
        get_n_bytes_data_from_ROM(addrHigh,addrMid,addrLow,fontbuf,32 );
        display_graphic_16x16(y,x,fontbuf);
    }
    else if((code_h>=0x20) &&(code_h<=0x7e))
    {
        unsigned char *fontbuf;
        fontbuf=ascii_table_8x16+(code_h- 0x20)*16;
        display_graphic_8x16(y,x,fontbuf);

        if((code_l>=0x20) &&(code_l<=0x7e))
        {
            fontbuf=ascii_table_8x16+(code_l - 0x20)*16;
            display_graphic_8x16(y,x+8,fontbuf);
        }
    }
}



void display_GB2312_string(uint8 y,uint8 x,const uint8 *text)
{
    uint32  fontaddr=0;
    uint8   i= 0;
    uint8   addrHigh,addrMid,addrLow ;
    uint8   fontbuf[32];

    while((text[i]>0x00)&&x<128)
    {
        if(((text[i]>=0xb0) &&(text[i]<0xf7))&&(text[i+1]>=0xa1))
        {
            /*国标简体（GB2312）汉字在高通字库 IC 中的地址由以下公式来计算：*/
            /*Address = ((MSB - 0xB0) * 94 + (LSB - 0xA1)+ 846)*32+ BaseAdd;BaseAdd=0*/
            /*由于担心 8 位单片机有乘法溢出问题，所以分三部取地址*/
            fontaddr =  (text[i]- 0xb0)*94;
            fontaddr += (text[i+1]-0xa1)+846;
            fontaddr =  (fontaddr<<5);
            addrHigh =  (fontaddr>>16)&0xff;                                /*地址的高 8 位,共 24 位*/
            addrMid  =  (fontaddr>>8)&0xff;                                 /*地址的中 8 位,共 24 位*/
            addrLow  =  fontaddr&0xff;                                      /*地址的低 8 位,共 24 位*/
            get_n_bytes_data_from_ROM(addrHigh,addrMid,addrLow,fontbuf,32 );/*取 32 个字节的数据，存到"fontbuf[32]"*/
            display_graphic_16x16(y,x,fontbuf);                             /*显示汉字到 LCD 上，y 为页地址，x 为列地址，
			                                                                  fontbuf[]为数据*/
            i+=2;
            x+=16;
        }
        else if(((text[i]>0xA0) &&(text[i]<=0xA9))&&(text[i+1]>=0xA1))
        {
            unsigned char fontbuf[32];
            fontaddr = (text[i]- 0xA1)*94;
            fontaddr = text[i+1]- 0xA1;
            fontaddr = fontaddr<<5;
            addrHigh = (fontaddr>>16)&0xff;
            addrMid  = (fontaddr>>8)&0xff;
            addrLow  = fontaddr&0xff;
            get_n_bytes_data_from_ROM(addrHigh,addrMid,addrLow,fontbuf,32 );
            display_graphic_16x16(y,x,fontbuf);
            i+=2;
            x+=16;
        }
        else if((text[i]>=0x20) &&(text[i]<=0x7e))
        {
            unsigned char *fontbuf;
            fontbuf=ascii_table_8x16+(text[i]- 0x20)*16;
            display_graphic_8x16(y,x,fontbuf);
            i+=1;
            x+=8;
        }
        else
            i++;
    }
}



/*显示 8x16 点阵图像、ASCII, 或 8x16 点阵的自造字符、其他图标*/
void display_graphic_8x16(uint8 page,uint8 column,const uint8 *dp)
{
    uint8 i,j;
    uint8 page_address;
    uint8 column_address_L,column_address_H;
    page_address     = 0xB0+page;
    column_address_L = column&0x0f;
    column_address_H = ((column>>4)&0x0f)+0x10;
    cs1_0;
    Rom_CS_1;

    for(j=0; j<2; j++)
    {
        Transfer_command(page_address+j);   /*设置页地址*/
        Transfer_command(column_address_H); /*设置列地址的高 4 位*/
        Transfer_command(column_address_L); /*设置列地址的低 4 位*/

        for (i=0; i<8; i++)
        {
            Transfer_data(*dp);             //写数据到 LCD,每写完一个 8 位的数据后列地址自动加 1
            dp++;
        }
    }

    cs1_1;
}



uint8 ascii_table_8x16[]=
{

    /*--  文字:     --*/
    /*--  Comic Sans MS12;  此字体下对应的点阵为：宽x高=8x16   --*/
    0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,

    /*--  文字:  !  --*/
    /*--  Comic Sans MS12;  此字体下对应的点阵为：宽x高=8x16   --*/
    0x00,0x00,0x00,0xF8,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x33,0x30,0x00,0x00,0x00,

    /*--  文字:  "  --*/
    /*--  Comic Sans MS12;  此字体下对应的点阵为：宽x高=8x16   --*/
    0x00,0x10,0x0C,0x06,0x10,0x0C,0x06,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,

    /*--  文字:  #  --*/
    /*--  Comic Sans MS12;  此字体下对应的点阵为：宽x高=8x16   --*/
    0x40,0xC0,0x78,0x40,0xC0,0x78,0x40,0x00,0x04,0x3F,0x04,0x04,0x3F,0x04,0x04,0x00,

    /*--  文字:  $  --*/
    /*--  Comic Sans MS12;  此字体下对应的点阵为：宽x高=8x16   --*/
    0x00,0x70,0x88,0xFC,0x08,0x30,0x00,0x00,0x00,0x18,0x20,0xFF,0x21,0x1E,0x00,0x00,

    /*--  文字:  %  --*/
    /*--  Comic Sans MS12;  此字体下对应的点阵为：宽x高=8x16   --*/
    0xF0,0x08,0xF0,0x00,0xE0,0x18,0x00,0x00,0x00,0x21,0x1C,0x03,0x1E,0x21,0x1E,0x00,

    /*--  文字:  &  --*/
    /*--  Comic Sans MS12;  此字体下对应的点阵为：宽x高=8x16   --*/
    0x00,0xF0,0x08,0x88,0x70,0x00,0x00,0x00,0x1E,0x21,0x23,0x24,0x19,0x27,0x21,0x10,

    /*--  文字:  '  --*/
    /*--  Comic Sans MS12;  此字体下对应的点阵为：宽x高=8x16   --*/
    0x10,0x16,0x0E,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,

    /*--  文字:  (  --*/
    /*--  Comic Sans MS12;  此字体下对应的点阵为：宽x高=8x16   --*/
    0x00,0x00,0x00,0xE0,0x18,0x04,0x02,0x00,0x00,0x00,0x00,0x07,0x18,0x20,0x40,0x00,

    /*--  文字:  )  --*/
    /*--  Comic Sans MS12;  此字体下对应的点阵为：宽x高=8x16   --*/
    0x00,0x02,0x04,0x18,0xE0,0x00,0x00,0x00,0x00,0x40,0x20,0x18,0x07,0x00,0x00,0x00,

    /*--  文字:  *  --*/
    /*--  Comic Sans MS12;  此字体下对应的点阵为：宽x高=8x16   --*/
    0x40,0x40,0x80,0xF0,0x80,0x40,0x40,0x00,0x02,0x02,0x01,0x0F,0x01,0x02,0x02,0x00,

    /*--  文字:  +  --*/
    /*--  Comic Sans MS12;  此字体下对应的点阵为：宽x高=8x16   --*/
    0x00,0x00,0x00,0xF0,0x00,0x00,0x00,0x00,0x01,0x01,0x01,0x1F,0x01,0x01,0x01,0x00,

    /*--  文字:  ,  --*/
    /*--  Comic Sans MS12;  此字体下对应的点阵为：宽x高=8x16   --*/
    0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x80,0xB0,0x70,0x00,0x00,0x00,0x00,0x00,

    /*--  文字:  -  --*/
    /*--  Comic Sans MS12;  此字体下对应的点阵为：宽x高=8x16   --*/
    0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x01,0x01,0x01,0x01,0x01,0x01,0x01,

    /*--  文字:  .  --*/
    /*--  Comic Sans MS12;  此字体下对应的点阵为：宽x高=8x16   --*/
    0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x30,0x30,0x00,0x00,0x00,0x00,0x00,

    /*--  文字:  /  --*/
    /*--  Comic Sans MS12;  此字体下对应的点阵为：宽x高=8x16   --*/
    0x00,0x00,0x00,0x00,0x80,0x60,0x18,0x04,0x00,0x60,0x18,0x06,0x01,0x00,0x00,0x00,

    /*--  文字:  0  --*/
    /*--  Comic Sans MS12;  此字体下对应的点阵为：宽x高=8x16   --*/
    0x00,0xE0,0x10,0x08,0x08,0x10,0xE0,0x00,0x00,0x0F,0x10,0x20,0x20,0x10,0x0F,0x00,

    /*--  文字:  1  --*/
    /*--  Comic Sans MS12;  此字体下对应的点阵为：宽x高=8x16   --*/
    0x00,0x10,0x10,0xF8,0x00,0x00,0x00,0x00,0x00,0x20,0x20,0x3F,0x20,0x20,0x00,0x00,

    /*--  文字:  2  --*/
    /*--  Comic Sans MS12;  此字体下对应的点阵为：宽x高=8x16   --*/
    0x00,0x70,0x08,0x08,0x08,0x88,0x70,0x00,0x00,0x30,0x28,0x24,0x22,0x21,0x30,0x00,

    /*--  文字:  3  --*/
    /*--  Comic Sans MS12;  此字体下对应的点阵为：宽x高=8x16   --*/
    0x00,0x30,0x08,0x88,0x88,0x48,0x30,0x00,0x00,0x18,0x20,0x20,0x20,0x11,0x0E,0x00,

    /*--  文字:  4  --*/
    /*--  Comic Sans MS12;  此字体下对应的点阵为：宽x高=8x16   --*/
    0x00,0x00,0xC0,0x20,0x10,0xF8,0x00,0x00,0x00,0x07,0x04,0x24,0x24,0x3F,0x24,0x00,

    /*--  文字:  5  --*/
    /*--  Comic Sans MS12;  此字体下对应的点阵为：宽x高=8x16   --*/
    0x00,0xF8,0x08,0x88,0x88,0x08,0x08,0x00,0x00,0x19,0x21,0x20,0x20,0x11,0x0E,0x00,

    /*--  文字:  6  --*/
    /*--  Comic Sans MS12;  此字体下对应的点阵为：宽x高=8x16   --*/
    0x00,0xE0,0x10,0x88,0x88,0x18,0x00,0x00,0x00,0x0F,0x11,0x20,0x20,0x11,0x0E,0x00,

    /*--  文字:  7  --*/
    /*--  Comic Sans MS12;  此字体下对应的点阵为：宽x高=8x16   --*/
    0x00,0x38,0x08,0x08,0xC8,0x38,0x08,0x00,0x00,0x00,0x00,0x3F,0x00,0x00,0x00,0x00,

    /*--  文字:  8  --*/
    /*--  Comic Sans MS12;  此字体下对应的点阵为：宽x高=8x16   --*/
    0x00,0x70,0x88,0x08,0x08,0x88,0x70,0x00,0x00,0x1C,0x22,0x21,0x21,0x22,0x1C,0x00,

    /*--  文字:  9  --*/
    /*--  Comic Sans MS12;  此字体下对应的点阵为：宽x高=8x16   --*/
    0x00,0xE0,0x10,0x08,0x08,0x10,0xE0,0x00,0x00,0x00,0x31,0x22,0x22,0x11,0x0F,0x00,

    /*--  文字:  :  --*/
    /*--  Comic Sans MS12;  此字体下对应的点阵为：宽x高=8x16   --*/
    0x00,0x00,0x00,0xC0,0xC0,0x00,0x00,0x00,0x00,0x00,0x00,0x30,0x30,0x00,0x00,0x00,

    /*--  文字:  ;  --*/
    /*--  Comic Sans MS12;  此字体下对应的点阵为：宽x高=8x16   --*/
    0x00,0x00,0x00,0x80,0x00,0x00,0x00,0x00,0x00,0x00,0x80,0x60,0x00,0x00,0x00,0x00,

    /*--  文字:  <  --*/
    /*--  Comic Sans MS12;  此字体下对应的点阵为：宽x高=8x16   --*/
    0x00,0x00,0x80,0x40,0x20,0x10,0x08,0x00,0x00,0x01,0x02,0x04,0x08,0x10,0x20,0x00,

    /*--  文字:  =  --*/
    /*--  Comic Sans MS12;  此字体下对应的点阵为：宽x高=8x16   --*/
    0x40,0x40,0x40,0x40,0x40,0x40,0x40,0x00,0x04,0x04,0x04,0x04,0x04,0x04,0x04,0x00,

    /*--  文字:  >  --*/
    /*--  Comic Sans MS12;  此字体下对应的点阵为：宽x高=8x16   --*/
    0x00,0x08,0x10,0x20,0x40,0x80,0x00,0x00,0x00,0x20,0x10,0x08,0x04,0x02,0x01,0x00,

    /*--  文字:  ?  --*/
    /*--  Comic Sans MS12;  此字体下对应的点阵为：宽x高=8x16   --*/
    0x00,0x70,0x48,0x08,0x08,0x08,0xF0,0x00,0x00,0x00,0x00,0x30,0x36,0x01,0x00,0x00,

    /*--  文字:  @  --*/
    /*--  Comic Sans MS12;  此字体下对应的点阵为：宽x高=8x16   --*/
    0xC0,0x30,0xC8,0x28,0xE8,0x10,0xE0,0x00,0x07,0x18,0x27,0x24,0x23,0x14,0x0B,0x00,

    /*--  文字:  A  --*/
    /*--  Comic Sans MS12;  此字体下对应的点阵为：宽x高=8x16   --*/
    0x00,0x00,0xC0,0x38,0xE0,0x00,0x00,0x00,0x20,0x3C,0x23,0x02,0x02,0x27,0x38,0x20,

    /*--  文字:  B  --*/
    /*--  Comic Sans MS12;  此字体下对应的点阵为：宽x高=8x16   --*/
    0x08,0xF8,0x88,0x88,0x88,0x70,0x00,0x00,0x20,0x3F,0x20,0x20,0x20,0x11,0x0E,0x00,

    /*--  文字:  C  --*/
    /*--  Comic Sans MS12;  此字体下对应的点阵为：宽x高=8x16   --*/
    0xC0,0x30,0x08,0x08,0x08,0x08,0x38,0x00,0x07,0x18,0x20,0x20,0x20,0x10,0x08,0x00,

    /*--  文字:  D  --*/
    /*--  Comic Sans MS12;  此字体下对应的点阵为：宽x高=8x16   --*/
    0x08,0xF8,0x08,0x08,0x08,0x10,0xE0,0x00,0x20,0x3F,0x20,0x20,0x20,0x10,0x0F,0x00,

    /*--  文字:  E  --*/
    /*--  Comic Sans MS12;  此字体下对应的点阵为：宽x高=8x16   --*/
    0x08,0xF8,0x88,0x88,0xE8,0x08,0x10,0x00,0x20,0x3F,0x20,0x20,0x23,0x20,0x18,0x00,

    /*--  文字:  F  --*/
    /*--  Comic Sans MS12;  此字体下对应的点阵为：宽x高=8x16   --*/
    0x08,0xF8,0x88,0x88,0xE8,0x08,0x10,0x00,0x20,0x3F,0x20,0x00,0x03,0x00,0x00,0x00,

    /*--  文字:  G  --*/
    /*--  Comic Sans MS12;  此字体下对应的点阵为：宽x高=8x16   --*/
    0xC0,0x30,0x08,0x08,0x08,0x38,0x00,0x00,0x07,0x18,0x20,0x20,0x22,0x1E,0x02,0x00,

    /*--  文字:  H  --*/
    /*--  Comic Sans MS12;  此字体下对应的点阵为：宽x高=8x16   --*/
    0x08,0xF8,0x08,0x00,0x00,0x08,0xF8,0x08,0x20,0x3F,0x21,0x01,0x01,0x21,0x3F,0x20,

    /*--  文字:  I  --*/
    /*--  Comic Sans MS12;  此字体下对应的点阵为：宽x高=8x16   --*/
    0x00,0x08,0x08,0xF8,0x08,0x08,0x00,0x00,0x00,0x20,0x20,0x3F,0x20,0x20,0x00,0x00,

    /*--  文字:  J  --*/
    /*--  Comic Sans MS12;  此字体下对应的点阵为：宽x高=8x16   --*/
    0x00,0x00,0x08,0x08,0xF8,0x08,0x08,0x00,0xC0,0x80,0x80,0x80,0x7F,0x00,0x00,0x00,

    /*--  文字:  K  --*/
    /*--  Comic Sans MS12;  此字体下对应的点阵为：宽x高=8x16   --*/
    0x08,0xF8,0x88,0xC0,0x28,0x18,0x08,0x00,0x20,0x3F,0x20,0x01,0x26,0x38,0x20,0x00,

    /*--  文字:  L  --*/
    /*--  Comic Sans MS12;  此字体下对应的点阵为：宽x高=8x16   --*/
    0x08,0xF8,0x08,0x00,0x00,0x00,0x00,0x00,0x20,0x3F,0x20,0x20,0x20,0x20,0x30,0x00,

    /*--  文字:  M  --*/
    /*--  Comic Sans MS12;  此字体下对应的点阵为：宽x高=8x16   --*/
    0x08,0xF8,0xF8,0x00,0xF8,0xF8,0x08,0x00,0x20,0x3F,0x00,0x3F,0x00,0x3F,0x20,0x00,

    /*--  文字:  N  --*/
    /*--  Comic Sans MS12;  此字体下对应的点阵为：宽x高=8x16   --*/
    0x08,0xF8,0x30,0xC0,0x00,0x08,0xF8,0x08,0x20,0x3F,0x20,0x00,0x07,0x18,0x3F,0x00,

    /*--  文字:  O  --*/
    /*--  Comic Sans MS12;  此字体下对应的点阵为：宽x高=8x16   --*/
    0xE0,0x10,0x08,0x08,0x08,0x10,0xE0,0x00,0x0F,0x10,0x20,0x20,0x20,0x10,0x0F,0x00,

    /*--  文字:  P  --*/
    /*--  Comic Sans MS12;  此字体下对应的点阵为：宽x高=8x16   --*/
    0x08,0xF8,0x08,0x08,0x08,0x08,0xF0,0x00,0x20,0x3F,0x21,0x01,0x01,0x01,0x00,0x00,

    /*--  文字:  Q  --*/
    /*--  Comic Sans MS12;  此字体下对应的点阵为：宽x高=8x16   --*/
    0xE0,0x10,0x08,0x08,0x08,0x10,0xE0,0x00,0x0F,0x18,0x24,0x24,0x38,0x50,0x4F,0x00,

    /*--  文字:  R  --*/
    /*--  Comic Sans MS12;  此字体下对应的点阵为：宽x高=8x16   --*/
    0x08,0xF8,0x88,0x88,0x88,0x88,0x70,0x00,0x20,0x3F,0x20,0x00,0x03,0x0C,0x30,0x20,

    /*--  文字:  S  --*/
    /*--  Comic Sans MS12;  此字体下对应的点阵为：宽x高=8x16   --*/
    0x00,0x70,0x88,0x08,0x08,0x08,0x38,0x00,0x00,0x38,0x20,0x21,0x21,0x22,0x1C,0x00,

    /*--  文字:  T  --*/
    /*--  Comic Sans MS12;  此字体下对应的点阵为：宽x高=8x16   --*/
    0x18,0x08,0x08,0xF8,0x08,0x08,0x18,0x00,0x00,0x00,0x20,0x3F,0x20,0x00,0x00,0x00,

    /*--  文字:  U  --*/
    /*--  Comic Sans MS12;  此字体下对应的点阵为：宽x高=8x16   --*/
    0x08,0xF8,0x08,0x00,0x00,0x08,0xF8,0x08,0x00,0x1F,0x20,0x20,0x20,0x20,0x1F,0x00,

    /*--  文字:  V  --*/
    /*--  Comic Sans MS12;  此字体下对应的点阵为：宽x高=8x16   --*/
    0x08,0x78,0x88,0x00,0x00,0xC8,0x38,0x08,0x00,0x00,0x07,0x38,0x0E,0x01,0x00,0x00,

    /*--  文字:  W  --*/
    /*--  Comic Sans MS12;  此字体下对应的点阵为：宽x高=8x16   --*/
    0xF8,0x08,0x00,0xF8,0x00,0x08,0xF8,0x00,0x03,0x3C,0x07,0x00,0x07,0x3C,0x03,0x00,

    /*--  文字:  X  --*/
    /*--  Comic Sans MS12;  此字体下对应的点阵为：宽x高=8x16   --*/
    0x08,0x18,0x68,0x80,0x80,0x68,0x18,0x08,0x20,0x30,0x2C,0x03,0x03,0x2C,0x30,0x20,

    /*--  文字:  Y  --*/
    /*--  Comic Sans MS12;  此字体下对应的点阵为：宽x高=8x16   --*/
    0x08,0x38,0xC8,0x00,0xC8,0x38,0x08,0x00,0x00,0x00,0x20,0x3F,0x20,0x00,0x00,0x00,

    /*--  文字:  Z  --*/
    /*--  Comic Sans MS12;  此字体下对应的点阵为：宽x高=8x16   --*/
    0x10,0x08,0x08,0x08,0xC8,0x38,0x08,0x00,0x20,0x38,0x26,0x21,0x20,0x20,0x18,0x00,

    /*--  文字:  [  --*/
    /*--  Comic Sans MS12;  此字体下对应的点阵为：宽x高=8x16   --*/
    0x00,0x00,0x00,0xFE,0x02,0x02,0x02,0x00,0x00,0x00,0x00,0x7F,0x40,0x40,0x40,0x00,

    /*--  文字:  \  --*/
    /*--  Comic Sans MS12;  此字体下对应的点阵为：宽x高=8x16   --*/
    0x00,0x0C,0x30,0xC0,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x01,0x06,0x38,0xC0,0x00,

    /*--  文字:  ]  --*/
    /*--  Comic Sans MS12;  此字体下对应的点阵为：宽x高=8x16   --*/
    0x00,0x02,0x02,0x02,0xFE,0x00,0x00,0x00,0x00,0x40,0x40,0x40,0x7F,0x00,0x00,0x00,

    /*--  文字:  ^  --*/
    /*--  Comic Sans MS12;  此字体下对应的点阵为：宽x高=8x16   --*/
    0x00,0x00,0x04,0x02,0x02,0x02,0x04,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,

    /*--  文字:  _  --*/
    /*--  Comic Sans MS12;  此字体下对应的点阵为：宽x高=8x16   --*/
    0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x80,0x80,0x80,0x80,0x80,0x80,0x80,0x80,

    /*--  文字:  `  --*/
    /*--  Comic Sans MS12;  此字体下对应的点阵为：宽x高=8x16   --*/
    0x00,0x02,0x02,0x04,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,

    /*--  文字:  a  --*/
    /*--  Comic Sans MS12;  此字体下对应的点阵为：宽x高=8x16   --*/
    0x00,0x00,0x80,0x80,0x80,0x80,0x00,0x00,0x00,0x19,0x24,0x22,0x22,0x22,0x3F,0x20,

    /*--  文字:  b  --*/
    /*--  Comic Sans MS12;  此字体下对应的点阵为：宽x高=8x16   --*/
    0x08,0xF8,0x00,0x80,0x80,0x00,0x00,0x00,0x00,0x3F,0x11,0x20,0x20,0x11,0x0E,0x00,

    /*--  文字:  c  --*/
    /*--  Comic Sans MS12;  此字体下对应的点阵为：宽x高=8x16   --*/
    0x00,0x00,0x00,0x80,0x80,0x80,0x00,0x00,0x00,0x0E,0x11,0x20,0x20,0x20,0x11,0x00,

    /*--  文字:  d  --*/
    /*--  Comic Sans MS12;  此字体下对应的点阵为：宽x高=8x16   --*/
    0x00,0x00,0x00,0x80,0x80,0x88,0xF8,0x00,0x00,0x0E,0x11,0x20,0x20,0x10,0x3F,0x20,

    /*--  文字:  e  --*/
    /*--  Comic Sans MS12;  此字体下对应的点阵为：宽x高=8x16   --*/
    0x00,0x00,0x80,0x80,0x80,0x80,0x00,0x00,0x00,0x1F,0x22,0x22,0x22,0x22,0x13,0x00,

    /*--  文字:  f  --*/
    /*--  Comic Sans MS12;  此字体下对应的点阵为：宽x高=8x16   --*/
    0x00,0x80,0x80,0xF0,0x88,0x88,0x88,0x18,0x00,0x20,0x20,0x3F,0x20,0x20,0x00,0x00,

    /*--  文字:  g  --*/
    /*--  Comic Sans MS12;  此字体下对应的点阵为：宽x高=8x16   --*/
    0x00,0x00,0x80,0x80,0x80,0x80,0x80,0x00,0x00,0x6B,0x94,0x94,0x94,0x93,0x60,0x00,

    /*--  文字:  h  --*/
    /*--  Comic Sans MS12;  此字体下对应的点阵为：宽x高=8x16   --*/
    0x08,0xF8,0x00,0x80,0x80,0x80,0x00,0x00,0x20,0x3F,0x21,0x00,0x00,0x20,0x3F,0x20,

    /*--  文字:  i  --*/
    /*--  Comic Sans MS12;  此字体下对应的点阵为：宽x高=8x16   --*/
    0x00,0x80,0x98,0x98,0x00,0x00,0x00,0x00,0x00,0x20,0x20,0x3F,0x20,0x20,0x00,0x00,

    /*--  文字:  j  --*/
    /*--  Comic Sans MS12;  此字体下对应的点阵为：宽x高=8x16   --*/
    0x00,0x00,0x00,0x80,0x98,0x98,0x00,0x00,0x00,0xC0,0x80,0x80,0x80,0x7F,0x00,0x00,

    /*--  文字:  k  --*/
    /*--  Comic Sans MS12;  此字体下对应的点阵为：宽x高=8x16   --*/
    0x08,0xF8,0x00,0x00,0x80,0x80,0x80,0x00,0x20,0x3F,0x24,0x02,0x2D,0x30,0x20,0x00,

    /*--  文字:  l  --*/
    /*--  Comic Sans MS12;  此字体下对应的点阵为：宽x高=8x16   --*/
    0x00,0x08,0x08,0xF8,0x00,0x00,0x00,0x00,0x00,0x20,0x20,0x3F,0x20,0x20,0x00,0x00,

    /*--  文字:  m  --*/
    /*--  Comic Sans MS12;  此字体下对应的点阵为：宽x高=8x16   --*/
    0x80,0x80,0x80,0x80,0x80,0x80,0x80,0x00,0x20,0x3F,0x20,0x00,0x3F,0x20,0x00,0x3F,

    /*--  文字:  n  --*/
    /*--  Comic Sans MS12;  此字体下对应的点阵为：宽x高=8x16   --*/
    0x80,0x80,0x00,0x80,0x80,0x80,0x00,0x00,0x20,0x3F,0x21,0x00,0x00,0x20,0x3F,0x20,

    /*--  文字:  o  --*/
    /*--  Comic Sans MS12;  此字体下对应的点阵为：宽x高=8x16   --*/
    0x00,0x00,0x80,0x80,0x80,0x80,0x00,0x00,0x00,0x1F,0x20,0x20,0x20,0x20,0x1F,0x00,

    /*--  文字:  p  --*/
    /*--  Comic Sans MS12;  此字体下对应的点阵为：宽x高=8x16   --*/
    0x80,0x80,0x00,0x80,0x80,0x00,0x00,0x00,0x80,0xFF,0xA1,0x20,0x20,0x11,0x0E,0x00,

    /*--  文字:  q  --*/
    /*--  Comic Sans MS12;  此字体下对应的点阵为：宽x高=8x16   --*/
    0x00,0x00,0x00,0x80,0x80,0x80,0x80,0x00,0x00,0x0E,0x11,0x20,0x20,0xA0,0xFF,0x80,

    /*--  文字:  r  --*/
    /*--  Comic Sans MS12;  此字体下对应的点阵为：宽x高=8x16   --*/
    0x80,0x80,0x80,0x00,0x80,0x80,0x80,0x00,0x20,0x20,0x3F,0x21,0x20,0x00,0x01,0x00,

    /*--  文字:  s  --*/
    /*--  Comic Sans MS12;  此字体下对应的点阵为：宽x高=8x16   --*/
    0x00,0x00,0x80,0x80,0x80,0x80,0x80,0x00,0x00,0x33,0x24,0x24,0x24,0x24,0x19,0x00,

    /*--  文字:  t  --*/
    /*--  Comic Sans MS12;  此字体下对应的点阵为：宽x高=8x16   --*/
    0x00,0x80,0x80,0xE0,0x80,0x80,0x00,0x00,0x00,0x00,0x00,0x1F,0x20,0x20,0x00,0x00,

    /*--  文字:  u  --*/
    /*--  Comic Sans MS12;  此字体下对应的点阵为：宽x高=8x16   --*/
    0x80,0x80,0x00,0x00,0x00,0x80,0x80,0x00,0x00,0x1F,0x20,0x20,0x20,0x10,0x3F,0x20,

    /*--  文字:  v  --*/
    /*--  Comic Sans MS12;  此字体下对应的点阵为：宽x高=8x16   --*/
    0x80,0x80,0x80,0x00,0x00,0x80,0x80,0x80,0x00,0x01,0x0E,0x30,0x08,0x06,0x01,0x00,

    /*--  文字:  w  --*/
    /*--  Comic Sans MS12;  此字体下对应的点阵为：宽x高=8x16   --*/
    0x80,0x80,0x00,0x80,0x00,0x80,0x80,0x80,0x0F,0x30,0x0C,0x03,0x0C,0x30,0x0F,0x00,

    /*--  文字:  x  --*/
    /*--  Comic Sans MS12;  此字体下对应的点阵为：宽x高=8x16   --*/
    0x00,0x80,0x80,0x00,0x80,0x80,0x80,0x00,0x00,0x20,0x31,0x2E,0x0E,0x31,0x20,0x00,

    /*--  文字:  y  --*/
    /*--  Comic Sans MS12;  此字体下对应的点阵为：宽x高=8x16   --*/
    0x80,0x80,0x80,0x00,0x00,0x80,0x80,0x80,0x80,0x81,0x8E,0x70,0x18,0x06,0x01,0x00,

    /*--  文字:  z  --*/
    /*--  Comic Sans MS12;  此字体下对应的点阵为：宽x高=8x16   --*/
    0x00,0x80,0x80,0x80,0x80,0x80,0x80,0x00,0x00,0x21,0x30,0x2C,0x22,0x21,0x30,0x00,

    /*--  文字:  {  --*/
    /*--  Comic Sans MS12;  此字体下对应的点阵为：宽x高=8x16   --*/
    0x00,0x00,0x00,0x00,0x80,0x7C,0x02,0x02,0x00,0x00,0x00,0x00,0x00,0x3F,0x40,0x40,

    /*--  文字:  |  --*/
    /*--  Comic Sans MS12;  此字体下对应的点阵为：宽x高=8x16   --*/
    0x00,0x00,0x00,0x00,0xFF,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0xFF,0x00,0x00,0x00,

    /*--  文字:  }  --*/
    /*--  Comic Sans MS12;  此字体下对应的点阵为：宽x高=8x16   --*/
    0x00,0x02,0x02,0x7C,0x80,0x00,0x00,0x00,0x00,0x40,0x40,0x3F,0x00,0x00,0x00,0x00,

    /*--  文字:  ~  --*/
    /*--  Comic Sans MS12;  此字体下对应的点阵为：宽x高=8x16   --*/
    0x00,0x06,0x01,0x01,0x02,0x02,0x04,0x04,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00

};



//============initial
void Initial_Lcd()
{
    LED_on;
    reset_0;                 //Reset the chip when reset=0
    Delay(50);
    reset_1;
    Delay(50);
    Transfer_command(0xae);		/*Display OFF*/
    Transfer_command(0xe2);		/*软复位*/
    Transfer_command(0x2c);		/*升压步聚1*/
    Transfer_command(0x2e);		/*升压步聚2*/
    Transfer_command(0x2f);		/*升压步聚3*/
    Transfer_command(0x27);		/*粗调对比度，可设置范围20～27*/
    Transfer_command(0x81);		/*微调对比度*/
    //Transfer_command(0x58);		/*微调对比度的值，可设置范围0～63 28*/
    Transfer_command(0xa2);		/*1/9偏压比（bias）*/
    Transfer_command(0xc8);		/*行扫描顺序：从上到下*/
    Transfer_command(0xa0);		/*列扫描顺序：从左到右*/

    clear_screen();
    Transfer_command(0xaf);		/*开显示*/
}



//===============clear all dot martrics=============
void clear_screen()
{
    unsigned char i,j;
    for(i=0; i<9; i++)
    {
        cs1_0;
        Transfer_command(0xb0+i);
        Transfer_command(0x10);
        Transfer_command(0x00);
        for(j=0; j<132; j++)
        {
            Transfer_data(0x00);
        }
    }
}



/*=======写指令========*/
void Transfer_command(uint8 data1)
{
    char i;
    cs1_0;
    rs_0;
    for(i=0; i<8; i++)
    {
        sclk_0;

        if(data1&0x80)
            sid_1;
        else
            sid_0;

        //Delay1(2);
        sclk_1;
        //Delay1(2);
        data1<<=1;
    }
}



/*--------写数据------------*/
void Transfer_data(uint8 data1)
{
    uint8 i;
    cs1_0;
    rs_1;
    for(i=0; i<8; i++)
    {
        sclk_0;
        if(data1&0x80) sid_1;
        else sid_0;
        sclk_1;
        data1<<=1;
    }
//  EA=1;
}



/*=========延时=====================*/
void Delay(int i)
{
    int j,k;
    for(j=0; j<i; j++)
        for(k=0; k<990; k++)
            ;
}



/*=========延时=====================*/
void Delay1(int i)
{
    int j,k;
    for(j=0; j<i; j++)
        for(k=0; k<10; k++)
            ;
}



/****送指令到晶联讯字库 IC***/
void send_command_to_ROM( uint8 datu )
{
    uint8 i;
    for(i=0; i<8; i++ )
    {
        if(datu&0x80)
            Rom_IN_1;
        else
            Rom_IN_0;

        Rom_SCK_1;
        datu = datu<<1;
        Rom_SCK_0;
    }
}



/****从晶联讯字库 IC 中取汉字或字符数据（1 个字节）***/
static uint8 get_data_from_ROM( )
{
    uint8 i;
    uint8 ret_data=0;

    for(i=0; i<8; i++)
    {
        ret_data=ret_data<<1;
        Rom_SCK_1;

        if( Rom_OUT )
            ret_data=ret_data|0x01;

        Rom_SCK_0;
    }

    return(ret_data);
}



/*显示 16x16 点阵图像、汉字、生僻字或 16x16 点阵的其他图标*/
void display_graphic_16x16(uint8 page,uint8 column,const uint8 *dp)
{
    uint8 i,j;
    uint8 page_address;
    uint8 column_address_L,column_address_H;
    page_address     = 0xb0+page;
    column_address_L = column&0x0f;
    column_address_H = ((column>>4)&0x0f)|0x10;
    cs1_0;
    Rom_CS_1;

    for(j=0; j<2; j++)
    {
        Transfer_command(page_address+j);   /*设置页地址*/
        Transfer_command(column_address_H); /*设置列地址的高 4 位*/
        Transfer_command(column_address_L); /*设置列地址的低 4 位*/

        for (i=0; i<16; i++)
        {
            Transfer_data(*dp);             /*写数据到 LCD,每写完一个 8 位的数据后列地址自动加 1*/
            dp++;
        }
    }

    cs1_1;
}



void display_graphic_16x8(uint8 page,uint8 column,const uint8 *dp)
{
    uint8 i;
    uint8 page_address;
    uint8 column_address_L,column_address_H;
    page_address     = 0xb0+page;
    column_address_L = column&0x0f;
    column_address_H = ((column>>4)&0x0f)|0x10;
    cs1_0;
    Rom_CS_1;

    Transfer_command(page_address);     /*设置页地址*/
    Transfer_command(column_address_H); /*设置列地址的高 4 位*/
    Transfer_command(column_address_L); /*设置列地址的低 4 位*/

    for (i=0; i<16; i++)
    {
        Transfer_data(*dp);             /*写数据到 LCD,每写完一个 8 位的数据后列地址自动加 1*/
        dp++;
    }

    cs1_1;
}



/*从相关地址（addrHigh：地址高字节,addrMid：地址中字节,addrLow：地址低字节）中连续读出DataLen 个字节
  的数据到 pBuff 的地址*/
/*连续读取*/
void get_n_bytes_data_from_ROM(uint8 addrHigh,uint8 addrMid,uint8 addrLow,uint8 *pBuff,uint8 DataLen )
{
    uint8 i;
    cs1_1;
    Rom_CS_0;
    Rom_SCK_0;
    send_command_to_ROM(0x03);
    send_command_to_ROM(addrHigh);
    send_command_to_ROM(addrMid);
    send_command_to_ROM(addrLow);

    for(i = 0; i < DataLen; i++ )
        *(pBuff+i) =get_data_from_ROM();

    Rom_CS_1;
}



void displayNum(s32 num,uint8 y,uint8 x)
{
    u8 dp[15]="";
    u8 i,count,start=0;
    u32 dig=num;

    if(num<0)
    {
        dp[0]='-';
        start=1;
        dig=-num;
    }

    if(dig<10L)
        count=1;
    else if(dig<100L)
        count=2;
    else if(dig<1000L)
        count=3;
    else if(dig<10000L)
        count=4;
    else if(dig<100000L)
        count=5;
    else if(dig<1000000L)
        count=6;
    else if(dig<10000000L)
        count=7;
    else if(dig<100000000L)
        count=8;
    else if(dig<1000000000L)
        count=9;
    else
        count=10;

    for(i=0; i<count; i++)
    {
        dp[count-i+start-1]=(dig%10)+'0';
        dig/=10;
    }

    display_ascii(dp,start+i,y,x);
}
