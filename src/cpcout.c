/*

    casm - Simple, portable assembler

    Copyright (C) 2003-2015  Ian Cowburn (ianc@noddybox.co.uk)

    This program is free software: you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation, either version 3 of the License, or
    (at your option) any later version.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with this program.  If not, see <http://www.gnu.org/licenses/>.

    -------------------------------------------------------------------------

    Amstrad CPC tape output handler.

*/
#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#include "global.h"
#include "codepage.h"
#include "cpcout.h"
#include "expr.h"


/* ---------------------------------------- MACROS & TYPES
*/

#define BLOCK_SIZE      2048
#define LO_BYTE(w)      ((w) & 0xff)
#define HI_BYTE(w)      (((w) & 0xff00)>>8)

enum option_t
{
    OPT_START_ADDR,
    OPT_LOADER
};

static const ValueTable option_set[] =
{
    {"cpc-start",	OPT_START_ADDR},
    {"cpc-loader",	OPT_LOADER},
    {NULL}
};

typedef struct
{
    int         start_addr;
    int         loader;
} Options;

static Options options = {-1, 0};

typedef struct
{
    Byte *stream;
    size_t length;
} Stream;

/* ---------------------------------------- PRIVATE FUNCTIONS
*/
static void InitStream(Stream *s)
{
    s->stream = NULL;
    s->length = 0;
}


static void AddStreamByte(Stream *s, Byte b)
{
    s->length++;
    s->stream = Realloc(s->stream, s->length);
    s->stream[s->length - 1] = b;
}


static void AddStreamMem(Stream *s, Byte *mem, size_t len)
{
    while(len--)
    {
        AddStreamByte(s, *mem++);
    }
}


static void AddStreamWord(Stream *s, int w)
{
    AddStreamByte(s, LO_BYTE(w));
    AddStreamByte(s, HI_BYTE(w));
}


static void FreeStream(Stream *s)
{
    if (s->stream)
    {
        free(s->stream);
    }

    InitStream(s);
}


static Word CRC(const Byte *b, int size)
{
    Word crc = 0xffff;
    int f;

    for(f = 0; f < 256; f++)
    {
        Word w;
        int n;

        if (f < size)
        {
            w = b[f];
        }
        else
        {
            w = 0;
        }

        crc ^= w << 8;

        for(n = 0; n < 8; n++)
        {
            if (crc & 0x8000)
            {
                crc = ((crc << 1) ^ 0x1021) & 0xffff;
            }
            else
            {
                crc = (crc << 1) & 0xffff;
            }
        }
    }

    crc ^= 0xffff;

    return crc;
}


static void WriteByte(FILE *fp, Byte b)
{
    putc(b, fp);
}


static void WriteWord(FILE *fp, int w)
{
    WriteByte(fp, LO_BYTE(w));
    WriteByte(fp, HI_BYTE(w));
}


static void WriteDWord(FILE *fp, unsigned long w)
{
    WriteWord(fp, w & 0xffff);
    WriteWord(fp, (w & 0xffff0000) >> 16);
}


static void WriteMem(FILE *fp, const Byte *mem, int len)
{
    fwrite(mem, 1, len, fp);
}


static void WriteWordMem(Byte *mem, int offset, int w)
{
    mem[offset] = LO_BYTE(w);
    mem[offset+1] = HI_BYTE(w);
}


static void Write3Word(FILE *fp, int w)
{
    WriteByte(fp, LO_BYTE(w));
    WriteByte(fp, HI_BYTE(w));
    WriteByte(fp, (w & 0xff0000) >> 16);
}


static void WriteString(FILE *fp, const char *p, int len,
                        Byte fill, Codepage cp)
{
    while(len--)
    {
        WriteByte(fp, *p ? CodeFromNative(cp, *p++) : CodeFromNative(cp, fill));
    }
}


static void WriteStringMem(Byte *mem, int offset, const char *p, int len,
                           Byte fill, Codepage cp)
{
    while(len--)
    {
        mem[offset++] =
                *p ? CodeFromNative(cp, *p++) : CodeFromNative(cp, fill);
    }
}


