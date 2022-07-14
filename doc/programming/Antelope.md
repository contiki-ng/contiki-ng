# Antelope

[Antelope](http://dl.acm.org/citation.cfm?id=2070974) is a lightweight, relational database management system for resource-constrained IoT devices. It has been designed to run on top of a file system such as [Coffee](/doc/programming/Coffee).

## Programming Antelope

When using Antelope in Contiki-NG, one typically writes the query and processing functionality at the top level in a process, so that one can yield control back to the scheduler cooperatively. In what follows, we explain the relevant parts of the example found in `contiki-ng/examples/storage/antelope-shell/shell-db.c`.

At least two different variables are needed to issue queries and process their results: a handle of type `db_handle_t` and a result of type `db_result_t`. The handle variable is used to be able to refer to a single query over multiple functions calls to Antelope. The result variable stores the result of a database query, indicating whether it succeeded or failed. Since the handle typically needs to be kept over multiple Contiki-NG process schedulings, we make it static rather than to store it on the stack.

```c
  static db_handle_t handle;
  db_result_t result;
```

The database must be initialized before using it for the first time after the OS started:

```c
  db_init();
```

After that, queries can be submitted by using the `db_query()` function. The first argument is the aforementioned handle, whereas the second argument is a string expressed in the Antelope Query Language (AQL). Handles must be manually freed by the user after each query.

```c
  for(;;) {
    result = db_query(&handle, aql_string);
    if(DB_ERROR(result)) {
      printf("Query \"%s\" failed: %s\n",
             (char *)aql_string, db_get_result_message(result));
      db_free(&handle);
      continue;
    }

    if(!db_processing(&handle)) {
      printf("OK\n");
      continue;
    }

    db_print_header(&handle);

    /* The processing loop shown below can be inserted here. */
  }
```

The processing loop after a query has been issued can look as follows. Each time a tuple has been processed by Antelope, it yields control back to the caller so that the caller, in turn, can yield control back to the scheduler cooperatively. This is required in operating systems such as Contiki-NG.

```c
    while(db_processing(&handle)) {
      PROCESS_PAUSE();
      result = db_process(&handle);
      switch(result) {
      case DB_GOT_ROW:
        /* The processed tuple matched the condition in the query. */
        db_print_tuple(&handle);
        break;
      case DB_OK:
        /* A tuple was processed, but did not match the condition. */
        continue;
      case DB_FINISHED:
        /* The processing has finished. */
        printf("OK\n");
      default:
        if(DB_ERROR(result)) {
          printf("Processing error: %s\n", db_get_result_message(result));      
        }
        db_free(&handle);
        break;
      }
    }
  }
```

## Antelope Query Language (AQL)

AQL is a language that has some syntactic similarities with the Structured Query Language (SQL), but it is a considerably smaller language in order to make it simple to implement for resource-constrained embedded systems. Antelope includes a parses that can compile queries into an efficient binary format that is partly interpreted in an application-specific virtual machine called LVM.

Next, we will show how to create a database, add an index, insert data, and perform queries on the inserted data. For this example, we use data from _Old Faithful_, a geyser in Yellowstone, collected in the year 2000. These commands can be entered by executing the example in `contiki-ng/examples/storage/antelope-shell/`, or one can create a new application that enters them in some other way such as reading them from a file or receiving them over the network.

### Creating a database

We create a database called _faithful_, containing two attributes of the domain (i.e., type) `LONG`.

```
CREATE RELATION faithful;
CREATE ATTRIBUTE eruption DOMAIN LONG IN faithful;
CREATE ATTRIBUTE recharge DOMAIN LONG IN faithful;
```

### Adding an index

To make queries with conditions involving the `eruption` attribute faster, we create an index for this attribute as follows.

```
CREATE INDEX faithful.eruption TYPE INLINE;
```

We selected an index of type `INLINE` here because this is the fastest index for data that is inserted in a monotonically increasing order. The `INLINE` index does not store any index data itself in the underlying file system, but instead simply performs a binary search over the attribute values.

In case the data would be inserted in an arbitrary order, we would have to use a `MAXHEAP` index instead.

### Inserting data

After having added an index, we are ready to insert values into the database relation. The values within the parentheses of each `INSERT` line denote are inserted in the same order as they have been defined in the relation. In this case, the first value denotes the `eruption` attribute and the second value denotes the `recharge` attribute.

```
INSERT (0, 4860) INTO faithful;
INSERT (4860, 4980) INTO faithful;
INSERT (9840, 5340) INTO faithful;
INSERT (15180, 4860) INTO faithful;
INSERT (20040, 5400) INTO faithful;
INSERT (25440, 4860) INTO faithful;
INSERT (30300, 4980) INTO faithful;
INSERT (35280, 5700) INTO faithful;
INSERT (40980, 5760) INTO faithful;
INSERT (46740, 5460) INTO faithful;
INSERT (52200, 5220) INTO faithful;
INSERT (57420, 5400) INTO faithful;
INSERT (62820, 5700) INTO faithful;
INSERT (68520, 5100) INTO faithful;
INSERT (73620, 5100) INTO faithful;
INSERT (78720, 5280) INTO faithful;
INSERT (84000, 4920) INTO faithful;
INSERT (88920, 5520) INTO faithful;
INSERT (94440, 5400) INTO faithful;
INSERT (99840, 3240) INTO faithful;
INSERT (103080, 6060) INTO faithful;
```

### Querying the database

Next, we can issue various queries on the database relation. For example, the following `SELECT` query first specifies
that we are querying for `recharge` and `eruption` attribute values. After the `FROM` keyword, we write the name of the database relation to query. After the `WHERE` keyword, we write a logical condition that might contain multiple subconditions connected through keywords such as `AND` and `OR`. The logical condition is the part that is compiled to LVM bytecode for efficient execution, as mentioned above. The query is terminated by a semicolon character.

```
SELECT recharge, eruption FROM faithful WHERE recharge > 5000 AND eruption >= 60000 AND eruption < 90000;
```