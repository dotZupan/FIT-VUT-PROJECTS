#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <string.h>


#define LEFT 1
#define RIGHT 2
#define H_D 4

#define RHAND 1
#define LHAND 2

#define TEST 3
#define HELP 2
#define PATH 5

typedef struct {
    int rows;
    int cols;
    unsigned char *cells;
} Map;


/**
*@brief Frees all allocated memory
*@param map Map of the maze 
*/
void map_dtor(Map *map){
    if(map->cells != NULL ){    
        free(map->cells);
    }
    free(map);
}

/**
*@brief Allocates memorry for struct on the heap
*@return Struct of map if allocation was successful or NULL if it wasn't
*/
Map *map_ctor(){
    Map *map = (Map *)malloc(sizeof(Map));
    if(map == NULL){
        fprintf(stderr, "Erorr: ALLOC failed");
        return NULL;
    }
    map->cells = NULL;
    return map;
}


/**
*@brief Check if a border in the cell has a wall using bitwise AND operator
*@param map Map of maze 
*@param r Current row number of cell
*@param c Current column of cell
*@param border designated border to check
*@return True if designeded border in the cell has a wall or false if it hasn't
*/  
bool isborder(Map *map, int r, int c, int border){
    return map->cells[(r-1)*map->cols + (c-1)] & border;
}

/**
*@brief Checks if the cell has a Top or Bottom border based on its position
*@param r Current row number of cell
*@param c Current column of cell
*@return True if cell has Top or false if it has Bottom
*/
bool HasTop(int r, int c){
    if(r % 2 == 1){
        if(c % 2 == 1){
            return true;
        }
        else{
            return false;
        }
    }
    else{
        if(c % 2 == 1){
            return false;
        }
        else{
            return true;
        }
    }
}

/**
*@brief Tests the map to ensure it has the correct characters and that neighboring cells share the same border
*@param map Map of the maze 
*@return true if maze is valid or false if maze is invalid
*/
bool testing(Map *map){
    for(int i = 0; i < map->rows*map->cols; i++){
        if(map->cells[i]  > 7){
            fprintf(stderr, "Error: Wrong characters in the file");
            return false;
        }
    }
    for(int row = 1; row <= map->rows; row++){
        for(int column = 1; column <= map->cols; column++){
            for(int border = 1; border < 5; border *= 2){
                if(isborder(map, row, column, border)){
                    if(border == LEFT){
                        if(column == 1){
                            continue;
                        }
                        if(!isborder(map, row, column - 1, RIGHT)){
                            fprintf(stderr, "Error: Borders don't match");
                            return false;
                        }
                    }
                    if(border == RIGHT){
                        if(column == map->cols){
                            continue;
                            }
                        if(!isborder(map, row, column + 1, LEFT)){
                            fprintf(stderr, "Error: Borders don't match");
                            return false;
                        }
                    }

                    if(border == H_D){
                        if(HasTop(row, column)){
                            if(row != 1){
                                if(!isborder(map, row - 1 , column, H_D)){
                                    fprintf(stderr, "Error: Borders don't match");
                                    return false;
                                }
                            }
                        }
                        else{
                            if(row != map->rows){
                                if(!isborder(map, row + 1, column, H_D)){
                                    fprintf(stderr, "Error: Borders don't match");
                                    return false;
                                }
                            }
                        }
                    }
                }
            }
        }
    }
    return true;
}


/**
*@brief Loads characters from a file into an array of unsigned characters and performs some correctness tests
*@param file File where maze is stored
*@param map Map of the maze 
*@return return 1 if load was successfull and 0 if load wasn't successfull
*/
int load(char *file, Map *map){
    FILE *fl = fopen(file, "r");
    if(fl == NULL){
        fprintf(stderr, "Error: File opening failed");
        return 0;
    }

    if(fscanf(fl, "%d %d", &map->rows, &map->cols) != 2){
        fprintf(stderr, "Error: Wrong characters in the file");
        fclose(fl);
        return 0;
    }

    map->cells = (unsigned char *)malloc(sizeof(unsigned char) * map->rows * map->cols);
    if(map->cells == NULL){
        fprintf(stderr, "Erorr: ALLOC failed");
        fclose(fl);
        return 0;
    }

    int size;
    int checker;
    for(int i = 0; i < map->rows * map->cols; i++){
        if(feof(fl)){
            break;
        }
        checker = fscanf(fl, "%hhd", &map->cells[i]);
        if(checker == 0){
            fprintf(stderr, "Error: Wrong characters in the file");
            fclose(fl);
            return 0;
        }
        size = i;
    }
    /*
    if(fscanf(fl, "%d", &checker) == 1){
        fprintf(stderr, "Error: Wrong amount of elements in the file\n");
        fclose(fl);
        return 0;
    }*/
    
    
    if(size + 1 != map->rows * map->cols){
        fprintf(stderr, "Error: Wrong amount of elements in the file\n");
        fclose(fl);
        return 0;
    }
    

    if(!testing(map)){
        fclose(fl);
        return 0;
    }

    fclose(fl);
    return 1;
}