static void OutputTZXTurboBlock(FILE *fp, Stream *s)
{
    WriteByte(fp, 0x11);                /* Block type - Turbo block */
    WriteWord(fp, 0x0b21);              /* PILOT pulse len */
    WriteWord(fp, 0x05ad);              /* SYNC 1 len */
    WriteWord(fp, 0x05ad);              /* SYNC 2 len */
    WriteWord(fp, 0x05ac);              /* Zero len */
    WriteWord(fp, 0x0af4);              /* One len */
    WriteWord(fp, 0x1002);              /* PILOT tone */
    WriteByte(fp, 8);                   /* Last byte used bits */
    WriteWord(fp, 0x0011);              /* Pause after block */
    Write3Word(fp, s->length);
    WriteMem(fp, s->stream, s->length);
}


/* ---------------------------------------- INTERFACES
*/
const ValueTable *CPCOutputOptions(void)
{
    return option_set;
}

CommandStatus CPCOutputSetOption(int opt, int argc, char *argv[], int quoted[],
                                 char *err, size_t errsize)
{
    CommandStatus stat = CMD_OK;

    CMD_ARGC_CHECK(1);

    switch(opt)
    {
        case OPT_START_ADDR:
            CMD_EXPR_INT(argv[0], options.start_addr);
            break;

	case OPT_LOADER:
	    options.loader = ParseTrueFalse(argv[0], FALSE);
	    break;

        default:
            break;
    }

    return stat;
}

