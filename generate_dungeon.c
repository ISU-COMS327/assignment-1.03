#include <stdint.h>
#include <string.h>
#include <getopt.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <time.h>
#include <sys/stat.h>

#define HEIGHT 105
#define WIDTH 160
#define IMMUTABLE_ROCK 255
#define ROCK 200
#define ROOM -1
#define CORRIDOR 0
#define MIN_NUMBER_OF_ROOMS 10
#define MAX_NUMBER_OF_ROOMS 50
#define MIN_ROOM_WIDTH 7
#define DEFAULT_MAX_ROOM_WIDTH 15
#define MIN_ROOM_HEIGHT 5
#define DEFAULT_MAX_ROOM_HEIGHT 10

static char * TYPE_ROOM = "room";
static char * TYPE_CORRIDOR = "corridor";
static char * TYPE_ROCK = "rock";

typedef struct {
    int hardness;
    char * type;
} Board_Cell;

struct Room {
    uint8_t start_x;
    uint8_t end_x;
    uint8_t start_y;
    uint8_t end_y;
};

Board_Cell board[HEIGHT][WIDTH];
struct Room * rooms;
char * RLG_DIRECTORY;

int DO_SAVE = 0;
int DO_LOAD = 0;
int SHOW_HELP = 0;
int NUMBER_OF_ROOMS = MIN_NUMBER_OF_ROOMS;
int MAX_ROOM_WIDTH = DEFAULT_MAX_ROOM_WIDTH;
int MAX_ROOM_HEIGHT = DEFAULT_MAX_ROOM_HEIGHT;

void print_usage();
void make_rlg_directory();
void update_number_of_rooms();
int random_int(int min_num, int max_num, int add_to_seed);
void initialize_board();
void initialize_immutable_rock();
void load_board();
void save_board();
void print_board();
void print_cell();
void dig_rooms(int number_of_rooms_to_dig);
void dig_room(int index, int recursive_iteration);
int room_is_valid_at_index(int index);
void add_rooms_to_board();
void dig_cooridors();
void connect_rooms_at_indexes(int index1, int index2);

int main(int argc, char *args[]) {
    struct option longopts[] = {
        {"save", no_argument, &DO_SAVE, 1},
        {"load", no_argument, &DO_LOAD, 1},
        {"rooms", required_argument, 0, 'r'},
        {"help", no_argument, &SHOW_HELP, 'h'},
        {0, 0, 0, 0}
    };
    int c;
    while((c = getopt_long(argc, args, "h:", longopts, NULL)) != -1) {
        switch(c) {
            case 'r':
                NUMBER_OF_ROOMS = atoi(optarg);
                break;
            case 'h':
                SHOW_HELP = 1;
                break;
            default:
                break;
        }
    }

    if (SHOW_HELP) {
        print_usage();
        exit(0);
    }
    printf("Received Parameters: Save: %d, Load: %d, #Rooms: %d\n\n", DO_SAVE, DO_LOAD, NUMBER_OF_ROOMS);
    update_number_of_rooms();
    initialize_board();
    make_rlg_directory();

    if (DO_LOAD) {
        load_board();
    }
    else {
        printf("Generating dungeon... \n");
        printf("Making %d rooms.\n", NUMBER_OF_ROOMS);
        rooms = malloc(sizeof(struct Room) * NUMBER_OF_ROOMS);
        dig_rooms(NUMBER_OF_ROOMS);
        dig_cooridors();
    }

    print_board();

    if (DO_SAVE) {
        save_board();
    }

    return 0;
}

void update_number_of_rooms() {
    if (NUMBER_OF_ROOMS < MIN_NUMBER_OF_ROOMS) {
        printf("Minimum number of rooms is %d\n", MIN_NUMBER_OF_ROOMS);
        NUMBER_OF_ROOMS = MIN_NUMBER_OF_ROOMS;
    }
    if (NUMBER_OF_ROOMS > MAX_NUMBER_OF_ROOMS) {
        printf("Maximum number of rooms is %d\n", MAX_NUMBER_OF_ROOMS);
        NUMBER_OF_ROOMS = MAX_NUMBER_OF_ROOMS;
    }
}

void make_rlg_directory() {
    char * home = getenv("HOME");
    char dir[] = "/.rlg327/";
    RLG_DIRECTORY = malloc(strlen(home) + strlen(dir));
    strcat(RLG_DIRECTORY, home);
    strcat(RLG_DIRECTORY, dir);
    mkdir(RLG_DIRECTORY, 0777);
}

