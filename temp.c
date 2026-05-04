void OLED_SetPixel(uint8_t x, uint8_t y, uint8_t on)
{
    uint8_t page = y / 8;
    uint8_t bit = y % 8;
    
    OLED_Cmd(0xB0 | page);
    OLED_Cmd(0x10 | (x >> 4));
    OLED_Cmd(0x00 | (x & 0x0F));
    
    uint8_t data = OLED_ReadData();
    
    if (on) {
        data |= (1 << bit);
    } else {
        data &= ~(1 << bit);
    }
    
    OLED_Cmd(0xB0 | page);
    OLED_Cmd(0x10 | (x >> 4));
    OLED_Cmd(0x00 | (x & 0x0F));
    OLED_Data(data);
}
