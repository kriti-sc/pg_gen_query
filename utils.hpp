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

std::string collect_tables_and_columns() {
    Relation rel = table_open(RelationRelationId, AccessShareLock);
    TableScanDesc scan = table_beginscan(rel, SnapshotSelf, 0, NULL);

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
        }
    }

    table_endscan(scan);
    table_close(rel, AccessShareLock);

    return table_list;
}