#include "FLIPDOTS.h"

#define HEADER 0x80
#define ENDCHAR 0x8f

#define CMD_WRITE_UPDATE 0x87
#define CMD_WRITE 0x88
#define CMD_UPDATE 0x82

const byte font3x3Glyphs[][3] = {
    {0b010, 0b111, 0b101}, /* A */
    {0b110, 0b111, 0b111}, /* B */
    {0b111, 0b100, 0b111}, /* C */
    {0b110, 0b101, 0b110}, /* D */
    {0b111, 0b110, 0b111}, /* E */
    {0b111, 0b110, 0b100}, /* F */
    {0b110, 0b101, 0b111}, /* G */
    {0b101, 0b111, 0b101}, /* H */
    {0b111, 0b010, 0b111}, /* I */
    {0b001, 0b101, 0b111}, /* J */
    {0b101, 0b110, 0b101}, /* K */
    {0b100, 0b100, 0b111}, /* L */
    {0b111, 0b111, 0b101}, /* M */
    {0b111, 0b101, 0b101}, /* N */
    {0b111, 0b101, 0b111}, /* O */
    {0b111, 0b111, 0b100}, /* P */
    {0b111, 0b111, 0b001}, /* Q */
    {0b111, 0b100, 0b100}, /* R */
    {0b011, 0b010, 0b110}, /* S */
    {0b111, 0b010, 0b010}, /* T */
    {0b101, 0b101, 0b111}, /* U */
    {0b101, 0b101, 0b010}, /* V */
    {0b101, 0b111, 0b111}, /* W */
    {0b101, 0b010, 0b101}, /* X */
    {0b101, 0b010, 0b010}, /* Y */
    {0b110, 0b010, 0b011}, /* Z */
    {0b111, 0b101, 0b111}, /* 0 */
    {0b110, 0b010, 0b111}, /* 1 */
    {0b110, 0b010, 0b011}, /* 2 */
    {0b111, 0b011, 0b111}, /* 3 */
    {0b101, 0b111, 0b001}, /* 4 */
    {0b011, 0b010, 0b110}, /* 5 */
    {0b100, 0b111, 0b111}, /* 6 */
    {0b111, 0b001, 0b001}, /* 7 */
    {0b011, 0b111, 0b111}, /* 8 */
    {0b111, 0b111, 0b001}, /* 9 */
    {0b000, 0b000, 0b000}, /* EMPTY */
};

/**
 * Begin communication with the FLIPDOTS display.
 * @param baudRate - baudrate of the serial port
 * @param ms - delay to account for display startup time
 */
void FLIPDOTS::begin(unsigned long baudRate, unsigned long ms)
{
    Serial->begin(baudRate);
    delay(ms);
}

/* 
 * Force the display to update with whatever is in its buffer
 */
void FLIPDOTS::update()
{
    const byte refreshSequence[] = {HEADER, CMD_UPDATE, ENDCHAR};
    Serial->write(refreshSequence, sizeof(refreshSequence) / sizeof(byte));
}

/**
 * Write an array of 7 bytes to the display, where the least significant 7 bits of each byte are used as rows/columns.
 * @param data The 7 byte array
 * @param autoUpdate If true, the display will be updated after the write
 */
void FLIPDOTS::write(const byte data[7], bool autoUpdate)
{

    byte writeSequence[] = {HEADER, autoUpdate ? CMD_WRITE_UPDATE : CMD_WRITE, address, 0, 0, 0, 0, 0, 0, 0, ENDCHAR};
    for (int i = 0; i < 7; i++)
    {
        writeSequence[i + 3] = inverted ? ~data[i] : data[i];
    }
    Serial->write(writeSequence, sizeof(writeSequence) / sizeof(byte));
}

void FLIPDOTS::setInverted(bool inverted)
{
    this->inverted = inverted;
}

void FLIPDOTS::write3x3char4(const char *charArray)
{
    const byte *g1 = get3x3FontGlyph(charArray[0]);
    const byte *g2 = get3x3FontGlyph(charArray[1]);
    const byte *g3 = get3x3FontGlyph(charArray[2]);
    const byte *g4 = get3x3FontGlyph(charArray[3]);

    byte data[7] = {0};
    for (uint8_t i = 0; i < 3; i++)
    {
        // i from 0 to 2 is the row of the 3x3 glyph
        // two glyphs are joined together in one row,
        // then the rows are pushed into the data buffer
        // in reverse order.
        data[6 - i] = g1[i] << 4 | g2[i];
        data[2 - i] = g3[i] << 4 | g4[i];
    }
    write(data);
}

void FLIPDOTS::clear()
{
    const byte clear[7] = {0};
    write(clear);
}

const byte *FLIPDOTS::get3x3FontGlyph(char c)
{
    if (c >= 'A' && c <= 'Z')
    {
        return font3x3Glyphs[c - 'A'];
    }
    if (c >= 'a' && c <= 'z')
    {
        return font3x3Glyphs[c - 'a'];
    }
    if (c >= '0' && c <= '9')
    {
        return font3x3Glyphs[c - '0' + 26];
    }
    return font3x3Glyphs[36]; // char not in font, return empty glyph
}
