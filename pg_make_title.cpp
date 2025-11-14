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
#include "openai/openai.hpp"

extern "C" {

// Required PostgreSQL macros
PG_MODULE_MAGIC;
PG_FUNCTION_INFO_V1(convert_to_title);

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

    Relation rel = table_open(RelationRelationId, AccessShareLock);
    elog(DEBUG1, "list_tables: opened pg_class Relation");
    TableScanDesc scan = table_beginscan(rel, SnapshotSelf, 0, NULL);
    elog(DEBUG1, "list_tables: began scan");

    std::string table_list;

    HeapTuple tuple;
    while ((tuple = heap_getnext(scan, ForwardScanDirection)) != NULL)
    {
        Form_pg_class rel_form = (Form_pg_class)GETSTRUCT(tuple);
        
        // Only include ordinary tables
        if (rel_form->relkind == RELKIND_RELATION || rel_form->relkind == RELKIND_VIEW)
        {
            const char *nspname = get_namespace_name(rel_form->relnamespace);

            // Skip system schemas
            if (strcmp(nspname, "pg_catalog") == 0 ||
                strcmp(nspname, "information_schema") == 0 )
                continue;
            
            elog(DEBUG1, "list_tables: found table %s", NameStr(rel_form->relname));
            char *relname = NameStr(rel_form->relname);

            Oid relid = RelnameGetRelid(relname);
            if (!OidIsValid(relid))
                ereport(ERROR, (errmsg("table \"%s\" does not exist", relname)));

            Relation rel = relation_open(relid, AccessShareLock);

            TupleDesc desc = RelationGetDescr(rel);
            int natts = desc->natts;
            std::string columns;
            for (int i = 0; i < natts; i++)
            {
                Form_pg_attribute attr = TupleDescAttr(desc, i);
                if (attr->attisdropped || attr->attnum <= 0)
                    continue;

                const char *colname = NameStr(attr->attname);
                const char *typename_str = format_type_be(attr->atttypid);
                std::string coldef = std::string(colname) + ":" + std::string(typename_str);
                
                if (i > 0)
                    columns += ", ";
                columns += coldef;
            }
            elog(DEBUG1, "list_tables: columns for table %s: %s", relname, columns.c_str());
            relation_close(rel, AccessShareLock);

            table_list += "{\"table_name\":\"" + std::string(relname) + "\", \"columns\":\"" + columns + "\"},\n";

            // Datum values[2];
            // bool nulls[2] = {false, false};
            // values[0] = CStringGetTextDatum(relname);
            // values[1] = CStringGetTextDatum(columns.c_str());
            // tuplestore_putvalues(tupstore, tupdesc, values, nulls);
        }
    }

    table_endscan(scan);
    table_close(rel, AccessShareLock);

    // tuplestore_donestoring(tupstore);
    // MemoryContextSwitchTo(oldcontext);

    std::string system_prompt = "You are a helpful assistant that writes SQL queries based on the following database schema.\n\nSchema: [" + table_list + "]";
    std::string user_query = "Give me the SQL for finding out " +std::string(input_cstr) + ". Do not include any explanation, comments, or text.";

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
    elog(INFO, "AI Response: %s", chat.dump(2).c_str());
    std::string sql_query = chat["choices"][0]["message"]["content"].get<std::string>();


    elog(DEBUG1, "Preparing result");

    text* result = (text *)palloc(sql_query.size() + VARHDRSZ);
    SET_VARSIZE(result, sql_query.size() + VARHDRSZ);
    memcpy(VARDATA(result), sql_query.data(), sql_query.size());
    elog(DEBUG1, "Preparing to return");
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
