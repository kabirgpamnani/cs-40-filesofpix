/*     restoration.c
 *     by Kabir Pamnani and Isaac Monheit, 01/31/2023
 *     HW1: filesofpix
 *
 *     Summary: restoration.c reads in a corrupted .pgm from a file or from 
 *              stdin and prints out the restored .pgm file in a format that 
 *              the pgm reader module, Pnmrdr, can read. It uses the 
 *              implementation of readaline.c to read input from a file.
 * 
 */

#include <stdio.h>
#include <stdlib.h>
#include <assert.h>
#include <ctype.h>
#include <stdbool.h>
#include "readaline.h"
#include "table.h"
#include "list.h"
#include "mem.h"
#include "atom.h"

List_T original_list(FILE *inputfd, int *width);

int remove_digits(char *one_line, size_t line_size, char **no_digits); 
void find_original_rows(Table_T injected_table, List_T *original_rows, 
                        char *no_digits, char *value, int no_digits_length); 
char *duplicate_char_array(char *array, size_t length);
char *convert_to_ASCII_digits(char *one_line, int *width);
List_T insert_ASCII_digits(List_T original_rows, int *width);  

void print_output(List_T final_list, int line_length);
void print_line(char *one_line, int line_length);

static void vfree(const void *key, void **value, void *cl);

int char_count (char *first_line);

int main(int argc, char *argv[]) 
{
        
        /* making sure the correct number of args has been provided */
        FILE *inputfd;
         
        if (argc == 1) {
                inputfd = stdin;
        }
        else if (argc > 2) {
                printf("One argument expected.\n");
                return EXIT_FAILURE;
        } else { 
                inputfd = fopen(argv[1], "r");
                assert(inputfd != NULL);
        }

        /*
         * A general width value that will be equal to the width of one row of
         * the pgm file. This int is passed by reference between the two main
         * functions and is changed a number of times to maintain the current
         * state of the width of one row
         */
        int width = 0;

        List_T org_list = original_list(inputfd, &width);
        List_T final_list = insert_ASCII_digits(org_list, &width);
        int final_list_len = List_length(final_list);
        
        /* Printing the correct pgm file to stdout */
        printf("P5 %i %i %i\n", width, final_list_len, 255);
        print_output(final_list, width);

        fclose(inputfd);
        exit(EXIT_SUCCESS);
}  

/**********original_list********
 *
 * Creates a list of the non-injected char arrays from a corrupted .pgm file
 * Inputs:
 *              FILE *inputfd: pointer to open file
 *              int *width: pointer to the largest length of one single char 
 *                          array in the list that will be passed by reference
 *                          (stored for use in other functions outside of this
 *                           one)
 * Return: A list of all the original char arrays within the corrupted file
 * Expects
 *      inputfd is non-null
 * Notes:
 *      Is the first function called in main after the file is opened
 ************************/
List_T original_list(FILE *inputfd, int *width)
{
        /* datapp, will point to one row of the file */
        char *dpp;

        size_t curr_line;
        char *no_digits;

        Table_T injected_table = Table_new(10, NULL, NULL);
        List_T original_rows = List_list(NULL);

        while (!(feof(inputfd))) {

                curr_line = readaline(inputfd, &dpp);
                if ((int)curr_line > *width) {
                        *width = (int)curr_line;
                }

                int no_dig_len = remove_digits(dpp, curr_line, &no_digits);
                
                find_original_rows(injected_table, &original_rows, no_digits, 
                                   dpp, no_dig_len);
           
                free(no_digits);        
        }

        /* free up all the memory in the table */
        Table_map(injected_table, vfree, NULL);
        Table_free(&injected_table);

        return original_rows;
}

/**********vfree********
 *
 * Frees up space of one value within a Hanson Table
 * Inputs:
 *      const void *key: pointer to a key in the Table (not used)
 *      void **value: a pointer to the address of corresponding value to the
 *                    key in the table
 *      void *cl: pointer to the closure variable (not used)
 * Return: void
 * Expects
 *      To be used as a function in the parameters of Table_map()
 * Notes:
 *      To be used in conjunction with Table_map(), frees up all memory within
 *          the Table
 *      Implementation of this function is taken from Hanson
 ************************/
static void vfree(const void *key, void **value, void *cl) 
{
        (void)key;
        (void)cl;
        FREE(*value);
}

