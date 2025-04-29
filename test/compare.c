/*

    casm - Simple, portable assembler

    Copyright (C) 2003-2025  Ian Cowburn (ianc@noddybox.co.uk)

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

    Test compare utility

*/
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <ctype.h>
#include <errno.h>

static int StrEqual(const char *a, const char *b)
{
    while(*a && tolower((unsigned char)*a) == tolower((unsigned char)*b))
    {
        a++;
        b++;
    }

    return tolower((unsigned char)*a) == tolower((unsigned char)*b);
}


static char *Chomp(char *p)
{
    size_t l = strlen(p);

    while(l && p[l - 1] == '\n')
    {
        p[--l] = 0;
    }

    return p;
}


static int Tokenise(char *p, char *tokens[], int max_tokens)
{
    int no = 0;

    char *t = strtok(p, " \t\n");

    while(t && no < max_tokens)
    {
        tokens[no++] = t;
        t = strtok(NULL, " \t\n");
    }

    return no;
}


static void OutputTokens(int no, char *tokens[])
{
    int f;

    for(f = 0; f < no; f++)
    {
        printf(" %s", tokens[f]);
    }
}


static void ReportDifference(int no_source_tokens, char *source_tokens[],
                             int no_output_tokens, char *output_tokens[])
{
    printf("Difference found\n");
    printf("Source:");
    OutputTokens(no_source_tokens, source_tokens);
    printf("\nOutput:");
    OutputTokens(no_output_tokens, output_tokens);
    printf("\n");
}


int main(int argc, char *argv[])
{
    FILE *source;
    FILE *output;
    char source_buff[1024];
    char output_buff[1024];
    int start = 0;
    char *source_tokens[10];
    char *output_tokens[10];
    int no_source_tokens;
    int no_output_tokens;
    int difference_found = 0;

    if (argc != 3)
    {
        fprintf(stderr, "%s: usage %s asm_file output_file\n",
                                                argv[0], argv[0]);
        return EXIT_FAILURE;
    }

    if (!(source = fopen(argv[1], "r")))
    {
        perror(argv[1]);
        return EXIT_FAILURE;
    }

    if (!(output = fopen(argv[2], "r")))
    {
        perror(argv[2]);
        return EXIT_FAILURE;
    }

    while(!start && fgets(source_buff, sizeof source_buff, source))
    {
        Chomp(source_buff);
        no_source_tokens = Tokenise(source_buff, source_tokens, 10);

        if (no_source_tokens == 2 && StrEqual(source_tokens[0], ";") &&
                                        StrEqual(source_tokens[1], "START"))
        {
            start = 1;
        }
    }

    while(fgets(source_buff, sizeof source_buff, source))
    {
        Chomp(source_buff);

        if (fgets(output_buff, sizeof output_buff, output))
        {
            no_source_tokens = Tokenise(source_buff, source_tokens, 10);
            no_output_tokens = Tokenise(output_buff, output_tokens, 10);

            if (no_source_tokens != no_output_tokens)
            {
                ReportDifference(no_source_tokens, source_tokens,
                                 no_output_tokens, output_tokens);

                difference_found = 1;
            }
            else
            {
                int f;
                int ok = 1;

                for(f = 0; f < no_source_tokens; f++)
                {
                    ok = ok && StrEqual(source_tokens[f], output_tokens[f]);
                }

                if (!ok)
                {
                    ReportDifference(no_source_tokens, source_tokens,
                                     no_output_tokens, output_tokens);

                    difference_found = 1;
                }
            }
        }
        else
        {
            printf("Extra source line: %s\n", source_buff);
            difference_found = 1;
        }
    }

    while (fgets(output_buff, sizeof output_buff, output))
    {
        printf("Extra output line: %s\n", output_buff);
        difference_found = 1;
    }

    fclose(source);
    fclose(output);

    return difference_found ? EXIT_FAILURE : EXIT_SUCCESS;
}

/*
vim: ai sw=4 ts=8 expandtab
*/
