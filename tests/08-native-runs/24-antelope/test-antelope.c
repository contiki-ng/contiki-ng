/*
 * Copyright (c) 2025, RISE Research Institutes of Sweden.
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 * 3. Neither the name of the Institute nor the names of its contributors
 *    may be used to endorse or promote products derived from this software
 *    without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE INSTITUTE AND CONTRIBUTORS ``AS IS'' AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED.  IN NO EVENT SHALL THE INSTITUTE OR CONTRIBUTORS BE LIABLE
 * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS
 * OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 */

/**
 * \file
 *         Unit tests for the Antelope database management system.
 * \author
 *         Nicolas Tsiftes <nicolas.tsiftes@ri.se>
 */

#include "contiki.h"
#include "cfs/cfs.h"
#include "cfs/cfs-coffee.h"

#include "antelope.h"
#include "attribute.h"

#include "unit-test/unit-test.h"

#include <stdio.h>
#include <string.h>

/* Log configuration */
#include "sys/log.h"
#define LOG_MODULE "test-antelope"
#define LOG_LEVEL LOG_LEVEL_NONE

/*---------------------------------------------------------------------------*/
PROCESS(test_process, "Antelope test process");
AUTOSTART_PROCESSES(&test_process);

/*
 * A captured result set. The generic query helpers below store the rows
 * that a SELECT/JOIN produced here, so that individual tests can inspect
 * them with ordinary assertions rather than driving the iterator inline.
 */
#define MAX_RESULT_ROWS 16
#define MAX_RESULT_COLS 6

struct result_cell {
  domain_t domain;
  long l;
  char s[DB_MAX_ELEMENT_SIZE];
};

static struct result_cell result_set[MAX_RESULT_ROWS][MAX_RESULT_COLS];
static int result_rows;
static int result_cols;

/*---------------------------------------------------------------------------*/
/*
 * Execute a query that is not expected to return a result set (DDL, INSERT,
 * REMOVE). If the query does start processing (e.g. REMOVE ... WHERE), the
 * iterator is drained so that the operation takes full effect. Returns the
 * outcome as a db_result_t, mapping a successfully drained iteration to DB_OK.
 */
static db_result_t
exec_query(const char *query)
{
  static db_handle_t handle;
  db_result_t result;

  result = db_query(&handle, "%s", query);
  if(DB_ERROR(result)) {
    return result;
  }

  if(db_processing(&handle)) {
    do {
      result = db_process(&handle);
    } while(result != DB_FINISHED && !DB_ERROR(result));
    db_free(&handle);
    if(DB_ERROR(result)) {
      return result;
    }
    result = DB_OK;
  }

  return result;
}
/*---------------------------------------------------------------------------*/
/*
 * Execute a SELECT/JOIN query and capture the resulting rows into the global
 * result_set/result_rows/result_cols variables. Returns DB_OK on success or
 * a negative db_result_t on failure.
 */