/**
*@brief Checks if the coordinates are a valid entrance position and determines the border to focus on
*@param map Map of the maze 
*@param r Starting row number of cell
*@param c Starting column of cell
*@param leftright Method usedfor passage
*@return 0 if starting position is invalid or Border which we will focus on
*/
int start_border(Map *map, int r, int c, int leftright){
    if(r > map->rows || r < 1 || c > map->cols || c < 1){
        fprintf(stderr, "Error: Wrong entrance coordinates");
        return 0;
    }

    if(c == 1 && !isborder(map, r, c, LEFT)){
        if(r % 2 == 1){
            if(leftright == RHAND){
                return RIGHT;
                }
            else{
                return H_D;
            }
        }
        else{
            if(leftright == RHAND){
                return H_D;
                }
            else{
                return RIGHT;
            }
        }
    }

    else if(c == map->cols && !isborder(map, r, c, RIGHT)){       
        if(HasTop(r, c)){
            if(leftright == RHAND){
                return H_D;
            }
            else{
                return LEFT;
            }
        }
        else{
            if(leftright == RHAND){
                return LEFT;
            }
            else{
                return H_D;
            }
        }
    }

    else if(r == 1 && HasTop(r, c) && !isborder(map, r, c, H_D)){
        if(leftright == RHAND){
            return LEFT;
        }
        else{
            return RIGHT;
        }
    }

    else if(r == map->rows && !HasTop(r, c) && !isborder(map, r, c, H_D)){
        if(leftright == RHAND){
            return RIGHT;
        }
        else{
            return LEFT;
        }
    }

    fprintf(stderr, "Error: Wrong entrance coordinates");
    return 0;
}


/**
*@brief Determines the entry border based on the starting position
*@param map Map of the maze 
*@param r Starting row number of cell
*@param c Starting column of cell
*@return Border which will be focused on
*/
int starting_entry(Map *map, int r, int c){
    if(c == 1 && !isborder(map, r, c, LEFT)){
        return LEFT;
    }
    else if(c == map->cols && !isborder(map, r, c, RIGHT)){       
        return RIGHT;
    }
    else{
        return H_D;
    }
}


/**
*@brief Finds the first available passage from the cell
*@param map Map of the  maze 
*@param r Current row number of cell
*@param c Current column of cell
*@param dir Border on which we are focused on
*@param mypos Border by which we entered the cell
*@param hand Defines which passage rule is used
*@return First border that can be crossed
*/
int find_direction(Map *map, int r, int c, int dir, int mypos, int hand){
    while (isborder(map, r, c, dir)){
        switch(mypos){
            case LEFT:
                mypos = dir;
                if(!HasTop(r, c)){    
                    dir = (hand == RHAND)? RIGHT: H_D;
                    break;
                }
                else{
                    dir = (hand == RHAND)? H_D : RIGHT;
                    break;
                }
            case RIGHT:
                mypos = dir;
                if(!HasTop(r, c)){
                    dir = (hand == RHAND)? H_D: LEFT;
                    break;
                }
                else{
                    dir = (hand == RHAND)? LEFT : H_D;
                    break;
                }
            case H_D:
                mypos = dir;
                if(!HasTop(r, c)){
                    dir = (hand == RHAND)?  LEFT : RIGHT;
                    break;
                }
                else{
                    dir = (hand == RHAND)? RIGHT : LEFT;
                    break;
                }
        }
    }
    return dir;
}


/**
*@brief Changes the position based on direction and determines the first border to be checked in the next cell
*@param r Row number of cell to be changed
*@param c Column number of cell to be changed
*@param dir Passage border of cell to be changed 
*@param entry Border by which we will enter the next cell
*@param hand Defines which passage rule is used
*/
void move(int *r, int *c, int *dir, int *entry, int hand){
    int old_dir = *dir;
    switch(old_dir){
        case LEFT:
            *c -= 1;
            *entry = RIGHT;
            if(HasTop(*r, *c)){
                *dir = (hand == RHAND)? H_D : LEFT;
            }else{
                *dir = (hand == RHAND)? LEFT : H_D;
            }
            
            break;
        case RIGHT:
            *c += 1;
            *entry = LEFT;
            if(HasTop(*r, *c)){
                *dir = (hand == RHAND)? RIGHT : H_D;
                
            }else{
                *dir = (hand == RHAND)? H_D : RIGHT;
            }
            
            break;
        case H_D:
            *entry = H_D;
            if(HasTop(*r, *c)){
                *dir = (hand == RHAND)? RIGHT : LEFT;
                *r -= 1;
                break;
            }
            else{
                *dir = (hand == RHAND)? LEFT : RIGHT;
                *r += 1;
                break;
            }
        default:
            return;
    }   
}


