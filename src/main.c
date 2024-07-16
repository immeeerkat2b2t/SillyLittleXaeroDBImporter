#include <sqlite3.h>
#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include <string.h>
#include <unistd.h>
#include <ncurses.h>
#include <dirent.h>
#include <sys/stat.h>

typedef struct {
    sqlite3 *db1;
    sqlite3 *db2;
    int paused;
    int total_tables;
    int current_table;
    int rows_added;
    pthread_mutex_t mutex;
    pthread_cond_t cond;
    WINDOW *win;
} AppData;

void rainbow_text(WINDOW *win, int y, int x, const char *text);

void select_file(char *prompt, char *filepath, size_t size) {
    int valid = 0;
    struct stat buffer;
    
    while (!valid) {
        werase(stdscr);
        rainbow_text(stdscr, 3, 0, prompt);
        wrefresh(stdscr);
        int ch, i = 0;
        char temp_filepath[256] = {0};
        while ((ch = getch()) != '\n' && i < size - 1) {
            if (ch == '\t') {  // Tab key for listing directory contents
                temp_filepath[i] = '\0';
                DIR *dir;
                struct dirent *ent;
                char dir_path[256] = {0};
                strncpy(dir_path, temp_filepath, sizeof(dir_path) - 1);
                if (dir != NULL && (dir = opendir(dir_path)) != NULL) {
                    werase(stdscr);
                    rainbow_text(stdscr, 3, 0, prompt);
                    mvwprintw(stdscr, 4, 0, "%s", temp_filepath);
                    int row = 5;
                    while ((ent = readdir(dir)) != NULL) {
                        mvwprintw(stdscr, row++, 0, "%s", ent->d_name);
                    }
                    closedir(dir);
                    wrefresh(stdscr);
                }
            } else if (ch == KEY_BACKSPACE || ch == 127 || ch == '\b') {
                if (i > 0) {
                    i--;
                    temp_filepath[i] = '\0';
                    mvwprintw(stdscr, 4, 0, "%s ", temp_filepath);
                    wmove(stdscr, 4, i);
                    wrefresh(stdscr);
                }
            } else {
                temp_filepath[i++] = ch;
                mvwprintw(stdscr, 4, 0, "%s", temp_filepath);
                wmove(stdscr, 4, i);
                wrefresh(stdscr);
            }
        }
        temp_filepath[i] = '\0';

        if (stat(temp_filepath, &buffer) == 0 && S_ISREG(buffer.st_mode) && strcmp(strrchr(temp_filepath, '.'), ".db") == 0) {
            valid = 1;
            strncpy(filepath, temp_filepath, size);
        } else {
            werase(stdscr);
            rainbow_text(stdscr, 3, 0, "Invalid path or not a .db file, you stupid silly kitten. Please enter a valid path: ");
            wrefresh(stdscr);
            sleep(2);
        }
    }
}

void *sync_databases(void *arg) {
    AppData *app_data = (AppData *)arg;
    sqlite3_stmt *stmt1, *stmt2;
    const char *sql;
    int rc;

    sql = "SELECT name FROM sqlite_master WHERE type='table'";
    rc = sqlite3_prepare_v2(app_data->db2, sql, -1, &stmt2, NULL);
    if (rc != SQLITE_OK) {
        mvwprintw(app_data->win, 1, 1, "Failed to fetch table names: %s\n", sqlite3_errmsg(app_data->db2));
        wrefresh(app_data->win);
        return NULL;
    }

    int total_tables = 0;
    while (sqlite3_step(stmt2) == SQLITE_ROW) {
        total_tables++;
    }
    sqlite3_reset(stmt2);

    while (sqlite3_step(stmt2) == SQLITE_ROW) {
        const char *table_name = (const char *)sqlite3_column_text(stmt2, 0);
        char query[256];

        snprintf(query, sizeof(query), "SELECT * FROM \"%s\"", table_name); // Quoted table name
        rc = sqlite3_prepare_v2(app_data->db2, query, -1, &stmt1, NULL);
        if (rc != SQLITE_OK) {
            mvwprintw(app_data->win, 2, 1, "Failed to fetch data from table %s: %s\n", table_name, sqlite3_errmsg(app_data->db2));
            wrefresh(app_data->win);
            continue;
        }

        mvwprintw(app_data->win, 1, 1, "Processing table: %s\n", table_name);
        wrefresh(app_data->win);

        int column_count = sqlite3_column_count(stmt1);
        int rows_added = 0;

        while (sqlite3_step(stmt1) == SQLITE_ROW) {
            pthread_mutex_lock(&app_data->mutex);
            while (app_data->paused) {
                pthread_cond_wait(&app_data->cond, &app_data->mutex);
            }
            pthread_mutex_unlock(&app_data->mutex);

            char insert_query[1024] = {0};
            snprintf(insert_query, sizeof(insert_query), "INSERT OR IGNORE INTO \"%s\" VALUES (", table_name); // Quoted table name

            for (int i = 0; i < column_count; i++) {
                if (i > 0) strcat(insert_query, ",");
                strcat(insert_query, "?");
            }
            strcat(insert_query, ")");

            sqlite3_stmt *insert_stmt;
            rc = sqlite3_prepare_v2(app_data->db1, insert_query, -1, &insert_stmt, NULL);
            if (rc != SQLITE_OK) {
                mvwprintw(app_data->win, 2, 1, "Failed to prepare insert statement: %s\n", sqlite3_errmsg(app_data->db1));
                wrefresh(app_data->win);
                continue;
            }

            for (int i = 0; i < column_count; i++) {
                sqlite3_bind_value(insert_stmt, i + 1, sqlite3_column_value(stmt1, i));
            }

            rc = sqlite3_step(insert_stmt);
            if (rc == SQLITE_DONE) {
                rows_added++;
                app_data->rows_added++;
                mvwprintw(app_data->win, 2, 1, "Rows added: %d\n", rows_added);
                wrefresh(app_data->win);
            } else {
                mvwprintw(app_data->win, 2, 1, "Failed to insert data: %s\n", sqlite3_errmsg(app_data->db1));
                wrefresh(app_data->win);
            }

            sqlite3_finalize(insert_stmt);
        }

        sqlite3_finalize(stmt1);

        app_data->current_table++;
        double fraction = (double)app_data->current_table / total_tables;
        mvwprintw(app_data->win, 3, 1, "Progress: %.2f%%\n", fraction * 100);
        wrefresh(app_data->win);
    }

    sqlite3_finalize(stmt2);

    mvwprintw(app_data->win, 4, 1, "Good silly little kitten make your database nice and fat\n");
    wrefresh(app_data->win);
    return NULL;
}

