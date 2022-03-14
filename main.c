#include <assert.h>
#include <getopt.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sqlite3.h>

// Passed to getopt to specify what arguments can be parsed
struct option argumentSpec[] = {
    {"path", required_argument, NULL, 0},
    {"alias", required_argument, NULL, 0},
    {"games", required_argument, NULL, 0},
    {0, 0, 0, 0}
};

// All possible arguments for commands. They are parsed ahead of command selection.
typedef struct arguments {
    char* path;
    char* alias;
    int games;
} arguments;

typedef struct report_row {
    const char* name;
    const char* alias;
    int plays;
    int games;
} report_row;

// Default argument values.
const arguments defaultArgs = {
    .path = "gamelog.db",
    .alias = "",
    .games = 1
};

void assignArgument(int index, arguments* args, char* value) {
    // Indexes are in order of specification in argumentSpec.
    if (index == 0) {
        args->path = value;
    } else if (index == 1) {
        args->alias = value;
    } else if (index == 2) {
        args->games = atoi(value);
    }
}

arguments parseArguments(int argc, char* argv[], int* remainingOptionsIndex) {
    arguments args = defaultArgs;
    int option_index = 0;
    int option;

    while ((option = getopt_long(argc, argv, "", argumentSpec, &option_index)) != -1) {
        if (option == 0) {
           assignArgument(option_index, &args, optarg);
        }
    }

    *remainingOptionsIndex = optind;

    return args;
}

void printHelp() {
    printf("gamelog v0.1\n");
    printf("Supported commands:\n");
    printf("- add-game <name> --alias <string>\n");
    printf("- log <name> --games <int> --date <date>\n");
    printf("- report\n");
}

int getGamesCount(sqlite3* db) {
    sqlite3_stmt* query;
    const char* sql = "SELECT COUNT(game_id) FROM game";

    sqlite3_prepare_v2(db, sql, -1, &query, NULL);
    sqlite3_step(query);

    int count = sqlite3_column_int(query, 0);

    sqlite3_finalize(query);

    return count;
}

const char* duplicateString(const char* str) {
    int size = strlen(str);
    char* duplicate = (char*)malloc(size + 1);
    memset(duplicate, 0, size + 1);
    strncpy(duplicate, str, size);
    return duplicate;
}

report_row* getPlayReport(sqlite3* db, int* outRowCount) {
    int gamesCount = getGamesCount(db);
    if (gamesCount <= 0) return NULL;

    report_row* rows = (report_row*)malloc(sizeof(report_row) * gamesCount);

    sqlite3_stmt* query;
    const char* sql = "SELECT name, alias, COUNT(play.game_id), SUM(play.games) "
                        "FROM game LEFT JOIN play ON play.game_id = game.game_id "
                        "GROUP BY game.name, game.alias "
                        "ORDER BY 3 DESC, 4 DESC";

    sqlite3_prepare_v2(db, sql, -1, &query, NULL);
    int result = sqlite3_step(query);

    if (result != SQLITE_ROW && result != SQLITE_DONE) {
        printf("Error: %s\n", sqlite3_errmsg(db));
    }

    int i = 0;
    do {
        rows[i].name = duplicateString(sqlite3_column_text(query, 0));
        rows[i].alias = duplicateString(sqlite3_column_text(query, 1));
        rows[i].plays  = sqlite3_column_int(query, 2);
        rows[i].games = sqlite3_column_int(query, 3);
        i++;
    } while (sqlite3_step(query) == SQLITE_ROW);

    sqlite3_finalize(query);

    *outRowCount = gamesCount;
    return rows;
}

void disposeReport(report_row* rows, int rowCount) {
    for (int i = 0; i < rowCount; i++) {
        free((void*)rows[i].name);
        free((void*)rows[i].alias);
    }
    free(rows);
}

void getReportColumnWidths(report_row* rows, int rowCount, int* outNameWidth, int* outAliasWidth) {
    int maxNameWidth = 0;
    int maxAliasWidth = 0;

    for (int i = 0; i < rowCount; i++) {
        int nameWidth = strlen(rows[i].name);
        int aliasWidth = strlen(rows[i].alias);

        if (nameWidth > maxNameWidth) maxNameWidth = nameWidth;
        if (aliasWidth > maxAliasWidth) maxAliasWidth = aliasWidth;
    }

    *outNameWidth = maxNameWidth;
    *outAliasWidth = maxAliasWidth;
}

void report(sqlite3* db) {
    int rowCount, nameWidth, aliasWidth;
    report_row* rows = getPlayReport(db, &rowCount);
    getReportColumnWidths(rows, rowCount, &nameWidth, &aliasWidth);

    if (nameWidth < strlen("Name")) nameWidth = strlen("Name");
    if (aliasWidth < strlen("Alias")) aliasWidth = strlen("Alias");

    // Magic numbers are the constant column widths / spacings
    int tableWidth = nameWidth + 2 + aliasWidth + 3 + 8 + 6;

    // Table header
    for (int i = 0; i < tableWidth; i++) printf("-"); printf("\n");
    printf("%-*s | %-*s | Plays | Games\n", nameWidth, "Name", aliasWidth, "Alias");
    for (int i = 0; i < tableWidth; i++) printf("-"); printf("\n");

    // Table contents
    for (int i = 0; i < rowCount; i++) {
        printf("%-*s | %-*s | %5d | %5d\n",
            nameWidth, rows[i].name,
            aliasWidth, rows[i].alias,
            rows[i].plays,
            rows[i].games);
    }

    // Footer
    for (int i = 0; i < tableWidth; i++) printf("-"); printf("\n");

    disposeReport(rows, rowCount);
}