/**
*@brief Check if current position is end of the passage
*@param map Map ofthe  maze 
*@param r Current row number of cell
*@param c Current column of cell
*@param dir Border that indicates passage from the cell
*@return true if the current position is the end position, or false if it isn't
*/
bool isEnd(Map *map, int r, int c, int dir){
    if(r > map->rows || r < 1 || c > map->cols || c < 1){
        fprintf(stderr, "Error: Wrong final coordinates");
        return true;
    }
    if(r == 1 || r == map->rows || c == 1 || c == map->cols){
        if(c == 1 && !isborder(map, r, c, LEFT) && dir == LEFT && !isborder(map, r, c, dir)){
            return true;
        }
        else if(c == map->cols && !isborder(map, r, c, RIGHT) && dir == RIGHT && !isborder(map, r, c, dir)){
            return true;
        }
        else if(r == 1 && HasTop(r,c) && !isborder(map, r, c, H_D) && dir == H_D && !isborder(map, r, c, dir)){
            return true;
        }
        else if(r == map->rows && !HasTop(r,c) && !isborder(map, r, c, H_D) && dir == H_D && !isborder(map, r, c, dir)){
            return true;
        }
    }
    return false;
}

/**
*@brief Finds and prints a path out of the maze using Rhand or Lhand method
*@param r Current row number of cell
*@param c Current column of cell
*@param hand Defines which passage rule is used
*@return true passage of the maze ended or false if there is wrong starting position
*/
bool path(Map *map, int r, int c, int leftright){
    int dir = start_border(map, r, c, leftright);
    if(dir == 0){
        return false;
    }

    printf("%d,%d\n", r, c);
    int entry = starting_entry(map, r, c);  //decladre border of maze entrance
    dir = find_direction(map, r, c, dir, entry, leftright); 
    if(isEnd(map, r, c, dir)){   //if the first available passage leads out of the maze, end the function
        return true;
    }

    do{    
        move(&r, &c, &dir, &entry, leftright);
        printf("%d,%d\n", r, c);
        dir = find_direction(map, r, c, dir, entry, leftright);
    }while(!isEnd(map, r, c, dir));

    return true;
}


/**
*@brief Prints text with help instructions
*/
void help(){
    printf("Program runs with these arguments:\n");
    printf("--help\nPrints text with help instructions\n");
    printf("\n");
    printf("--test file.txt\nChecks if the file.txt is valid map of the maze\n");
    printf("\n");
    printf("--rpath R C file.txt\nSearches passage from the maze from R row and C colum using right hand method\n");
    printf("\n");
    printf("--lpath R C file.txt\nSearches passage from the maze from R row and C colum using left hand method\n");
}


int main(int argc, char *argv[]){
    int method, posx, posy;
    Map *map;
    if(argc == HELP && strcmp(argv[1], "--help") == 0){
        help();
        return 0;
    }
    
    
    else if(argc == TEST && strcmp(argv[1], "--test") == 0){
        map = map_ctor();
        if(map == NULL){
            return 1;
        }
        if(load(argv[2], map) == 0){
            printf("Invalid\n");
            map_dtor(map);
            return 0;
        }
        else{
            printf("Valid\n");
            map_dtor(map);
            return 0;
        }
    }


    else if(argc == PATH){
        if(strcmp(argv[1], "--rpath") == 0){
            method = RHAND;
        }
        else if(strcmp(argv[1], "--lpath") == 0){
            method = LHAND;
        }
        else{
            fprintf(stderr, "Error: wrong arguments");
            printf("run program with: --help\n");
            return 1;
        }

        posx = atoi(argv[2]);
        posy = atoi(argv[3]);
        map = map_ctor();
        if(map == NULL){
            map_dtor(map);
            return 1;
        }

        if(load(argv[4], map) == 0){
            map_dtor(map);
            return 1;
        }

        if(!path(map, posx, posy, method)){
            return 1;
        }
        map_dtor(map);
        return 0;
    }

    else{
        fprintf(stderr, "Error: Wrong arguments.\n");
        printf("run program with: --help\n");
        return 1;
    }
}