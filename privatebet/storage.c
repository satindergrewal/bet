#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/types.h>
#include <pwd.h>

#include "storage.h"

#define no_of_tables 7

char *db_name = NULL;

const char *table_names[no_of_tables] = { "dcv_tx_mapping", "player_tx_mapping", "cashier_tx_mapping",
					  "c_tx_addr_mapping", "dcv_game_state", "player_game_state", "cashier_game_state"};

const char *schemas[no_of_tables] = {
	"(tx_id varchar(100) primary key,table_id varchar(100), status bool)",
	"(tx_id varchar(100) primary key,table_id varchar(100), status bool)",
	"(tx_id varchar(100) primary key,table_id varchar(100), status bool)",
	"(payin_tx_id varchar(100) primary key,msig_addr varchar(100), min_notaries int, table_id varchar(100), msig_addr_nodes varchar(100), payin_tx_id_status int, payout_tx_id varchar(100))",
	"(table_id varchar(100) primary key,game_state varchar(1000))",
	"(table_id varchar(100) primary key,game_state varchar(1000))",
	"(table_id varchar(100) primary key,game_state varchar(1000))"
};

void sqlite3_init_db_name()
{
	struct passwd *pw = getpwuid(getuid());
	char *homedir = pw->pw_dir;
	db_name = calloc(1, 200);
	sprintf(db_name, "%s/.bet/db/pangea.db", homedir);
}

int32_t sqlite3_check_if_table_id_exists(const char *table_id)
{
	sqlite3_stmt *stmt = NULL;
	sqlite3 *db = NULL;
	char *sql_query = NULL;
	int32_t rc, retval = 0;

	db = bet_get_db_instance();
	sql_query = calloc(1, 200);

	sprintf(sql_query, "select count(table_id) from c_tx_addr_mapping where table_id = \"%s\";", table_id);
	rc = sqlite3_prepare_v2(db, sql_query, -1, &stmt, NULL);
	if (rc != SQLITE_OK) {
		printf("error: %s::%s", sqlite3_errmsg(db), sql_query);
		goto end;
	}
	while ((rc = sqlite3_step(stmt)) == SQLITE_ROW) {
		const int count = sqlite3_column_int(stmt, 0);
		if (count > 0) {
			retval = 1;
			break;
		}
	}
	sqlite3_finalize(stmt);
end:
	if (sql_query)
		free(sql_query);
	return retval;
}

int32_t sqlite3_check_if_table_exists(sqlite3 *db, const char *table_name)
{
	sqlite3_stmt *stmt = NULL;
	char *sql_query = NULL;
	int rc, retval = 0;

	db = bet_get_db_instance();
	sql_query = calloc(1, 200);

	sprintf(sql_query, "select name from sqlite_master where type = \"table\" and name =\"%s\";", table_name);
	rc = sqlite3_prepare_v2(db, sql_query, -1, &stmt, NULL);
	if (rc != SQLITE_OK) {
		printf("error: %s::%s", sqlite3_errmsg(db), sql_query);
		goto end;
	}
	while ((rc = sqlite3_step(stmt)) == SQLITE_ROW) {
		const char *name = sqlite3_column_text(stmt, 0);
		if (strcmp(name, table_name) == 0) {
			retval = 1;
			break;
		}
	}
	sqlite3_finalize(stmt);
end:
	if (sql_query)
		free(sql_query);
	return retval;
}

sqlite3 *bet_get_db_instance()
{
	sqlite3 *db = NULL;
	int rc;

	rc = sqlite3_open(db_name, &db);
	if (rc) {
		fprintf(stderr, "Can't open database: %s\n", sqlite3_errmsg(db));
		return (0);
	}
	return db;
}

void bet_make_insert_query(int argc, char **argv, char **sql_query)
{
	
	sprintf(*sql_query, "INSERT INTO %s values(",argv[0]);
	for(int32_t i = 1; i < argc; i++) {
		strcat(*sql_query,argv[i]);
		if((i+1) < argc)
			strcat(*sql_query,",");
		else
			strcat(*sql_query,");");
	}
}

int32_t bet_run_query(char *sql_query)
{
	sqlite3 *db;
	char *err_msg = NULL;
	int32_t rc = -1;

	if (sql_query == NULL)
		return rc;
	else {
		db = bet_get_db_instance();
		/* Execute SQL statement */
		rc = sqlite3_exec(db, sql_query, NULL, 0, &err_msg);

		if (rc != SQLITE_OK) {
			fprintf(stderr, "SQL error: %s::%s\n", err_msg, sql_query);
			sqlite3_free(err_msg);
		}
		sqlite3_close(db);
	}
	return rc;
}

void bet_create_schema()
{
	sqlite3 *db = NULL;
	int rc;
	char *sql_query = NULL, *err_msg = NULL;

	db = bet_get_db_instance();
	sql_query = calloc(1, 200);
	for (int32_t i = 0; i < no_of_tables; i++) {
		if (sqlite3_check_if_table_exists(db, table_names[i]) == 0) {
			sprintf(sql_query, "CREATE TABLE %s %s;", table_names[i], schemas[i]);

			rc = sqlite3_exec(db, sql_query, NULL, 0, &err_msg);
			if (rc != SQLITE_OK) {
				fprintf(stderr, "SQL error: %s::%s\n", err_msg, sql_query);
				sqlite3_free(err_msg);
			}
			memset(sql_query, 0x00, 200);
		}
	}
	if (sql_query)
		free(sql_query);
	sqlite3_close(db);
}

void bet_sqlite3_init()
{
	sqlite3_init_db_name();
	bet_create_schema();
}