static db_result_t
select_query(const char *query)
{
  static db_handle_t handle;
  db_result_t result;
  attribute_value_t value;
  int col;

  result_rows = 0;
  result_cols = 0;

  result = db_query(&handle, "%s", query);
  if(DB_ERROR(result)) {
    return result;
  }

  if(!db_processing(&handle)) {
    /* The query completed without producing a result set. */
    return DB_OK;
  }

  result_cols = handle.ncolumns;

  for(;;) {
    result = db_process(&handle);
    if(result == DB_GOT_ROW) {
      if(result_rows < MAX_RESULT_ROWS) {
        for(col = 0; col < handle.ncolumns && col < MAX_RESULT_COLS; col++) {
          if(db_get_value(&value, &handle, col) != DB_OK) {
            continue;
          }
          result_set[result_rows][col].domain = value.domain;
          if(value.domain == DOMAIN_STRING) {
            strncpy(result_set[result_rows][col].s,
                    (const char *)VALUE_STRING(&value),
                    DB_MAX_ELEMENT_SIZE - 1);
            result_set[result_rows][col].s[DB_MAX_ELEMENT_SIZE - 1] = '\0';
          } else {
            result_set[result_rows][col].l = db_value_to_long(&value);
          }
        }
      }
      result_rows++;
    } else if(result == DB_OK) {
      /* The tuple was processed but did not match the condition. */
      continue;
    } else {
      /* DB_FINISHED or an error. */
      break;
    }
  }

  db_free(&handle);

  if(DB_ERROR(result)) {
    return result;
  }

  return DB_OK;
}
/*---------------------------------------------------------------------------*/
static long
cell_long(int row, int col)
{
  return result_set[row][col].l;
}
/*---------------------------------------------------------------------------*/
static const char *
cell_str(int row, int col)
{
  return result_set[row][col].s;
}
/*---------------------------------------------------------------------------*/
/*
 * Reset persistent state and create the "students" relation used by several
 * tests, populating it with a fixed set of rows.
 *
 *   id (INT)  score (LONG)  name (STRING)
 *   1         90            Alice
 *   2         75            Bob
 *   3         60            Carol
 *   4         85            Dave
 *   5         95            Eve
 */
static int
setup_students(void)
{
  if(cfs_coffee_format() != 0) {
    return 0;
  }
  db_init();

  if(DB_ERROR(exec_query("CREATE RELATION students;"))) {
    return 0;
  }
  if(DB_ERROR(exec_query("CREATE ATTRIBUTE id DOMAIN INT IN students;"))) {
    return 0;
  }
  if(DB_ERROR(exec_query("CREATE ATTRIBUTE score DOMAIN LONG IN students;"))) {
    return 0;
  }
  if(DB_ERROR(exec_query("CREATE ATTRIBUTE name DOMAIN STRING(16) IN students;"))) {
    return 0;
  }

  if(DB_ERROR(exec_query("INSERT (1, 90, 'Alice') INTO students;")) ||
     DB_ERROR(exec_query("INSERT (2, 75, 'Bob') INTO students;")) ||
     DB_ERROR(exec_query("INSERT (3, 60, 'Carol') INTO students;")) ||
     DB_ERROR(exec_query("INSERT (4, 85, 'Dave') INTO students;")) ||
     DB_ERROR(exec_query("INSERT (5, 95, 'Eve') INTO students;"))) {
    return 0;
  }

  return 1;
}
/*---------------------------------------------------------------------------*/
UNIT_TEST_REGISTER(schema_and_insert, "Create schema, insert and read tuples");
UNIT_TEST(schema_and_insert)
{
  UNIT_TEST_BEGIN();

  UNIT_TEST_ASSERT(setup_students());

  /* Read every tuple back in insertion order. */
  UNIT_TEST_ASSERT(select_query("SELECT id, score, name FROM students;") == DB_OK);
  UNIT_TEST_ASSERT(result_rows == 5);
  UNIT_TEST_ASSERT(result_cols == 3);

  UNIT_TEST_ASSERT(cell_long(0, 0) == 1);
  UNIT_TEST_ASSERT(cell_long(0, 1) == 90);
  UNIT_TEST_ASSERT(strcmp(cell_str(0, 2), "Alice") == 0);

  UNIT_TEST_ASSERT(cell_long(2, 0) == 3);
  UNIT_TEST_ASSERT(cell_long(2, 1) == 60);
  UNIT_TEST_ASSERT(strcmp(cell_str(2, 2), "Carol") == 0);

  UNIT_TEST_ASSERT(cell_long(4, 0) == 5);
  UNIT_TEST_ASSERT(cell_long(4, 1) == 95);
  UNIT_TEST_ASSERT(strcmp(cell_str(4, 2), "Eve") == 0);

  UNIT_TEST_END();
}
/*---------------------------------------------------------------------------*/
UNIT_TEST_REGISTER(where_conditions, "Filter tuples with WHERE conditions");
UNIT_TEST(where_conditions)
{
  UNIT_TEST_BEGIN();

  UNIT_TEST_ASSERT(setup_students());

  /* Greater-than: scores 90, 85, 95 -> ids 1, 4, 5 (insertion order). */
  UNIT_TEST_ASSERT(select_query("SELECT score FROM students WHERE score > 80;")
                   == DB_OK);
  UNIT_TEST_ASSERT(result_rows == 3);
  UNIT_TEST_ASSERT(cell_long(0, 0) == 90);
  UNIT_TEST_ASSERT(cell_long(1, 0) == 85);
  UNIT_TEST_ASSERT(cell_long(2, 0) == 95);

  /* Range with AND: 75 <= score <= 90 -> 90, 75, 85. */
  UNIT_TEST_ASSERT(select_query(
                   "SELECT score FROM students WHERE score >= 75 AND score <= 90;")
                   == DB_OK);
  UNIT_TEST_ASSERT(result_rows == 3);

  /* Equality. */
  UNIT_TEST_ASSERT(select_query("SELECT score FROM students WHERE score = 60;")
                   == DB_OK);
  UNIT_TEST_ASSERT(result_rows == 1);
  UNIT_TEST_ASSERT(cell_long(0, 0) == 60);

  /* Not-equal (<>): everything except the score of 60. */
  UNIT_TEST_ASSERT(select_query("SELECT score FROM students WHERE score <> 60;")
                   == DB_OK);
  UNIT_TEST_ASSERT(result_rows == 4);

  /* Disjunction. */
  UNIT_TEST_ASSERT(select_query(
                   "SELECT score FROM students WHERE score = 60 OR score = 95;")
                   == DB_OK);
  UNIT_TEST_ASSERT(result_rows == 2);

  /* A condition that matches nothing. */
  UNIT_TEST_ASSERT(select_query("SELECT score FROM students WHERE score > 1000;")
                   == DB_OK);
  UNIT_TEST_ASSERT(result_rows == 0);

  UNIT_TEST_END();
}
/*---------------------------------------------------------------------------*/
UNIT_TEST_REGISTER(aggregates,
                   "Aggregate functions COUNT, SUM, MAX, MIN, MEAN and MEDIAN");