int CPCOutput(const char *filename, const char *filename_bank,
              const unsigned *banks, int count, char *error, size_t error_size)
{
    FILE *fp = fopen(filename, "wb");
    int f;

    if (!fp)
    {
        snprintf(error, error_size, "Failed to create %s", filename);
        return FALSE;
    }

    /* First get the initial address is none defined
    */
    if (options.start_addr == -1)
    {
        options.start_addr = GetLowWriteMarker(banks[0]);
    }

    /* Output the binary files
    */
    WriteString(fp, "ZXTape!", 7, 0, CP_ASCII);
    WriteByte(fp, 0x1a);
    WriteByte(fp, 1);
    WriteByte(fp, 0x14);

    /* Output the basic loader if defined
    */
    if (options.loader)
    {
    	Stream basic;
	Stream stream;
	Byte header[256] = {0};
	Word crc;

	InitStream(&basic);

	/* Line 10 MEMORY &xxxx
	*/
	AddStreamWord(&basic, 2 + 2 + 1 + 3 + 1);
	AddStreamWord(&basic, 10);
	AddStreamByte(&basic, 0xaa);
	AddStreamByte(&basic, 0x1c);
	AddStreamWord(&basic, GetLowWriteMarker(banks[0]));
	AddStreamByte(&basic, 0);

	/* Line 20 LOAD ""
	*/
	AddStreamWord(&basic, 2 + 2 + 1 + 3 + 1);
	AddStreamWord(&basic, 20);
	AddStreamByte(&basic, 0xa8);
	AddStreamByte(&basic, CodeFromNative(CP_ASCII, '"'));
	AddStreamByte(&basic, CodeFromNative(CP_ASCII, '!'));
	AddStreamByte(&basic, CodeFromNative(CP_ASCII, '"'));
	AddStreamByte(&basic, 0);

	/* Line 30 CALL &xxxx
	*/
	AddStreamWord(&basic, 2 + 2 + 1 + 3 + 1);
	AddStreamWord(&basic, 30);
	AddStreamByte(&basic, 0x83);
	AddStreamByte(&basic, 0x1c);
	AddStreamWord(&basic, options.start_addr);
	AddStreamByte(&basic, 0);

        /* End of program
        */
        AddStreamWord(&basic, 0);

	/* Create header
	*/
	WriteStringMem(header, 0, "LOADER.BAS", 16, 0, CP_ASCII);

	header[16] = 1;
	header[17] = 255;
	header[18] = 0;
	WriteWordMem(header, 19, basic.length);
	WriteWordMem(header, 21, 0x170);
	header[23] = 255;
	WriteWordMem(header, 24, basic.length);

	crc = CRC(header, 256);

	/* Output header block
	*/
	InitStream(&stream);

	AddStreamByte(&stream, 0x2c);
	AddStreamMem(&stream, header, 256);
	AddStreamByte(&stream, HIBYTE(crc));
	AddStreamByte(&stream, LOBYTE(crc));
	AddStreamByte(&stream, 0xff);
	AddStreamByte(&stream, 0xff);
	AddStreamByte(&stream, 0xff);
	AddStreamByte(&stream, 0xff);

	OutputTZXTurboBlock(fp, &stream);

	FreeStream(&stream);

	/* Output basic data block
	*/
	InitStream(&stream);

	memset(header, 0, 256);
	memcpy(header, basic.stream, basic.length);
	crc = CRC(header, 256);

	AddStreamByte(&stream, 0x16);
	AddStreamMem(&stream, header, 256);
	AddStreamByte(&stream, HIBYTE(crc));
	AddStreamByte(&stream, LOBYTE(crc));
	AddStreamByte(&stream, 0xff);
	AddStreamByte(&stream, 0xff);
	AddStreamByte(&stream, 0xff);
	AddStreamByte(&stream, 0xff);

	OutputTZXTurboBlock(fp, &stream);

	FreeStream(&stream);

	FreeStream(&basic);
    }

    for(f = 0; f < count; f++)
    {
        int min, max, len, blocks, addr;
        int block, blocklen;
        Stream stream;

        min = GetLowWriteMarker(banks[f]);
        addr = min;
        max = GetHighWriteMarker(banks[f]);
        len = max - min + 1;
        blocks = len / BLOCK_SIZE;

        if ((len % BLOCK_SIZE) == 0)
        {
            blocks--;
        }

        for(block = 0; block <= blocks; block++)
        {
            Byte header[256] = {0};
            Word crc;
            int first, last;
            int seg, segs;

            InitStream(&stream);

            first = 0;
            last = 0;

            /* Create the header
            */
            if (f == 0)
            {
                WriteStringMem(header, 0, filename, 16, 0, CP_ASCII);
            }
            else
            {
                char fn[16];

                snprintf(fn, sizeof fn, filename_bank, banks[f]);
                WriteStringMem(header, 0, fn, 16, 0, CP_ASCII);
            }

            header[16] = block + 1;

            if (block == 0)
            {
                first = 255;

                if (len > BLOCK_SIZE)
                {
                    blocklen = BLOCK_SIZE;
                }
                else
                {
                    blocklen = len;
                }
            }

            if (block == blocks)
            {
                last = 255;
                blocklen = len % BLOCK_SIZE;
            }

            if (blocklen == BLOCK_SIZE)
            {
                segs = 7;
            }
            else
            {
                segs = blocklen / 256;
            }

            header[17] = last;
            header[18] = 2;
            WriteWordMem(header, 19, blocklen);
            WriteWordMem(header, 21, addr);
            header[23] = first;
            WriteWordMem(header, 24, len);
            WriteWordMem(header, 26, options.start_addr);

            crc = CRC(header, 256);

            /* Write header data
            */
            AddStreamByte(&stream, 0x2c);
            AddStreamMem(&stream, header, 256);
            AddStreamByte(&stream, HIBYTE(crc));
            AddStreamByte(&stream, LOBYTE(crc));
            AddStreamByte(&stream, 0xff);
            AddStreamByte(&stream, 0xff);
            AddStreamByte(&stream, 0xff);
            AddStreamByte(&stream, 0xff);

            OutputTZXTurboBlock(fp, &stream);

            FreeStream(&stream);

            /* Loop round for the segments (up to 8)
            */
            InitStream(&stream);
            AddStreamByte(&stream, 0x16);

            for(seg = 0; seg <= segs; seg++)
            {
                Byte segment[256] = {0};
                int segi = 0;
                int last_seg = 0;

                last_seg = (seg == segs);

                if (!last_seg || blocklen == BLOCK_SIZE)
                {
                    addr += 256;
                }
                else
                {
                    addr += blocklen % 256;
                }

                while(min < addr)
                {
                    segment[segi++] = MemoryReadBank(banks[f], min++);
                }

                /* Add segment data to stream
                */
                AddStreamMem(&stream, segment, 256);
                crc = CRC(segment, 256);
                AddStreamByte(&stream, HIBYTE(crc));
                AddStreamByte(&stream, LOBYTE(crc));

                if (last_seg)
                {
                    AddStreamByte(&stream, 0xff);
                    AddStreamByte(&stream, 0xff);
                    AddStreamByte(&stream, 0xff);
                    AddStreamByte(&stream, 0xff);
                }
            }

            OutputTZXTurboBlock(fp, &stream);

            FreeStream(&stream);
        }
    }

    fclose(fp);

    return TRUE;
}

/*
vim: ai sw=4 ts=8 expandtab
*/
