/*--------------------------------------------------
 * pg_make_title.cpp
 * A PostgreSQL extension written in C++
 * that converts text to title case.
 * -------------------------------------------------
 */
extern "C" {
#include "postgres.h"
#include "fmgr.h"
#include "utils/builtins.h"
}

#include <string>
#include <cctype>

extern "C" {

// Required PostgreSQL macros
PG_MODULE_MAGIC;
PG_FUNCTION_INFO_V1(convert_to_title);

// Function definition
Datum convert_to_title(PG_FUNCTION_ARGS)
{
    // Get PostgreSQL text argument and convert to std::string
    text *input_text = PG_GETARG_TEXT_PP(0);
    std::string input(VARDATA_ANY(input_text), VARSIZE_ANY_EXHDR(input_text));

    std::string output;
    output.reserve(input.size());

    // bool new_word = true;

    for (char ch : input)
    {
        output.push_back(std::toupper(static_cast<unsigned char>(ch)));
        // if (std::isspace(static_cast<unsigned char>(ch)) || std::ispunct(static_cast<unsigned char>(ch)))
        // {
        //     new_word = true;
        //     output.push_back(ch);
        // }
        // else
        // {
        //     if (new_word)
        //         output.push_back(std::toupper(static_cast<unsigned char>(ch)));
        //     else
        //         output.push_back(std::tolower(static_cast<unsigned char>(ch)));
        //     new_word = false;
        // }
    }

    // Convert std::string back to PostgreSQL text
    text *result = (text *)palloc(output.size() + VARHDRSZ);
    SET_VARSIZE(result, output.size() + VARHDRSZ);
    memcpy(VARDATA(result), output.data(), output.size());
    
    PG_RETURN_TEXT_P(result);
}

} // extern "C"
