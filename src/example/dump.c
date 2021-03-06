#include <stdlib.h>
#include <unistd.h>
#include <stdio.h>
#include <string.h>
#include <ctype.h>

typedef struct
{
    int len;
    unsigned char *addr;
} Block;

static void HexDump(const unsigned char *p, int len)
{
    int o = 0;
    int f;
    char buff[17] = {0};

    while(o < len)
    {
    	strcpy(buff, "                ");

	printf("%4.4X:  ", o);

	for(f = 0; f < 16; f++)
	{
	    if (o < len)
	    {
	    	printf(" %2.2X", p[o]);
	    }
	    else
	    {
	    	printf(" **");
	    }

	    if (isprint(p[o]))
	    {
	    	buff[f] = p[o];
	    }
	    else
	    {
	    	buff[f] = '.';
	    }

	    o++;
	}

	printf("   %s\n", buff);
    }
}

static int Word(FILE *fp)
{
    int i0;
    int i1;

    i0 = getc(stdin);
    i1 = getc(stdin);

    return i0 | i1 << 8;
}

static int Triplet(FILE *fp)
{
    int i0;
    int i1;
    int i2;

    i0 = getc(stdin);
    i1 = getc(stdin);
    i2 = getc(stdin);

    return i0 | i1 << 8 | i2 << 16;
}

static Block *DumpBlock(FILE *fp)
{
    static int first = 1;
    Block *block;

    if (first)
    {
	printf("PILOT=%4.4x\n", Word(fp));	/* PILOT */
	printf("SYNC1=%4.4x\n", Word(fp));	/* SYNC1 */
	printf("SYNC2=%4.4x\n", Word(fp));	/* SYNC2 */
	printf("ZERO=%4.4x\n", Word(fp));	/* ZERO */
	printf("ONE=%4.4x\n", Word(fp));	/* ONE */
	printf("PILOT LEN=%4.4x\n", Word(fp));	/* PILOT LEN */
	printf("USED BITS=%2.2x\n", getc(fp));	/* USED BITS */
	printf("PAUSE=%4.4x\n", Word(fp));	/* PAUSE */
    }
    else
    {
	Word(fp);	/* PILOT */
	Word(fp);	/* SYNC1 */
	Word(fp);	/* SYNC2 */
	Word(fp);	/* ZERO */
	Word(fp);	/* ONE */
	Word(fp);	/* PILOT LEN */
	getc(fp);	/* USED BITS */
	Word(fp);	/* PAUSE */
    }

    block = malloc(sizeof *block);

    block->len = Triplet(fp);	/* LEN */

    if (first)
    {
	printf("LEN=%6.6x\n", block->len);
    }

    block->addr = malloc(block->len);
    fread(block->addr, 1, block->len, fp);

    if (first)
    {
    	HexDump(block->addr, block->len);
    }

    first = 1;

    return block;
}

static void CompareEnds(Block **block, int count)
{
    int f;
    int n;
    int min;
    int done;

    min = 0xffffff;

    for(f=0; f < count; f++)
    {
    	if (block[f]->len < min)
	{
	    min = block[f]->len;
	}
    }

    done = 0;

    for(f = 0; f < min && !done; f++)
    {
    	for(n = 0; n < count - 1 && !done; n++)
	{
	    int diff;

	    diff = (int)block[n]->addr[block[n]->len - 1 - f] -
	    		(int)block[n+1]->addr[block[n+1]->len - 1 - f];

	    if (abs(diff) > 5)
	    {
	    	done = 1;
	    }
	}
    }

    if (done)
    {
	printf("Last %d bytes match\n", --f);

	for(n = 0; n < f; n++)
	{
	    if (n > 0)
	    {
	    	printf(" ");
	    }

	    printf("%x", (unsigned)block[0]->addr[block[0]->len - 1 - n]);
	}

	printf("\n");
    }
}

int main(int argc, char *argv[])
{
    int ch;
    int count;
    int f;
    Block *block[256] = {0};
    char buff[9];

    fread(buff, 1, sizeof buff, stdin);

    ch = getc(stdin);

    while(ch != EOF)
    {
    	if (ch == 0x11)
	{
	    block[count++] = DumpBlock(stdin);
	}

	ch = getc(stdin);
    }

    printf("Got %d blocks\n", count);

    for(f = 0; f < count; f++)
    {
    	printf("Block %d len %d\n", f, block[f]->len);
    }

    CompareEnds(block, count);
}
