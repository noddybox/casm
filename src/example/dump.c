#include <stdlib.h>
#include <unistd.h>
#include <stdio.h>
#include <string.h>

typedef struct
{
    int len;
    unsigned char *addr;
} Block;

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
    Block *block;

    Word(fp);	/* PILOT */
    Word(fp);	/* SYNC1 */
    Word(fp);	/* SYNC2 */
    Word(fp);	/* ZERO */
    Word(fp);	/* ONE */
    Word(fp);	/* PILOT LEN */
    getc(fp);	/* USED BITS */
    Word(fp);	/* PAUSE */

    block = malloc(sizeof *block);

    block->len = Triplet(fp);	/* LEN */
    block->addr = malloc(block->len);
    fread(block->addr, 1, block->len, fp);

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
	    if (block[n]->addr[block[n]->len - 1 - f] !=
	    		block[n+1]->addr[block[n+1]->len - 1 - f])
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

int main(int argc, char *argv)
{
    int ch;
    int count;
    int f;
    Block *block[256] = {0};

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

    CompareEnds(block, count);
}