/**********remove_digits********
 *
 *      Removes all of the digits from a char array
 * Inputs:
 *      char *one_line: one line of the corrupted pgm file, taken from 
 *                      readaline, in the form of a char array
 *      size_t line_size: the length of one_line
 *      char **no_digits: a pointer to the address of a char array that will
 *                        store the result of remove_digits 
 *                        (call by reference)
 * Return: the length of the nondigit char array
 * Expects
 *      line_size is the exact length of one_line
 * Notes:
 *      Used within original_list
 ************************/
int remove_digits(char *one_line, size_t line_size, char **no_digits) 
{
        *no_digits = calloc(line_size, sizeof(char));
        assert(*no_digits != NULL);

        int count = 0;

        for (size_t i = 0; i < line_size; i++) {
                if (!(isdigit(one_line[i]))) {
                        (*no_digits)[count] = one_line[i];
                        count++;
                }
        }
        return count;
}

/**********find_original_rows********
 *
 *      Finds which rows of the corrupted pgm are original rows of the correct 
 *      image and puts them all into a List
 * Inputs:
 *      Table_T injected_table:A table where all of the injected files will go.
 *                         This table allows the find_original_rows to 
 *                         distinguish which lines are original and which are 
 *                         injected.
 *      List_T *original_rows: A pointer to the address of a list that stores
 *                             all the original rows
 *      char *no_digits: One array of all the nondigit characters in *value, 
 *                       to be used as a key when indexing in the table
 *      char *value: One array of characters, used as a value in the table or 
 *                   inserted into the list, depending on whether it is an 
 *                   injected row or original row
 *      int no_digits_len: The length of no_digits
 * Return: void
 * Expects
 *      The Table_T and List_T have already been created and are empty
 *      no_digits_len is the exact length of *no_digits 
 * Notes:
 *      Used within original_list after remove_digits
 ************************/
void find_original_rows(Table_T injected_table, List_T *original_rows, 
                        char *no_digits, char *value, int no_digits_len) 
{
        const char *key = Atom_new(no_digits, no_digits_len);

        /* case where key is not found in table */
        if (Table_get(injected_table, key) == NULL) {
                Table_put(injected_table, key, value);
                
        /* case where key is found in table*/
        } else {

                /* first time inserting into the list */
                if (List_length(*original_rows) == 0) {

        
                        /* duplicate the line that was in the Table */
                        int first_line_len = 
                                char_count(Table_get(injected_table, key));

                        char *first_line;
                        first_line = duplicate_char_array
                                        (Table_get(injected_table, key),
                                         first_line_len); 

                        /* 
                         * add both the Table line and the current line 
                         * to list 
                         */
                        *original_rows = List_push(*original_rows, first_line);
                        *original_rows = List_push(*original_rows, value);

                } else {
                        *original_rows = List_push(*original_rows, value);

                }
        }   
}

/**********char_count********
 *
 * Calculates the number of characters in the first original line 
 * Inputs:
 *      char *first_line: takes a pointer to a char array which is the first 
 *                        original uncorrupted image row
 * Return: number of characters/bytes in that line
 * Expects
 *      N/A
 * Notes:
 *      This function is used for one specific case. In the case where the key
 *      has been found in the table (in fn find_original_rows), but nothing
 *      has been added to the list yet, we needed to calculate the length of 
 *      that specific line, so that we could reate a copy of this specific char 
 *      array. 
 ************************/
int char_count (char *first_line)
{
        int count = 1;
        while (*first_line != '\n') {
                count++;
                first_line++;
        }
        return count;
}

/**********duplicate_char_array********
 *
 * Creates an exact copy of a char array
 * Inputs:
 *      char *array: any char array
 *      size_t length: the length of *array
 * Return: The pointer to the head of the newly duplicated char array
 * Expects
 *      N/A
 * Notes:
 *      This function is used for one specific case along with char_count.
 *      In the case where the key has been found in the table 
 *      (in fn find_original_rows), but nothing has been added to the list yet, 
 *      we needed to duplicate the array in the table that we found the
 *      matching key for, in order to not remove it from memory preemptively 
 *      when we deallocate the Table
 ************************/
char *duplicate_char_array(char *array, size_t length) {
        
        char *dupe;
        dupe = calloc(length, sizeof(char));
        assert(dupe != NULL);

        for (size_t i = 0; i < length; i++) {
                dupe[i] = array[i];
        }
        return dupe;
}

