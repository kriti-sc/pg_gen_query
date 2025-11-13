/*--------------------------------------------------
 * pg_make_title.c
 * -------------------------------------------------
 */
#include "postgres.h"
#include "utils/builtins.h"
#include "c.h"
 
PG_MODULE_MAGIC;
PG_FUNCTION_INFO_V1(convert_to_title);
 
Datum
convert_to_title(PG_FUNCTION_ARGS)
{
    // Get the input text argument
    text *input_text = PG_GETARG_TEXT_PP(0);
    char *input = text_to_cstring(input_text);

    // Allocate output buffer (same size as input, plus null terminator)
    size_t len = strlen(input);
    char *output = palloc(len + 1);

    bool new_word = true;
    for (size_t i = 0; i < len; i++)
    {
        char ch = input[i];

        if (isspace((unsigned char)ch) || ispunct((unsigned char)ch))
        {
            new_word = true;
            output[i] = ch;
        }
        else
        {
            if (new_word)
                output[i] = toupper((unsigned char)ch);
            else
                output[i] = tolower((unsigned char)ch);
            new_word = false;
        }
    }

    output[len] = '\0';

    // Convert back to PostgreSQL text type
    text *result = cstring_to_text(output);

    // Free allocated memory
    pfree(output);
    pfree(input);

    PG_RETURN_TEXT_P(result);
 
}
