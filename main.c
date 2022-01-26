#include <stdbool.h>
#include <stdio.h>
#include <string.h>
#include <sqlite3.h>

void printVersion() {
    printf("gamelog v0.1\n");
}

void printHelp() {
    printVersion();
    printf("Supported commands:\n");
    // TODO: Indicate optional parameters
    printf("- add-game <name> <alias> (x)\n");
    printf("- list\n");
    printf("- log <name> (x)\n");
    printf("- report (x)\n");
}

void list() {
    // TODO: Implement this :D
    printf("list\n");
}

bool tableExists(sqlite3* db, char* name) {
    sqlite3_stmt* query;
    const char* sql = "SELECT name FROM sqlite_master WHERE type='table' AND name=?";

    // Consider: preparing any re-usable statements on program launch
    sqlite3_prepare_v2(db, sql, -1, &query, NULL);
    sqlite3_bind_text(query, 1, name, -1,  SQLITE_STATIC);

    int result = sqlite3_step(query);

    sqlite3_finalize(query);

    return result == SQLITE_ROW;
}

int prepareDb(sqlite3* db) {
    if (!tableExists(db, "games")) {
        const char* sql = "CREATE TABLE games(id INT PRIMARY KEY, name NVARCHAR(255), alias NVARCHAR (20))";
        // TODO: Check for errors
        sqlite3_exec(db, sql, NULL, NULL, NULL);
    }

    // TODO: Add all required tables (plays)
}

int main(int argc, char* argv[]) {
    if (argc < 2) {
        printHelp();
        return 0;
    }

    sqlite3* db;
    // Consider: add a parameter for the db path
    int result = sqlite3_open("./gamelog.db", &db);
    if (result != SQLITE_OK) {
        printf("Unable to open database: %s\n", sqlite3_errmsg(db));
        sqlite3_close(db);
        return 1;
    }

    prepareDb(db);

    char* cmd = argv[1];
    // TODO: Add more commands
    if (strcmp(cmd, "list") == 0) {
        list();
    }

    sqlite3_close(db);

    return 0;
}