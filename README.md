## README

This extension provides a natural language query interface to Postgres.

### Usage
```{sql}

CREATE EXTENSION pg_gen_query;

SELECT * FROM pg_qen_query('get all nation names');

```