UNIT_TEST(aggregates)
{
  UNIT_TEST_BEGIN();

  UNIT_TEST_ASSERT(setup_students());

  /* Aggregates over the INT attribute "id" (values 1..5). */

  /* COUNT over all tuples. */
  UNIT_TEST_ASSERT(select_query("SELECT COUNT(id) FROM students;") == DB_OK);
  UNIT_TEST_ASSERT(result_rows == 1);
  UNIT_TEST_ASSERT(cell_long(0, 0) == 5);

  /* SUM (1+2+3+4+5). */
  UNIT_TEST_ASSERT(select_query("SELECT SUM(id) FROM students;") == DB_OK);
  UNIT_TEST_ASSERT(result_rows == 1);
  UNIT_TEST_ASSERT(cell_long(0, 0) == 15);

  /* MAX and MIN in a single query. */
  UNIT_TEST_ASSERT(select_query("SELECT MAX(id), MIN(id) FROM students;")
                   == DB_OK);
  UNIT_TEST_ASSERT(result_rows == 1);
  UNIT_TEST_ASSERT(result_cols == 2);
  UNIT_TEST_ASSERT(cell_long(0, 0) == 5);
  UNIT_TEST_ASSERT(cell_long(0, 1) == 1);

  /*
   * Aggregates over the LONG attribute "score" (90, 75, 60, 85, 95). These
   * exercise aggregation over a 32-bit attribute; the accumulated value must
   * reflect the full source values.
   */

  /* SUM (90+75+60+85+95 = 405). */
  UNIT_TEST_ASSERT(select_query("SELECT SUM(score) FROM students;") == DB_OK);
  UNIT_TEST_ASSERT(result_rows == 1);
  UNIT_TEST_ASSERT(cell_long(0, 0) == 405);

  /* MAX and MIN over the LONG attribute. */
  UNIT_TEST_ASSERT(select_query("SELECT MAX(score), MIN(score) FROM students;")
                   == DB_OK);
  UNIT_TEST_ASSERT(result_rows == 1);
  UNIT_TEST_ASSERT(cell_long(0, 0) == 95);
  UNIT_TEST_ASSERT(cell_long(0, 1) == 60);

  /* MEAN: 15/5 = 3 over id, 405/5 = 81 over score. */
  UNIT_TEST_ASSERT(select_query("SELECT MEAN(id) FROM students;") == DB_OK);
  UNIT_TEST_ASSERT(result_rows == 1);
  UNIT_TEST_ASSERT(cell_long(0, 0) == 3);
  UNIT_TEST_ASSERT(select_query("SELECT MEAN(score) FROM students;") == DB_OK);
  UNIT_TEST_ASSERT(result_rows == 1);
  UNIT_TEST_ASSERT(cell_long(0, 0) == 81);

  /* MEDIAN of an odd number of values: sorted scores are
     60, 75, 85, 90, 95, so the median is 85. */
  UNIT_TEST_ASSERT(select_query("SELECT MEDIAN(score) FROM students;") == DB_OK);
  UNIT_TEST_ASSERT(result_rows == 1);
  UNIT_TEST_ASSERT(cell_long(0, 0) == 85);

  /* MEDIAN of an even number of values: filtering out 60 leaves the sorted
     scores 75, 85, 90, 95, whose median is the mean of the two middle
     values, (85 + 90) / 2 = 87. */
  UNIT_TEST_ASSERT(select_query(
                   "SELECT MEDIAN(score) FROM students WHERE score > 60;")
                   == DB_OK);
  UNIT_TEST_ASSERT(result_rows == 1);
  UNIT_TEST_ASSERT(cell_long(0, 0) == 87);

  /* At most one MEDIAN aggregate is allowed per query. */
  UNIT_TEST_ASSERT(DB_ERROR(
                   select_query("SELECT MEDIAN(id), MEDIAN(score) FROM students;")));

  /* A sum whose result exceeds 16 bits (100000 + 200000 = 300000). */
  UNIT_TEST_ASSERT(exec_query("CREATE RELATION big;") == DB_OK);
  UNIT_TEST_ASSERT(exec_query("CREATE ATTRIBUTE amount DOMAIN LONG IN big;")
                   == DB_OK);
  UNIT_TEST_ASSERT(exec_query("INSERT (100000) INTO big;") == DB_OK);
  UNIT_TEST_ASSERT(exec_query("INSERT (200000) INTO big;") == DB_OK);
  UNIT_TEST_ASSERT(select_query("SELECT SUM(amount) FROM big;") == DB_OK);
  UNIT_TEST_ASSERT(result_rows == 1);
  UNIT_TEST_ASSERT(cell_long(0, 0) == 300000);

  UNIT_TEST_END();
}
/*---------------------------------------------------------------------------*/
UNIT_TEST_REGISTER(indexed_query,
                   "Point and range queries on an INLINE-indexed attribute");
