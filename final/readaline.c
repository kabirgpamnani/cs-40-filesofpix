/*
 *     readaline.c
 *     by Kabir Pamnani and Isaac Monheit, 01/31/2023
 *     HW1: filesofpix
 *
 *     Summary: readaline.c contains a function that reads a single line of 
 *     input from a file that has already been opened
 */

#include <stdio.h>
#include <stdlib.h>
#include <assert.h>
#include "readaline.h"

/**********readaline********
 *
 * Reads a single line of input from an open file
 * Inputs:
 *      FILE *inputfd: pointer to open file
 *      char **datapp: pointer to the address of the first byte in 
 *                     the line (char array)
 * Return: number of bytes in the line
 * Expects
 *      Each line to contain at least one character
 *      New lines begin at the start of the file, 
 *      and after each newline character (‘\n’)
 *      Each newline character is included in the line that it terminates
 * Notes:
 *      readaline terminates with a CRE in each of the following cases:
 *      1) Either or both of the supplied arguments is NULL
 *      2) An error is encountered reading from the line
 *      3) Memory allocation fails
 ************************/

size_t readaline(FILE *inputfd, char **datapp) 
{
        /* cases that cause CRE */
        if (inputfd == NULL || datapp == NULL) {
                exit(1);
        }

        if (feof(inputfd)) {
                *datapp = NULL;
                return 0;
        }

        int c;
        int count = 0;

        char *line;
        line = malloc(1000 * sizeof(*line));
        assert(line != NULL);

        *datapp = line;

        while (inputfd != NULL) {
                
                if (ferror(inputfd)) {
                        exit(EXIT_FAILURE);
                }
                c = fgetc(inputfd);
                
                line[count] = c;
                count++;

                if((c == '\n') || feof(inputfd)) {
                        break;
                }

                /* case that causes CRE */
                if (count >= 1000) {
                        fprintf(stderr, "%s\n", 
                                        "readaline: input line too long");
                        exit(4);
                }
        }
        
        /* returns number of bytes in the line */
        return count;
}  