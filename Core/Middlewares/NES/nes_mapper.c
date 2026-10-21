/**
 ****************************************************************************************************
 * @file        nes_mapper.c
 * @author      正点原子团队(ALIENTEK)
 * @version     V1.1
 * @date        2022-05-31
 * @brief       NES地图 代码
 *              本程序移植自网友ye781205的NES模拟器工程, 特此感谢!
 * @license     Copyright (c) 2020-2032, 广州市星翼电子科技有限公司
 ****************************************************************************************************
 * @attention
 *
 * 实验平台:正点原子 开发板
 * 在线视频:www.yuanzige.com
 * 技术论坛:www.openedv.com
 * 公司网址:www.alientek.com
 * 购买地址:openedv.taobao.com
 *
 * 修改说明
 * V1.1 20220531
 * 1, 修改注释方式
 * 2, 修改u8/u16/u32为uint8_t/uint16_t/uint32_t
 ****************************************************************************************************
 */

#include "nes_mapper.h"


/* 首期只保留已由ROM层放行并有代表ROM覆盖的Mapper。 */
#include "mapper/000.cpp"
#include "mapper/003.cpp"
#include "mapper/004.cpp"
#include "mapper/023.cpp"


/* 与nes_rom_mapper_supported()保持一致。 */
const int MapTab[] =
{
    0, 3, 4, 23, -1
};
#define MASK_BANK(bank,mask) (bank) = ((bank) & (mask))
#define VALIDATE_VROM_BANK(bank) \
    MASK_BANK(bank,VROM_mask); \
    if((VROM_1K_SIZE <= 0) || ((bank) >= (uint32)VROM_1K_SIZE)) return;


uint32 VROM_mask;

void Mapper_Reset(void) {}
uint8 Mapper_ReadLow(uint16 addr)
{
    (void)addr;
    return 0;
}
void Mapper_WriteLow(uint16 addr, uint8 data)
{
    (void)addr;
    (void)data;
}
void Mapper_Write( uint16 addr, uint8 data)
{
    (void)addr;
    (void)data;
}
void Mapper_Read( uint8 data, uint16 addr )
{
    (void)data;
    (void)addr;
}
void Mapper_HSync( int scanline )
{
    (void)scanline;
}
void Mapper_VSync(void)  {}

void Mapper_Init(void)
{
    uint32 probe;
    NES_Mapper->Reset         = Mapper_Reset;
    NES_Mapper->Write         = Mapper_Write;
    NES_Mapper->Read          = Mapper_Read;
    NES_Mapper->WriteLow      = Mapper_WriteLow;
    NES_Mapper->ReadLow       = Mapper_ReadLow;
    NES_Mapper->HSync         = Mapper_HSync;
    NES_Mapper->VSync         = Mapper_VSync;
    VROM_mask = 0xFFFF;

    for (probe = 0x8000; probe; probe >>= 1)
    {
        if ((VROM_1K_SIZE - 1) & probe) break;

        VROM_mask >>= 1;
    }

    switch (MapperNo)
    {
        case 0 :
            MAP0_Init();
            break;

        case 3 :
            MAP3_Init();
            break;

        case 4 :
            MAP4_Init();
            break;

        case 23:
            MAP23_Init();
            break;

        default:
            break;
    }

    NES_Mapper->Reset();
}
void asm_Mapper_Write(uint8 byData, uint16 wAddr)
{
    NES_Mapper->Write(wAddr, byData);
}
void asm_Mapper_ReadLow( uint16 wAddr) //添加的
{
    NES_Mapper->ReadLow( wAddr);
}
void asm_Mapper_WriteLow( uint8 byData, uint16 wAddr)
{
    NES_Mapper->WriteLow(wAddr, byData );
}