void on_start_sync(AppData *app_data) {
    char db1_path[256];
    char db2_path[256];

    select_file("Enter path to local Xaeros DB: ", db1_path, sizeof(db1_path));
    werase(stdscr);  // Clear the screen after entering the first path
    select_file("Enter path to Xaeros DB to import: ", db2_path, sizeof(db2_path));
    werase(stdscr);  // Clear the screen after entering the second path

    int rc = sqlite3_open(db1_path, &app_data->db1);
    if (rc) {
        mvwprintw(app_data->win, 2, 1, "Can't open database: %s\n", sqlite3_errmsg(app_data->db1));
        wrefresh(app_data->win);
        return;
    }

    rc = sqlite3_open(db2_path, &app_data->db2);
    if (rc) {
        mvwprintw(app_data->win, 2, 1, "Can't open database: %s\n", sqlite3_errmsg(app_data->db2));
        wrefresh(app_data->win);
        return;
    }

    const char *sql = "SELECT COUNT(*) FROM sqlite_master WHERE type='table'";
    sqlite3_stmt *stmt;
    rc = sqlite3_prepare_v2(app_data->db2, sql, -1, &stmt, NULL);
    if (rc != SQLITE_OK) {
        mvwprintw(app_data->win, 2, 1, "Failed to fetch table count: %s\n", sqlite3_errmsg(app_data->db2));
        wrefresh(app_data->win);
        return;
    }

    if (sqlite3_step(stmt) == SQLITE_ROW) {
        app_data->total_tables = sqlite3_column_int(stmt, 0);
    }
    sqlite3_finalize(stmt);

    pthread_t sync_thread;
    pthread_create(&sync_thread, NULL, sync_databases, app_data);
    pthread_detach(sync_thread);
}

void on_pause_sync(AppData *app_data) {
    pthread_mutex_lock(&app_data->mutex);
    app_data->paused = 1;
    pthread_mutex_unlock(&app_data->mutex);
    mvwprintw(app_data->win, 5, 1, "Sync paused.\n");
    wrefresh(app_data->win);
}

void on_resume_sync(AppData *app_data) {
    pthread_mutex_lock(&app_data->mutex);
    app_data->paused = 0;
    pthread_cond_signal(&app_data->cond);
    pthread_mutex_unlock(&app_data->mutex);
    mvwprintw(app_data->win, 5, 1, "Sync resumed.\n");
    wrefresh(app_data->win);
}

void rainbow_text(WINDOW *win, int y, int x, const char *text) {
    int len = strlen(text);
    int colors[] = {COLOR_RED, COLOR_YELLOW, COLOR_GREEN, COLOR_CYAN, COLOR_BLUE, COLOR_MAGENTA};
    for (int i = 0; i < len; i++) {
        wattron(win, COLOR_PAIR(colors[i % 6]));
        mvwaddch(win, y, x + i, text[i]);
        wattroff(win, COLOR_PAIR(colors[i % 6]));
    }
    wrefresh(win);
}

void show_help() {
    initscr();
    printw("This is the help description for now\n");
    refresh();
    getch();
    endwin();
}

int main(int argc, char *argv[]) {
    if (argc > 1 && strcmp(argv[1], "-h") == 0) {
        show_help();
        return 0;
    }

    initscr();
    start_color();
    use_default_colors();
    for (int i = 1; i <= 6; i++) {
        init_pair(i, i, -1);
    }
    cbreak();
    noecho();
    keypad(stdscr, TRUE);

    AppData app_data = {
        .paused = 0,
        .total_tables = 0,
        .current_table = 0,
        .rows_added = 0,
        .mutex = PTHREAD_MUTEX_INITIALIZER,
        .cond = PTHREAD_COND_INITIALIZER,
    };

    app_data.win = newwin(10, 50, 1, 1);
    box(app_data.win, 0, 0);
    wrefresh(app_data.win);

    char command[10];
    while (1) {
        werase(stdscr);
        rainbow_text(stdscr, 0, 0, "Silly Little Xaero DB Importer - Type start to begin: Commands (start, pause, resume, quit, catnip)");
        wrefresh(stdscr);
        echo();
        wgetnstr(stdscr, command, sizeof(command) - 1);
        noecho();
        if (strncmp(command, "start", 5) == 0) {
            on_start_sync(&app_data);
        } else if (strncmp(command, "pause", 5) == 0) {
            on_pause_sync(&app_data);
        } else if (strncmp(command, "resume", 6) == 0) {
            on_resume_sync(&app_data);
        } else if (strncmp(command, "quit", 4) == 0) {
            break;
        }
    }

    sqlite3_close(app_data.db1);
    sqlite3_close(app_data.db2);
    endwin();
    return 0;
}