void save_board() {
    char filename[] = "dungeon";
    char * filepath = malloc(strlen(filename) + strlen(RLG_DIRECTORY));
    strcat(filepath, RLG_DIRECTORY);
    strcat(filepath, filename);
    printf("Saving file to: %s\n", filepath);
    FILE * fp = fopen(filepath, "wb+");
    if (fp == NULL) {
        printf("Cannot save file\n");
        return;
    }
    char * file_marker = "RLG327-S2017";
    uint32_t version = htonl(0);
    uint32_t file_size = htonl(16820 + (NUMBER_OF_ROOMS * 4));

    fwrite(file_marker, 1, strlen(file_marker), fp);
    fwrite(&version, 1, 4, fp);
    fwrite(&file_size, 1, 4, fp);
    for (int y = 0; y < HEIGHT; y++) {
        for (int x = 0; x < WIDTH; x++) {
            uint8_t num = board[y][x].hardness;
            fwrite(&num, 1, 1, fp);
        }
    }

    for (int i = 0; i < NUMBER_OF_ROOMS; i++) {
        struct Room room = rooms[i];
        uint8_t height = room.end_y - room.start_y + 1;
        uint8_t width = room.end_x - room.start_x + 1;
        fwrite(&room.start_x, 1, 1, fp);
        fwrite(&room.start_y, 1, 1, fp);
        fwrite(&(width), 1, 1, fp);
        fwrite(&(height), 1, 1, fp);
    }
    fclose(fp);
}

void load_board() {
    char filename[] = "dungeon";
    char * filepath = malloc(strlen(filename) + strlen(RLG_DIRECTORY));
    strcat(filepath, RLG_DIRECTORY);
    strcat(filepath, filename);
    printf("Loading dungeon: %s\n", filepath);
    FILE *fp = fopen(filepath, "r");
    if (fp == NULL) {
        printf("Cannot load '%s'\n", filepath);
        exit(1);
    }
    char title[13]; // one extra index for the null value at the end
    uint32_t version;
    uint32_t file_size;


    // Get title
    fread(title, 1, 12, fp);

    // Get version
    fread(&version, 4, 1, fp);
    version = ntohl(version);

    // Get file size
    fread(&file_size, 4, 1, fp);
    file_size = ntohl(file_size);

    printf("File Marker: %s :: Version: %d :: File Size: %d bytes\n", title, version, file_size);

    uint8_t num;
    int x = 0;
    int y = 0;
    for (int i = 0; i < 16800; i++) {
        fread(&num, 1, 1, fp);
        Board_Cell cell;
        cell.hardness = num;
        if (num == 0) {
            cell.type = TYPE_CORRIDOR;
        }
        else {
            cell.type = TYPE_ROCK;
        }
        board[y][x] = cell;
        if (x == WIDTH - 1) {
            x = 0;
            y ++;
        }
        else {
            x ++;
        }
    }

    uint8_t start_x;
    uint8_t start_y;
    uint8_t width;
    uint8_t height;
    NUMBER_OF_ROOMS = (file_size - ftell(fp)) / 4;
    rooms = malloc(sizeof(struct Room) * NUMBER_OF_ROOMS);
    int counter = 0;
    while(ftell(fp) != file_size) {
        fread(&start_x, 1, 1, fp);
        fread(&start_y, 1, 1, fp);
        fread(&width, 1, 1, fp);
        fread(&height, 1, 1, fp);

        struct Room room;
        room.start_x = start_x;
        room.start_y = start_y;
        room.end_x = start_x + width - 1;
        room.end_y = start_y + height - 1;
        rooms[counter] = room;
        counter ++;
    }
    add_rooms_to_board();
    fclose(fp);
}

void print_usage() {
    printf("usage: generate_dungeon [--save] [--load] [--rooms=<number of rooms>]\n");
}

int random_int(int min_num, int max_num, int add_to_seed) {
    int seed = time(NULL);
    if (add_to_seed) {
        seed += add_to_seed * 10;
    }
    max_num ++;
    int delta = max_num - min_num;
    srand(seed);
    return (rand() % delta) + min_num;
}

void initialize_board() {
    Board_Cell cell;
    cell.type = TYPE_ROCK;
    cell.hardness = ROCK;
    for (int y = 0; y < HEIGHT; y++) {
        for (int x = 0; x < WIDTH; x++) {
            board[y][x] = cell;
        }
    }
    initialize_immutable_rock();
}

void initialize_immutable_rock() {
    int y;
    int x;
    int max_x = WIDTH - 1;
    int max_y = HEIGHT - 1;
    Board_Cell cell;
    cell.type = TYPE_ROCK;
    cell.hardness = IMMUTABLE_ROCK;
    for (y = 0; y < HEIGHT; y++) {
        board[y][0] = cell;
        board[y][max_x] = cell;
    }
    for (x = 0; x < WIDTH; x++) {
        board[0][x] = cell;
        board[max_y][x] = cell;
    }
}

void print_board() {
    for (int y = 0; y < HEIGHT; y++) {
        for (int x = 0; x < WIDTH; x++) {
            print_cell(board[y][x]);
        }
        printf("\n");
    }
}

void print_cell(Board_Cell cell) {
    if (strcmp(cell.type, TYPE_ROCK) == 0) {
        printf(" ");
    }
    else if (strcmp(cell.type, TYPE_ROOM) == 0) {
        printf(".");
    }
    else if (strcmp(cell.type, TYPE_CORRIDOR) == 0) {
        printf("#");
    }
    else {
        printf("F");
    }
}

void dig_rooms(int number_of_rooms_to_dig) {
    for (int i = 0; i < number_of_rooms_to_dig; i++) {
        dig_room(i, 0);
    }
    add_rooms_to_board();
}