void set_CPU_bank3(signed char page )
{
    map67_(page) ;
}
void set_CPU_bank4(signed char page )
{
    map89_(page) ;
}
void set_CPU_bank5(signed char page )
{
    mapAB_(page) ;
}
void set_CPU_bank6(signed char page )
{
    mapCD_(page) ;
}
void set_CPU_bank7(signed char page )
{
    mapEF_(page) ;
}
void set_CPU_banks(int bank0_num, int bank1_num, int bank2_num, int bank3_num)
{
    map89_(bank0_num);
    mapAB_(bank1_num);
    mapCD_(bank2_num);
    mapEF_(bank3_num);
}
void set_PPU_banks(uint32 bank0_num, uint32 bank1_num,
                   uint32 bank2_num, uint32 bank3_num,
                   uint32 bank4_num, uint32 bank5_num,
                   uint32 bank6_num, uint32 bank7_num)
{
    VALIDATE_VROM_BANK(bank0_num);
    VALIDATE_VROM_BANK(bank1_num);
    VALIDATE_VROM_BANK(bank2_num);
    VALIDATE_VROM_BANK(bank3_num);
    VALIDATE_VROM_BANK(bank4_num);
    VALIDATE_VROM_BANK(bank5_num);
    VALIDATE_VROM_BANK(bank6_num);
    VALIDATE_VROM_BANK(bank7_num);

    ppu->PPU_VRAM_banks[0] = VROM_banks + (bank0_num << 10); // * 0x400
    ppu->PPU_VRAM_banks[1] = VROM_banks + (bank1_num << 10);
    ppu->PPU_VRAM_banks[2] = VROM_banks + (bank2_num << 10);
    ppu->PPU_VRAM_banks[3] = VROM_banks + (bank3_num << 10);
    ppu->PPU_VRAM_banks[4] = VROM_banks + (bank4_num << 10);
    ppu->PPU_VRAM_banks[5] = VROM_banks + (bank5_num << 10);
    ppu->PPU_VRAM_banks[6] = VROM_banks + (bank6_num << 10);
    ppu->PPU_VRAM_banks[7] = VROM_banks + (bank7_num << 10);

    set_tile_banks( VROM_tiles + bank0_num * 0x400,
                    VROM_tiles + bank1_num * 0x400,
                    VROM_tiles + bank2_num * 0x400,
                    VROM_tiles + bank3_num * 0x400,
                    VROM_tiles + bank4_num * 0x400,
                    VROM_tiles + bank5_num * 0x400,
                    VROM_tiles + bank6_num * 0x400,
                    VROM_tiles + bank7_num * 0x400 );
}
void set_PPU_bank0(uint32 bank_num)
{
    VALIDATE_VROM_BANK(bank_num);
    ppu->PPU_VRAM_banks[0] = VROM_banks + (bank_num << 10); // * 0x400
    set_tile_bank(0, VROM_tiles + bank_num * 0x400);
}

void set_PPU_bank1(uint32 bank_num)
{
    VALIDATE_VROM_BANK(bank_num);
    ppu->PPU_VRAM_banks[1] = VROM_banks + (bank_num << 10); // * 0x400
    set_tile_bank(1, VROM_tiles + bank_num * 0x400 );
}

void set_PPU_bank2(uint32 bank_num)
{
    VALIDATE_VROM_BANK(bank_num);
    ppu->PPU_VRAM_banks[2] = VROM_banks + (bank_num << 10); // * 0x400
    set_tile_bank(2, VROM_tiles + bank_num * 0x400 );
}

void set_PPU_bank3(uint32 bank_num)
{
    VALIDATE_VROM_BANK(bank_num);
    ppu->PPU_VRAM_banks[3] = VROM_banks + (bank_num << 10); // * 0x400
    set_tile_bank(3, VROM_tiles + bank_num * 0x400 );
}

void set_PPU_bank4(uint32 bank_num)
{
    VALIDATE_VROM_BANK(bank_num);
    ppu->PPU_VRAM_banks[4] = VROM_banks + (bank_num << 10); // * 0x400
    set_tile_bank(4, VROM_tiles + bank_num * 0x400 );
}
void set_PPU_bank5(uint32 bank_num)
{
    VALIDATE_VROM_BANK(bank_num);
    ppu->PPU_VRAM_banks[5] = VROM_banks + (bank_num << 10); // * 0x400
    set_tile_bank(5, VROM_tiles + bank_num * 0x400 );
}

void set_PPU_bank6(uint32 bank_num)
{
    VALIDATE_VROM_BANK(bank_num);
    ppu->PPU_VRAM_banks[6] = VROM_banks + (bank_num << 10); // * 0x400
    set_tile_bank(6, VROM_tiles + bank_num * 0x400 );
}

void set_PPU_bank7(uint32 bank_num)
{
    VALIDATE_VROM_BANK(bank_num);
    ppu->PPU_VRAM_banks[7] = VROM_banks + (bank_num << 10); // * 0x400
    set_tile_bank(7, VROM_tiles + bank_num * 0x400);
}
// for mapper 19,68,90
void set_PPU_bank8(uint32 bank_num)
{
    VALIDATE_VROM_BANK(bank_num);
    ppu->PPU_VRAM_banks[8] = VROM_banks + (bank_num << 10);
}
void set_PPU_bank9(uint32 bank_num)
{
    VALIDATE_VROM_BANK(bank_num);
    ppu->PPU_VRAM_banks[9] = VROM_banks + (bank_num << 10);
}
void set_PPU_bank10(uint32 bank_num)
{
    VALIDATE_VROM_BANK(bank_num);
    ppu->PPU_VRAM_banks[10] = VROM_banks + (bank_num << 10);
}
void set_PPU_bank11(uint32 bank_num)
{
    VALIDATE_VROM_BANK(bank_num);
    ppu->PPU_VRAM_banks[11] = VROM_banks + (bank_num << 10);
}

// for mapper 1,4,5,6,13,19,80,85,96,119
void set_VRAM_bank(uint8 bank, uint32 bank_num)
{
    if (bank < 8)
    {
        ppu->PPU_VRAM_banks[bank] = PPU_patterntables + ((bank_num & 0x0f) << 10);
    }
    else if (bank < 12)
    {
        set_name_table(bank, bank_num);
    }
}












