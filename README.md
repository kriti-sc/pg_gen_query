## pg_gen_query

This extension provides a natural language query interface to Postgres, allowing you do to something like `SELECT * FROM pg_gen_query('number of events from Texas')`.

### Usage

Build the extension:
```
git clone git@github.com:kriti-sc/pg_gen_query.git

in `api_key.hpp`, add OpenAI api key: 
const std::string open_api_key = "sk-verylongopenaikey";

make clean && make && make install
```

Install and use:
```
CREATE EXTENSION pg_gen_query;

SELECT * FROM pg_qen_query('get all nation names');
```

> **Note:** first time execution will be slow as the extension develops an understanding of the schema on initialization. This understanding is stored therefore, subsequent executions will be faster.

### Example

Below is sample usage against the TPCH schema. 

```
tpchsf1=# select * from pg_gen_query('get all nation names');
        pg_gen_query
----------------------------
 SELECT n_name FROM nation;
(1 row)

tpchsf1=# select * from pg_gen_query('get the top 5 customers');
                        pg_gen_query
------------------------------------------------------------
 SELECT c_custkey, c_name, c_phone, c_acctbal, c_mktsegment+
 FROM customer                                             +
 ORDER BY c_acctbal DESC                                   +
 LIMIT 5;
(1 row)

```





<screenshots>


### Mechanics

This extension works by developing an "understanding" of the entire schema and then passing that understanding along with the user query to the LLM.

It first generates an "understanding" of the entire schema, i.e., all tables and all the column in each table. It then sends this understanding, along with the user’s natural-language query, to the LLM (currently OpenAI). The LLM returns a generated SQL query, which the extension then surfaces to the user.


#### Improvements
1. **Incremental schema understanding.** Currently, the extension builds a complete understanding of the entire schema on initialization. For large schemata, this would be slow and would consume significant memory on Postgres' memory stack. A more efficient approach would be to build this understanding incrementally. For ex., understanding only the tables at startup and deferring column-level details until a user query indicates which tables are relevant.

2. **Better memory management.** The extension stores its schema understanding in Postgres' `TopMemoryContext`. This memory gets pinned by the extension process and cannot be reclaimed by other processes. For larger schemata, this is suboptimal. Sophisticated memory management techniques, such as spilling portions of the understanding to disk or using a more appropriate memory context, would ensure efficiency.

### Future Direction

Broadly speaking, extensions is not the best approach for building natural language query interfaces. An external tool that interfaces with the database would be more suitable. There are three main reasons: extensions are limited to only the database they are implemented for, they require complex low-level memory management to operate within the context of the database, and executing the generated query is in the best case non-trivial and in the worst case, impossible.

We will examine these reasons in reverse order, building a case for why this use case is better served by an external tool rather than a database extension.

The core promise of a natural language query interface is the ability to "talk to your data". Picture it as follows: you can ask the database a question in natural language and receive a table of results. But in some databases like Postgres, Citus, and ClickHouse, executing arbitrary queries directly from an extension is impossible; in others, like DuckDB it is possible but non-trivial. As a result, an extension-based approach forces users to copy the generated SQL into a separate console to run it, creating a disjointed and less intuitive experience. 

However, an external tool that interfaces with the database can execute arbitrary queries trivially.

Secondly, extensions run inside the database process and consume memory from the database’s own stack. This means they must be extremely careful not to degrade database performance. For this use case, the system also needs to maintain conversational context, since data exploration is inherently iterative and interactive. Storing that context increases memory demands and heightens the need for careful memory management.

Moreover, table names, column names, and datatypes rarely capture the true semantics of the underlying data. Representing that semantic layer requires additional context and further amplifies the memory requirements—another challenge when operating inside the database process.

Not only does an external tool avoid the complexities of in-database memory management, it is also far better suited for user-facing workflows that rely on an interactive flow. Additionally, it can leverage third-party systems to manage conversational context for LLMs which is a non-trivial problem in itself.

The principles of natural language query support remains the same across databases. An external tool would not be scoped to any particular database but would work for multiple datbaes. The same principles apply, making it possible to build a generic tool that works across different database systems rather than being locked to Postgres.

The principles behind natural language query support remain the same across databases. An external tool would not be limited to a single database but could operate across multiple databases. 

Thus, an external tool can focus on functionality and user experience without being constrained by the architectural limitations of a database engine. An extension, on the other hand, would remain severely limited.