void addGame(sqlite3* db, char* name, char* alias) {
    sqlite3_stmt* query;
    const char* sql = "INSERT INTO game(name, alias) VALUES(?, ?)";

    sqlite3_prepare_v2(db, sql, -1, &query, NULL);
    sqlite3_bind_text(query, 1, name, -1, SQLITE_STATIC);
    sqlite3_bind_text(query, 2, alias, -1, SQLITE_STATIC);

    int result = sqlite3_step(query);

    sqlite3_finalize(query);

    if (result != SQLITE_DONE) {
        printf("Error inserting game: %s\n", sqlite3_errmsg(db));
    }
}

int getGameId(sqlite3* db, char* nameOrAlias) {
    sqlite3_stmt* query;
    const char* sql = "SELECT game_id FROM game WHERE name = ?1 OR alias = ?1";

    sqlite3_prepare_v2(db, sql, -1, &query, NULL);
    sqlite3_bind_text(query, 1, nameOrAlias, -1, SQLITE_STATIC);

    int result = sqlite3_step(query);

    if (result != SQLITE_ROW) {
        return -1;
    }

    int gameId = sqlite3_column_int(query, 0);

    sqlite3_finalize(query);

    return gameId;
}

void logPlay(sqlite3* db, char* nameOrAlias, int games) {
    int gameId = getGameId(db, nameOrAlias);

    if (gameId == -1) {
        printf("No game found matching name or alias: %s\n", nameOrAlias);
        return;
    }

    // TODO: Add date param
    sqlite3_stmt* query;
    const char* sql = "INSERT INTO play(game_id, games) VALUES(?, ?)";

    sqlite3_prepare_v2(db, sql, -1, &query, NULL);
    sqlite3_bind_int(query, 1, gameId);
    sqlite3_bind_int(query, 2, games);

    sqlite3_step(query);
    sqlite3_finalize(query);
}

bool tableExists(sqlite3* db, char* name) {
    sqlite3_stmt* query;
    const char* sql = "SELECT name FROM sqlite_master WHERE type='table' AND name=?";

    // We could prepare this statement on program launch, but realistically,
    // it is not used enough to justify it.
    sqlite3_prepare_v2(db, sql, -1, &query, NULL);
    sqlite3_bind_text(query, 1, name, -1,  SQLITE_STATIC);

    int result = sqlite3_step(query);

    sqlite3_finalize(query);

    return result == SQLITE_ROW;
}

int prepareDb(sqlite3* db) {
    // TODO: Add PRAGMA foreign_keys = ON

    if (!tableExists(db, "game")) {
        const char* sql = "CREATE TABLE game("
                            "game_id INTEGER PRIMARY KEY,"
                            "name NVARCHAR(255) UNIQUE,"
                            "alias NVARCHAR (20) UNIQUE"
                          ")";

        int error = sqlite3_exec(db, sql, NULL, NULL, NULL);
        if (error != SQLITE_OK) return error;
    }

    if (!tableExists(db, "play")) {
        const char* sql = "CREATE TABLE play("
                            "game_id INT,"
                            "games INT DEFAULT 1,"
                            "timestamp TEXT DEFAULT current_timestamp,"
                            "FOREIGN KEY (game_id) REFERENCES game(game_id) ON DELETE CASCADE"
                          ")";

        int error = sqlite3_exec(db, sql, NULL, NULL, NULL);
        if (error != SQLITE_OK) return error;
    }

    return SQLITE_OK;
}

void printArguments(int argc, char* argv[]) {
    printf("Arguments list:\n");
    for (int i = 0; i< argc; i++) {
        printf("%d. %s\n", i, argv[i]);
    }
}

int main(int argc, char* argv[]) {
    int remainingOptionsIndex;
    arguments args = parseArguments(argc, argv, &remainingOptionsIndex);

    if (remainingOptionsIndex >= argc) {
        printHelp();
        return 0;
    }

    sqlite3* db;
    // Consider: add a parameter for the db path
    int result = sqlite3_open(args.path, &db);
    if (result != SQLITE_OK) {
        printf("Unable to open database: %s\n", sqlite3_errmsg(db));
        sqlite3_close(db);
        return 1;
    }

    if (prepareDb(db) != SQLITE_OK) {
        printf("Error preparing database: %s\n", sqlite3_errmsg(db));
        sqlite3_close(db);
        return 1;
    }

    // TODO: Add rename feature
    char* cmd = argv[remainingOptionsIndex];
    if (strcmp(cmd, "report") == 0) {
        report(db);
    } else if (strcmp(cmd, "add-game") == 0) {
        if (remainingOptionsIndex + 1 >= argc) {
            printf("add-game: required argument <name>\n");
            return 1;
        }

        char* name = argv[remainingOptionsIndex + 1];
        addGame(db, name, args.alias);
    } else if (strcmp(cmd, "log") == 0) {
         if (remainingOptionsIndex + 1 >= argc) {
            printf("log: required argument <name-or-alias>\n");
            return 1;
        }

        char *nameOrAlias = argv[remainingOptionsIndex + 1];
        logPlay(db, nameOrAlias, args.games);
    } else {
        printf("Command not recognised: %s\n", cmd);
    }

    sqlite3_close(db);

    return 0;
}
