/*--------------------------------------------------
 * pg_gen_query.cpp
 * A PostgreSQL extension written in C++
 * -------------------------------------------------
 */
extern "C" {
#include "postgres.h"
}

#include <string>
#include <cctype>

#include "api_key.hpp"
#include "utils.hpp"
#include "openai/openai.hpp"

extern "C" {

// Required PostgreSQL macros
PG_MODULE_MAGIC;
PG_FUNCTION_INFO_V1(pg_gen_query);

static char *GlobalTableInfoString = NULL;

/* Runs automatically when extension is loaded */
void
_PG_init(void)
{
    if (GlobalTableInfoString == NULL)
    {
        std::string data = collect_tables_and_columns();

        MemoryContext old = MemoryContextSwitchTo(TopMemoryContext);
        GlobalTableInfoString = pstrdup(data.c_str());
        MemoryContextSwitchTo(old);
    }
}

Datum pg_gen_query(PG_FUNCTION_ARGS)
{
    text *input_text = PG_GETARG_TEXT_PP(0);
    char *input_cstr = text_to_cstring(input_text);
    elog(DEBUG1, "pg_gen_query called with arg: %s", input_cstr);

    std::string system_prompt = "You are a helpful assistant that writes SQL queries based on the following database schema.\n\nSchema: [" + std::string(GlobalTableInfoString) + "]";
    std::string user_query = "Give me the SQL for finding out " +std::string(input_cstr) + ". Remove all newlines and formatting in the query. Do not include any explanation, comments, or text.";

    nlohmann::json chat_json = {
        {"model", "gpt-3.5-turbo"},
        {"messages", {
            {{"role", "system"}, {"content", system_prompt}},
            {{"role", "user"}, {"content", user_query}}
        }},
        {"temperature", 0}
    };

    elog(DEBUG1, "Sending request to OpenAI: %s", chat_json.dump(2).c_str());
    openai::start(open_api_key); 
    auto chat = openai::chat().create(chat_json);
    elog(DEBUG1, "AI Response: %s", chat.dump(2).c_str());
    std::string sql_query = chat["choices"][0]["message"]["content"].get<std::string>();


    elog(DEBUG1, "Preparing result");
    text* result = (text *)palloc(sql_query.size() + VARHDRSZ);
    SET_VARSIZE(result, sql_query.size() + VARHDRSZ);
    memcpy(VARDATA(result), sql_query.data(), sql_query.size());

    PG_RETURN_TEXT_P(result);
}

} // extern "C"