void dig_room(int index, int recursive_iteration) {
    // The index + recusrive_iteration is just a way to gain variety in the
    // random number. The hope is that it makes the program run faster.
    int start_x = random_int(0, WIDTH - MIN_ROOM_WIDTH - 1, index + recursive_iteration * 10);
    int start_y = random_int(0, HEIGHT - MIN_ROOM_HEIGHT - 1, index + recursive_iteration / 10);
    int room_height = random_int(MIN_ROOM_HEIGHT, MAX_ROOM_HEIGHT, index + recursive_iteration - 5000);
    int room_width = random_int(MIN_ROOM_WIDTH, MAX_ROOM_WIDTH, index + recursive_iteration + 5000);
    int end_y = start_y + room_height;
    if (end_y >= HEIGHT - 1) {
        end_y = HEIGHT - 2;

    }
    int end_x = start_x + room_width;
    if (end_x >= WIDTH - 1) {
        end_x = WIDTH - 2;

    }
    int height = end_y - start_y;
    int height_diff = MIN_ROOM_HEIGHT - height;
    if (height_diff > 0) {
        start_y -= height_diff + 1;
    }

    int width = end_x - start_x;
    int width_diff = MIN_ROOM_WIDTH - width;
    if (width_diff > 0) {
        start_x -= width_diff;
    }
    rooms[index].start_x = start_x;
    rooms[index].start_y = start_y;
    rooms[index].end_x = end_x;
    rooms[index].end_y = end_y;
    if (!room_is_valid_at_index(index)) {
        dig_room(index, recursive_iteration + 1);
    }
}

int room_is_valid_at_index(int index) {
    struct Room room = rooms[index];
    int width = room.end_x - room.start_x;
    int height = room.end_y - room.start_y;
    if (height < MIN_ROOM_HEIGHT || width < MIN_ROOM_WIDTH) {
        return 0;
    }
    for (int i = 0; i < index; i++) {
        struct Room current_room = rooms[i];
        int start_x = current_room.start_x - 1;
        int start_y = current_room.start_y - 1;
        int end_x = current_room.end_x + 1;
        int end_y = current_room.end_y + 1;
        if ((room.start_x >= start_x  && room.start_x <= end_x) ||
                (room.end_x >= start_x && room.end_x <= end_x)) {
            if ((room.start_y >= start_y && room.start_y <= end_y) ||
                    (room.end_y >= start_y && room.end_y <= end_y)) {
                return 0;
            }
        }
    }
    return 1;
}

void add_rooms_to_board() {
    Board_Cell cell;
    cell.type = TYPE_ROOM;
    cell.hardness = ROOM;
    for(int i = 0; i < NUMBER_OF_ROOMS; i++) {
        struct Room room = rooms[i];
        for (int y = room.start_y; y <= room.end_y; y++) {
            for(int x = room.start_x; x <= room.end_x; x++) {
                board[y][x] = cell;
            }
        }
    }
}

void dig_cooridors() {
    for (int i = 0; i < NUMBER_OF_ROOMS; i++) {
        int next_index = i + 1;
        if (next_index == NUMBER_OF_ROOMS) {
            next_index = 0;
        }
        connect_rooms_at_indexes(i, next_index);
    }
}

void connect_rooms_at_indexes(int index1, int index2) {
    struct Room room1 = rooms[index1];
    struct Room room2 = rooms[index2];
    int start_x = ((room1.end_x - room1.start_x) / 2) + room1.start_x;
    int end_x = ((room2.end_x - room2.start_x) / 2) + room2.start_x;
    int start_y = ((room1.end_y - room1.start_y) / 2) + room1.start_y;
    int end_y = ((room2.end_y - room2.start_y) / 2) + room2.start_y;
    int x_incrementer = 1;
    int y_incrementer = 1;
    if (start_x > end_x) {
        x_incrementer = -1;
    }
    if (start_y > end_y) {
        y_incrementer = -1;
    }
    int cur_x = start_x;
    int cur_y = start_y;
    while(1) {
        int random_num = random_int(0, RAND_MAX, cur_x + cur_y) >> 3;
        int move_y = random_num % 2 == 0;
        if (strcmp(board[cur_y][cur_x].type,  TYPE_ROCK) != 0) {
            if (cur_y != end_y) {
                cur_y += y_incrementer;
            }
            else if(cur_x != end_x) {
                cur_x += x_incrementer;
            }
            else if(cur_y == end_y && cur_x == end_x) {
                break;
            }
            continue;
        }
        Board_Cell corridor_cell;
        corridor_cell.type = TYPE_CORRIDOR;
        corridor_cell.hardness = CORRIDOR;
        board[cur_y][cur_x] = corridor_cell;
        if ((cur_y != end_y && move_y) || (cur_x == end_x)) {
            cur_y += y_incrementer;
        }
        else if ((cur_x != end_x && !move_y) || (cur_y == end_y)) {
            cur_x += x_incrementer;
        }
        else {
            break;
        }
    }
}
