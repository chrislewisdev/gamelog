#include <assert.h>
#include <getopt.h>
#include <stdbool.h>
#include <stdio.h>
#include <string.h>
#include <sqlite3.h>

// Passed to getopt to specify what arguments can be parsed
struct option argumentSpec[] = {
    {"alias", required_argument, NULL, 0},
    {0, 0, 0, 0}
};

// All possible arguments for commands. They are parsed ahead of command selection.
typedef struct arguments {
    char* alias;
} arguments;

void assignArgument(int index, arguments* args, char* value) {
    switch (index) {
        // Consider: how to maintain index correctness as we add more
        case 0:
            args->alias = value;
            break;
    }
}

arguments parseArguments(int argc, char* argv[], int* remainingOptionsIndex) {
    arguments args = {""};
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
    printf("- add-game <name> --alias <alias>\n");
    printf("- log <name> --games <games> --date <date>\n");
    printf("- report\n");
}

void report() {
    printf("report\n");
}

void addGame() {
    printf("Add a game...\n");
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
    if (!tableExists(db, "game")) {
        const char* sql = "CREATE TABLE game("
                            "game_id INTEGER PRIMARY KEY,"
                            "name NVARCHAR(255),"
                            "alias NVARCHAR (20)"
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
    int result = sqlite3_open("./gamelog.db", &db);
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

    char* cmd = argv[remainingOptionsIndex];
    if (strcmp(cmd, "report") == 0) {
        report();
    } else if (strcmp(cmd, "add-game") == 0) {
        addGame();
    }

    sqlite3_close(db);

    return 0;
}
