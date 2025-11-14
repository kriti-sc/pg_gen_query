/*--------------------------------------------------
 * pg_make_title.cpp
 * A PostgreSQL extension written in C++
 * that converts text to title case.
 * -------------------------------------------------
 */
extern "C" {
#include "postgres.h"
#include "fmgr.h"
#include "funcapi.h"
#include "access/htup_details.h"
#include "access/table.h"
#include "access/heapam.h"
#include "catalog/pg_class.h"
#include "catalog/namespace.h"
#include "utils/builtins.h"
#include "utils/lsyscache.h"
#include "utils/memutils.h"
#include "utils/rel.h"
#include "utils/guc.h" 
#include "utils/snapmgr.h"

}

#include <string>
#include <cctype>

#include "api_key.hpp"
#include "utils.hpp"
#include "openai/openai.hpp"

extern "C" {

// Required PostgreSQL macros
PG_MODULE_MAGIC;
PG_FUNCTION_INFO_V1(convert_to_title);

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


// Function definition
Datum convert_to_title(PG_FUNCTION_ARGS)
{
    elog(DEBUG1, "Entering extension");

    text *input_text = PG_GETARG_TEXT_PP(0);
    char *input_cstr = text_to_cstring(input_text);
    elog(INFO, "convert_to_title called with arg: %s", input_cstr);

    // ReturnSetInfo *rsinfo = (ReturnSetInfo *)fcinfo->resultinfo;
    // if (!rsinfo) {
    //     elog(ERROR, "list_tables: fcinfo->resultinfo is NULL");
    // }


    // TupleDesc tupdesc;
    // Tuplestorestate *tupstore;
    // MemoryContext per_query_ctx;
    // MemoryContext oldcontext;
    // elog(DEBUG1, "Initializing memory context");

    // per_query_ctx = rsinfo->econtext->ecxt_per_query_memory;
    // oldcontext = MemoryContextSwitchTo(per_query_ctx);

    // tupdesc = CreateTemplateTupleDesc(2);
    // TupleDescInitEntry(tupdesc, (AttrNumber)1, "table_name", TEXTOID, -1, 0);
    // TupleDescInitEntry(tupdesc, (AttrNumber)2, "columns", TEXTOID, -1, 0);
    // tupdesc = BlessTupleDesc(tupdesc);
    // elog(DEBUG1, "list_tables: created tupledesc");

    // int maxKBytes = atoi(GetConfigOption("work_mem", false, false));
    // tupstore = tuplestore_begin_heap(true, false, maxKBytes);
    
    // if (!tupstore) {
    //     elog(ERROR, "list_tables: tuplestore_begin_heap returned NULL");
    // }
    // elog(DEBUG1, "list_tables: created tuplestore");
    
    // rsinfo->returnMode = SFRM_Materialize;
    // rsinfo->setDesc = tupdesc;
    // rsinfo->setResult = tupstore;
    // elog(DEBUG1, "configuring return set info");

    // MemoryContextSwitchTo(oldcontext);

    // --- moved to utils

    // tuplestore_donestoring(tupstore);
    // MemoryContextSwitchTo(oldcontext);

    std::string system_prompt = "You are a helpful assistant that writes SQL queries based on the following database schema.\n\nSchema: [" + std::string(GlobalTableInfoString) + "]";
    std::string user_query = "Give me the SQL for finding out " +std::string(input_cstr) + ". Do not include any explanation, comments, or text.";

    nlohmann::json chat_json = {
        {"model", "gpt-3.5-turbo"},
        {"messages", {
            {{"role", "system"}, {"content", system_prompt}},
            {{"role", "user"}, {"content", user_query}}
        }},
        {"temperature", 0}
    };

    // elog(DEBUG1, "Sending request to OpenAI: %s", chat_json.dump(2).c_str());

    openai::start(open_api_key); 
    auto chat = openai::chat().create(chat_json);
    // elog(INFO, "AI Response: %s", chat.dump(2).c_str());
    std::string sql_query = chat["choices"][0]["message"]["content"].get<std::string>();


    elog(DEBUG1, "Preparing result");

    text* result = (text *)palloc(sql_query.size() + VARHDRSZ);
    SET_VARSIZE(result, sql_query.size() + VARHDRSZ);
    memcpy(VARDATA(result), sql_query.data(), sql_query.size());
    
    PG_RETURN_TEXT_P(result);

    // return (Datum) 0;
    
    // title case extension code
    // text *input_text = PG_GETARG_TEXT_PP(0);
    // std::string input(VARDATA_ANY(input_text), VARSIZE_ANY_EXHDR(input_text));
    // std::string output;
    // output.reserve(input.size());
    // for (char ch : input)
    // {
    //     output.push_back(std::toupper(static_cast<unsigned char>(ch)));
    // }

    // // Convert std::string back to PostgreSQL text
    // text *result = (text *)palloc(output.size() + VARHDRSZ);
    // SET_VARSIZE(result, output.size() + VARHDRSZ);
    // memcpy(VARDATA(result), output.data(), output.size());

    // PG_RETURN_TEXT_P(result);
}

} // extern "C"