UNIT_TEST(indexed_query)
{
  UNIT_TEST_BEGIN();

  UNIT_TEST_ASSERT(cfs_coffee_format() == 0);
  db_init();

  UNIT_TEST_ASSERT(exec_query("CREATE RELATION readings;") == DB_OK);
  UNIT_TEST_ASSERT(exec_query("CREATE ATTRIBUTE t DOMAIN LONG IN readings;")
                   == DB_OK);
  UNIT_TEST_ASSERT(exec_query("CREATE ATTRIBUTE v DOMAIN LONG IN readings;")
                   == DB_OK);

  /*
   * The INLINE index performs a binary search over a monotonically
   * increasing attribute, so it must be created before the (ordered) inserts.
   */
  UNIT_TEST_ASSERT(exec_query("CREATE INDEX readings.t TYPE INLINE;") == DB_OK);

  UNIT_TEST_ASSERT(exec_query("INSERT (0, 10) INTO readings;") == DB_OK);
  UNIT_TEST_ASSERT(exec_query("INSERT (10, 20) INTO readings;") == DB_OK);
  UNIT_TEST_ASSERT(exec_query("INSERT (20, 30) INTO readings;") == DB_OK);
  UNIT_TEST_ASSERT(exec_query("INSERT (30, 40) INTO readings;") == DB_OK);
  UNIT_TEST_ASSERT(exec_query("INSERT (40, 50) INTO readings;") == DB_OK);

  /* Point query resolved through the index. */
  UNIT_TEST_ASSERT(select_query("SELECT t FROM readings WHERE t = 20;") == DB_OK);
  UNIT_TEST_ASSERT(result_rows == 1);
  UNIT_TEST_ASSERT(cell_long(0, 0) == 20);

  /* Range query resolved through the index (t in [10, 30]). */
  UNIT_TEST_ASSERT(select_query(
                   "SELECT t FROM readings WHERE t >= 10 AND t <= 30;") == DB_OK);
  UNIT_TEST_ASSERT(result_rows == 3);
  UNIT_TEST_ASSERT(cell_long(0, 0) == 10);
  UNIT_TEST_ASSERT(cell_long(1, 0) == 20);
  UNIT_TEST_ASSERT(cell_long(2, 0) == 30);

  UNIT_TEST_END();
}
/*---------------------------------------------------------------------------*/
UNIT_TEST_REGISTER(remove_tuples, "Remove tuples that match a condition");
UNIT_TEST(remove_tuples)
{
  UNIT_TEST_BEGIN();

  UNIT_TEST_ASSERT(setup_students());

  /* Remove the two students whose score is below 80 (Bob and Carol). */
  UNIT_TEST_ASSERT(exec_query("REMOVE FROM students WHERE score < 80;") == DB_OK);

  UNIT_TEST_ASSERT(select_query("SELECT score FROM students;") == DB_OK);
  UNIT_TEST_ASSERT(result_rows == 3);
  UNIT_TEST_ASSERT(cell_long(0, 0) == 90);
  UNIT_TEST_ASSERT(cell_long(1, 0) == 85);
  UNIT_TEST_ASSERT(cell_long(2, 0) == 95);

  /* Nothing below 80 should remain. */
  UNIT_TEST_ASSERT(select_query("SELECT score FROM students WHERE score < 80;")
                   == DB_OK);
  UNIT_TEST_ASSERT(result_rows == 0);

  UNIT_TEST_END();
}
/*---------------------------------------------------------------------------*/
UNIT_TEST_REGISTER(join_relations, "Join two relations on a shared attribute");
UNIT_TEST(join_relations)
{
  UNIT_TEST_BEGIN();

  UNIT_TEST_ASSERT(cfs_coffee_format() == 0);
  db_init();

  /* A relation mapping a group id to a group name. */
  UNIT_TEST_ASSERT(exec_query("CREATE RELATION groups;") == DB_OK);
  UNIT_TEST_ASSERT(exec_query("CREATE ATTRIBUTE gid DOMAIN INT IN groups;")
                   == DB_OK);
  UNIT_TEST_ASSERT(exec_query("CREATE ATTRIBUTE gname DOMAIN STRING(16) IN groups;")
                   == DB_OK);

  /*
   * Antelope performs equi-joins through an index on the join attribute of
   * the right-hand relation, so "groups.gid" must be indexed. A MAXHEAP index
   * is used because, unlike an INLINE index, it is persisted and thus restored
   * when the relation is reloaded to execute the join. It is created before the
   * inserts so that it is populated as the rows are added.
   */
  UNIT_TEST_ASSERT(exec_query("CREATE INDEX groups.gid TYPE MAXHEAP;") == DB_OK);

  UNIT_TEST_ASSERT(exec_query("INSERT (1, 'Red') INTO groups;") == DB_OK);
  UNIT_TEST_ASSERT(exec_query("INSERT (2, 'Blue') INTO groups;") == DB_OK);

  /* A relation mapping a member to a group id. */
  UNIT_TEST_ASSERT(exec_query("CREATE RELATION members;") == DB_OK);
  UNIT_TEST_ASSERT(exec_query("CREATE ATTRIBUTE gid DOMAIN INT IN members;")
                   == DB_OK);
  UNIT_TEST_ASSERT(exec_query("CREATE ATTRIBUTE uid DOMAIN INT IN members;")
                   == DB_OK);
  UNIT_TEST_ASSERT(exec_query("INSERT (1, 100) INTO members;") == DB_OK);
  UNIT_TEST_ASSERT(exec_query("INSERT (2, 101) INTO members;") == DB_OK);
  UNIT_TEST_ASSERT(exec_query("INSERT (1, 102) INTO members;") == DB_OK);

  /*
   * Join members with groups on the group id, projecting the member id and
   * the group name. Three members should be matched to a group.
   */
  UNIT_TEST_ASSERT(select_query(
                   "JOIN members, groups ON gid PROJECT uid, gname;") == DB_OK);
  UNIT_TEST_ASSERT(result_rows == 3);
  UNIT_TEST_ASSERT(result_cols == 2);

  UNIT_TEST_END();
}
/*---------------------------------------------------------------------------*/
UNIT_TEST_REGISTER(large_relation, "Bulk insert and query a larger relation");
UNIT_TEST(large_relation)
{
  static int i;
  static char query[64];
  static const int n = 1000;

  UNIT_TEST_BEGIN();

  UNIT_TEST_ASSERT(cfs_coffee_format() == 0);
  db_init();

  UNIT_TEST_ASSERT(exec_query("CREATE RELATION samples;") == DB_OK);
  UNIT_TEST_ASSERT(exec_query("CREATE ATTRIBUTE k DOMAIN INT IN samples;")
                   == DB_OK);
  UNIT_TEST_ASSERT(exec_query("CREATE ATTRIBUTE v DOMAIN LONG IN samples;")
                   == DB_OK);

  /*
   * A persisted MAXHEAP index over the key. This also confirms that the single
   * MAXHEAP index slot freed by db_init() can be reused: an earlier test
   * (join_relations) already created a MAXHEAP index in this run.
   */
  UNIT_TEST_ASSERT(exec_query("CREATE INDEX samples.k TYPE MAXHEAP;") == DB_OK);

  /* Insert n tuples: k = 0..n-1, v = 2*k. */
  for(i = 0; i < n; i++) {
    snprintf(query, sizeof(query), "INSERT (%d, %d) INTO samples;", i, i * 2);
    UNIT_TEST_ASSERT(exec_query(query) == DB_OK);
  }

  /* A full scan must return every inserted tuple. */
  UNIT_TEST_ASSERT(select_query("SELECT k FROM samples;") == DB_OK);
  UNIT_TEST_ASSERT(result_rows == n);

  /* COUNT, MAX and MIN over the whole relation. */
  UNIT_TEST_ASSERT(select_query("SELECT COUNT(k) FROM samples;") == DB_OK);
  UNIT_TEST_ASSERT(cell_long(0, 0) == n);
  UNIT_TEST_ASSERT(select_query("SELECT MAX(k), MIN(k) FROM samples;") == DB_OK);
  UNIT_TEST_ASSERT(cell_long(0, 0) == n - 1);
  UNIT_TEST_ASSERT(cell_long(0, 1) == 0);

  /*
   * A point query that must find exactly one tuple among many. An attribute
   * referenced in a WHERE clause must also be projected, so both k and v are
   * selected here (k = 777, v = 2*k = 1554).
   */
  UNIT_TEST_ASSERT(select_query("SELECT v, k FROM samples WHERE k = 777;")
                   == DB_OK);
  UNIT_TEST_ASSERT(result_rows == 1);
  UNIT_TEST_ASSERT(cell_long(0, 0) == 1554);
  UNIT_TEST_ASSERT(cell_long(0, 1) == 777);

  /* A filter that matches a large, known subset (k in [900, 999]). */
  UNIT_TEST_ASSERT(select_query("SELECT k FROM samples WHERE k >= 900;") == DB_OK);
  UNIT_TEST_ASSERT(result_rows == 100);

  /* Bulk removal of the lower half, then confirm the new cardinality. */
  UNIT_TEST_ASSERT(exec_query("REMOVE FROM samples WHERE k < 500;") == DB_OK);
  UNIT_TEST_ASSERT(select_query("SELECT COUNT(k) FROM samples;") == DB_OK);
  UNIT_TEST_ASSERT(cell_long(0, 0) == 500);
  UNIT_TEST_ASSERT(select_query("SELECT k FROM samples;") == DB_OK);
  UNIT_TEST_ASSERT(result_rows == 500);

  UNIT_TEST_END();
}
/*---------------------------------------------------------------------------*/
UNIT_TEST_REGISTER(error_handling, "Invalid queries are rejected");
UNIT_TEST(error_handling)
{
  UNIT_TEST_BEGIN();

  UNIT_TEST_ASSERT(setup_students());

  /* Syntactically invalid query. */
  UNIT_TEST_ASSERT(DB_ERROR(exec_query("SELECT FROM;")));

  /* Selecting from a relation that does not exist. */
  UNIT_TEST_ASSERT(DB_ERROR(select_query("SELECT id FROM nonexistent;")));

  /* Selecting an attribute that does not exist. */
  UNIT_TEST_ASSERT(DB_ERROR(select_query("SELECT missing FROM students;")));

  /* Creating a relation that already exists must fail. */
  UNIT_TEST_ASSERT(DB_ERROR(exec_query("CREATE RELATION students;")));

  /* The valid schema should still be usable afterwards. */
  UNIT_TEST_ASSERT(select_query("SELECT id FROM students;") == DB_OK);
  UNIT_TEST_ASSERT(result_rows == 5);

  UNIT_TEST_END();
}
/*---------------------------------------------------------------------------*/
PROCESS_THREAD(test_process, ev, data)
{
  PROCESS_BEGIN();

  printf("Run unit-test\n");
  printf("---\n");

  UNIT_TEST_RUN(schema_and_insert);
  UNIT_TEST_RUN(where_conditions);
  UNIT_TEST_RUN(aggregates);
  UNIT_TEST_RUN(indexed_query);
  UNIT_TEST_RUN(remove_tuples);
  UNIT_TEST_RUN(join_relations);
  UNIT_TEST_RUN(large_relation);
  UNIT_TEST_RUN(error_handling);

  if(!UNIT_TEST_PASSED(schema_and_insert) ||
     !UNIT_TEST_PASSED(where_conditions) ||
     !UNIT_TEST_PASSED(aggregates) ||
     !UNIT_TEST_PASSED(indexed_query) ||
     !UNIT_TEST_PASSED(remove_tuples) ||
     !UNIT_TEST_PASSED(join_relations) ||
     !UNIT_TEST_PASSED(large_relation) ||
     !UNIT_TEST_PASSED(error_handling)) {
    printf("=check-me= FAILED\n");
    printf("---\n");
  }

  printf("=check-me= DONE\n");
  printf("---\n");

  PROCESS_END();
}
/*---------------------------------------------------------------------------*/