/**********convert_to_ASCII_digits********
 *
 * Converts each original image row to its corresponding ASCII digits by 
 * collecting each digit and storing it as its ASCII value
 * Inputs:
 *      char *one_line: pointer to a single char array (which is one line)
 *      int *width: int pointer to the length/width of each original image row
 * Return: a pointer to a char array that contains only the digits from the 
 *         original image row, converted to its ASCII value
 * Expects
 *      N/A
 * Notes:
 *      Contains 3 different conditions when determining whether the each char 
 *      is a digit or not:
 *      1) Checks if there are 3 digits next to each other. If so, concatenates 
 *      them and converts the combined value into its ASCII value
 *      2) Checks if there are 2 digits next to each other. If so, concatenates 
 *      them and converts the combined value into its ASCII value
 *      3) Checks if there is a digit on its own. If so, directly converts it 
 *      to its ASCII value
 ************************/
char *convert_to_ASCII_digits(char *one_line, int *width) 
{
        char *ASCII_line;
        ASCII_line = malloc(*width);
        assert(ASCII_line != NULL);

        int total = 0, ASCII_index = 0;
        
        while (*one_line != '\n') {
                if (isdigit(*one_line) && isdigit(*(one_line + 1)) 
                && isdigit(*(one_line + 2))) {
                        
                        total = ((*one_line - '0') * 100) +
                                ((*(one_line + 1) - '0') * 10) +
                                (*(one_line + 2) - '0'); 

                        ASCII_line[ASCII_index] = total;
                        ASCII_index++;

                        one_line += 3;

                } else if (isdigit(*one_line) && isdigit(*(one_line + 1))) {

                        total = ((*one_line - '0') * 10) +
                                (*(one_line + 1) - '0'); 

                        ASCII_line[ASCII_index] = total;
                        ASCII_index++;

                        one_line += 2;

                } else if (isdigit(*one_line)) {
                        
                        total = (*one_line - '0'); 

                        ASCII_line[ASCII_index] = total;
                        ASCII_index++;
                        
                        one_line++;

                } else {
                        one_line++;
                }
                        

        }
        *width = ASCII_index;
        return ASCII_line;
} 

/**********insert_ASCII_digits********
 *
 * Inserts each line (in its ASCII digit format) into a new list which will 
 * serve as the list that contains the final, uncorrupted image rows
 * Inputs:
 *      List_T original_rows: List containing the original image rows 
 *                            (includes non-digit characters)
 *      int *width: int pointer to the length/width of each original image row
 * Return: a list containing the final image rows, where each element is an 
 *         image row (excluding the non-digits) in its ASCII format
 * Expects
 *      N/A
 * Notes:
 *      Memory is allocated for the list at the start of function
 *      Memory is freed element by element after each element of the list 
 *      original_rows is pushed to the list ASCII_list
 ************************/
List_T insert_ASCII_digits(List_T original_rows, int *width)
{
        /* creates and allocates memory for a new list */
        List_T ASCII_list = List_list(NULL);

        int count = 0;

        while (original_rows != NULL) {
                
                /* pushes each ASCII converted original image row to new list*/
                ASCII_list = List_push(ASCII_list, 
                        convert_to_ASCII_digits(original_rows->first, width));
                free(original_rows->first);
                original_rows = List_pop(original_rows, &original_rows->first);
                count++;
        }
        return ASCII_list;

}

/**********print_output********
 *
 * Prints the raster of height rows in appropriate "P5" PGM format 
 * Inputs:
 *      List_T final_list: list containing the uncorrupted image row in ASCII
 *                         format
 *      int line_length: length of each uncorrupted image row
 * Return: void
 * Expects
 *      list to contain only ASCII digits
 * Notes:
 *      Called in main to output final PGM format
 ************************/
void print_output(List_T final_list, int line_length) 
{
        while (final_list != NULL) {
                print_line(final_list->first, line_length);
                free(final_list->first);
                final_list = List_pop(final_list, &final_list->first);
        }
}

/**********print_line********
 *
 * Prints each character in a single line
 * Inputs:
 *      char *one_line: pointer to a single char array
 *      int line_length: length of the char array
 * Return: void
 * Expects
 *      N/A
 * Notes:
 *      serves as a helper function, called repetitively to print final output 
 *      of uncorrupted pgm image
 ************************/
void print_line(char *one_line, int line_length) 
{
        for (int i = 0; i < line_length; i++) {
                printf("%c", one_line[i]);
        }
}




