#include <stdio.h>
#include <stdlib.h>
#include <fcntl.h>
#include <sys/stat.h>
#include <limits.h>
#include <sys/wait.h>
#include <semaphore.h>
#include <sys/mman.h> 
#include <unistd.h>
#include <string.h>
#include <time.h>
#include <sys/shm.h>
#include <sys/ipc.h>
#include <stdarg.h>


FILE *file;
sem_t *bus = NULL;       // Semaphore to ensure correct boarding of passengers at specific stops
int *line_num = 0;       // Shared variable signaling the order of processes
sem_t *output = NULL;    // Semaphore for printout synchronization
sem_t *mutex = NULL;     // Main mutex for locking important processes for synchronization
int *pole[10];           // Array of pointers to integers for counting waiting passengers at bus stops
sem_t *konecna = NULL;   // Semaphore to signal that the bus has arrived at the final destination and passengers are about to leave
sem_t *mutex2 = NULL;    // Second main semaphore for shared variable buffer
int *buffer = 0;         // Shared variable initializing the number of passengers who have boarded
sem_t **stops = NULL;    // Array of semaphores for every bus stop

void flags(int argc, char *argv[]){
    // Check for correct number of arguments
    if(argc != 6){
        fprintf(stderr, "Wrong amount of arguments (requiered amount: 6 given amount: %d)\n", argc);
        exit(1);
    }
    // Check for valid skier count range
    if(atoi(argv[1]) <= 0 || atoi(argv[1]) >= 20000 ){
        fprintf(stderr, "Invalid number of skiers (required range: 1 to 20000)\n");
        exit(1);
    }
    // Check for valid bus stops count range
    if(atoi(argv[2]) < 0 || atoi(argv[2]) > 10 ){
        fprintf(stderr, "Invalid number of bus stops (required range: 1 to 10)\n");
        exit(1);
    }
    // Check for valid bus capacity range
    if(atoi(argv[3]) < 10 || atoi(argv[3]) > 100 ){
        fprintf(stderr, "Invalid bus capacity (required range: 10 to 100)\n");
        exit(1);
    }
    // Check for valid maximal waiting time for skiers range
    if(atoi(argv[4]) < 0 || atoi(argv[4]) > 10000 ){
        fprintf(stderr, "Invalid maximal waiting time for skiers in microseconds (required range: 0 to 10000)\n");
        exit(1);
    }
    // Check for valid maximal travel time for buses range
    if(atoi(argv[5]) < 0 || atoi(argv[5]) > 1000 ){
        fprintf(stderr, "Invalid maximal travel time for buses (required range: 1 to 1000)\n");
        exit(1);
    }
}



// Function to check for errors when calling munmap
void check_munmap_error(int status, const char *func_name) {
    if (status == -1) {
        perror(func_name);
        exit(EXIT_FAILURE);
    }
}


//inicialization of semaphores and shared variale
int semaphore_init(int zastavky){ 
    if((bus = mmap(NULL, sizeof(sem_t), PROT_READ | PROT_WRITE, MAP_ANONYMOUS | MAP_SHARED, -1, 0)) == MAP_FAILED) return 1;
    sem_init(bus, 1, 0);

    if((output = mmap(NULL, sizeof(sem_t), PROT_READ | PROT_WRITE, MAP_ANONYMOUS | MAP_SHARED, -1, 0)) == MAP_FAILED) return 1;
    sem_init(output, 1, 1);

    if((konecna = mmap(NULL, sizeof(sem_t), PROT_READ | PROT_WRITE, MAP_ANONYMOUS | MAP_SHARED, -1, 0)) == MAP_FAILED) return 1;
    sem_init(konecna, 1, 0);

    if((mutex2 = mmap(NULL, sizeof(sem_t), PROT_READ | PROT_WRITE, MAP_ANONYMOUS | MAP_SHARED, -1, 0)) == MAP_FAILED) return 1;
    sem_init(mutex2, 1, 1);

    if((buffer = mmap(NULL, sizeof(int), PROT_READ | PROT_WRITE, MAP_ANONYMOUS | MAP_SHARED, -1, 0)) == MAP_FAILED) return 1;

    if((line_num = mmap(NULL, sizeof(int), PROT_READ | PROT_WRITE, MAP_ANONYMOUS | MAP_SHARED, -1, 0)) == MAP_FAILED) return 1;
    *line_num = 1;

    if((mutex = mmap(NULL, sizeof(sem_t), PROT_READ | PROT_WRITE, MAP_ANONYMOUS | MAP_SHARED, -1, 0)) == MAP_FAILED) return 1;
    sem_init(mutex, 1, 1);

    for(int i = 0; i < zastavky; i++){
        if((pole[i] = mmap(NULL, sizeof(int), PROT_READ | PROT_WRITE, MAP_ANONYMOUS | MAP_SHARED, -1, 0)) == MAP_FAILED) return 1;
        *pole[i] = 0;
    }

    if((stops = mmap(NULL, sizeof(sem_t *) * zastavky, PROT_READ | PROT_WRITE, MAP_ANONYMOUS | MAP_SHARED, -1, 0)) == MAP_FAILED) return 1;
    for(int i = 0; i < zastavky; i++){
        if((stops[i] = mmap(NULL, sizeof(sem_t), PROT_READ | PROT_WRITE, MAP_ANONYMOUS | MAP_SHARED, -1, 0)) == MAP_FAILED) return 1;
        sem_init(stops[i], 1, 0);
    }
    return 0;
}

// Cleanup function for semaphores and shared variables
void cleanup(int zastavky){
    munmap(bus, sizeof(sem_t));
    sem_destroy(bus);
    
    munmap(line_num, sizeof(int));
    munmap(output, sizeof(sem_t));
    sem_destroy(output);
    munmap(mutex, sizeof(sem_t));
    sem_destroy(mutex);
    munmap(konecna, sizeof(sem_t));
    sem_destroy(konecna);
    munmap(mutex2, sizeof(sem_t));
    sem_destroy(mutex2);
    munmap(buffer, sizeof(int));
    for(int i = 0; i < zastavky; i++){
        munmap(pole[i], sizeof(int));
    }
    for(int i = 0; i < zastavky; i++){
        munmap(stops[i], sizeof(int));
    }
    munmap(stops, sizeof(sem_t));
    munmap(pole, sizeof(int));
    fclose(file);
    }


// Function for printing in correct order
void my_print(const char * format, ...){
    sem_wait(output);
    va_list args;
    va_start (args, format);
    fprintf(file, "%d: ", *line_num);
    (*line_num)++;
    vfprintf (file, format, args);
    fflush(file);
    va_end (args);
    sem_post(output);
}


// Function for checking if there are skiers waiting at any of the bus stops
int is_waitin( int lyz){
    sem_wait(mutex2);
    if (*buffer == lyz ) {  // Compare the number of passengers that went skiing to the total number of skiers
        sem_post(mutex2);
        return 0;
    }
    sem_post(mutex2);
    return 1;
}

// Function for passengers to disembark the bus
int final_stop(int kapacita, int max_doba, int zastavky, int used_seats, int lyz){
        srand(time(NULL) * zastavky); 
        usleep(rand() % max_doba+1); 
        my_print("BUS: arrived to final\n");
        for(int j = 0; j < used_seats && j < kapacita; j++){ 
            sem_post(konecna);
            sem_wait(bus);
            sem_wait(mutex2);
            (*buffer)++;
            sem_post(mutex2);
        }
    
        if(is_waitin(lyz) == 1){ // Check if there are any skiers waiting at bus stops
            return 1;
        }
        else{
            return 0;
        }
}


// Function for all bus processes
void bussing(int zastavky, int kapacita, int max_doba, int lyz){
    int used_seats = 0;
    int idZ = 0;
    my_print("BUS: started\n");
    while(1){
        srand(time(NULL) * idZ);
        usleep(rand() % (max_doba+1));
        
        if(idZ  == zastavky){   //when bus reached final start passenger dropping out
            if(final_stop(kapacita, max_doba, zastavky, used_seats, lyz) == 0){
                my_print("BUS: leaving final\n");
                break;
            }
            else{
                idZ = 0;
                used_seats= 0;
                my_print("BUS: leaving final\n");
            }
        }
        sem_wait(mutex);
        idZ += 1;
        my_print("BUS: arrived to %d\n", idZ);
        int waiters = (*pole[(idZ) - 1]); // Copy the number of passengers waiting at the stop into a variable
        sem_post(mutex);
        for(int i = 0; i < waiters && used_seats < kapacita; i++){  // Loop until the stop is empty or capacity is reached
            used_seats++;
            sem_post(stops[idZ -1]);    // Decrement the number of waiting passengers at the current bus stop
            sem_wait(bus);  // Continue only when the passenger has boarded the bus (waiting for signal from passenger process)
        }
        my_print("BUS: leaving %d\n", idZ);
    }
}

// Function for all skiers' activities
void skiers_doing_stuff(int id, int wait_stop, int wait_time){
    my_print("L %d: started\n", id);
    usleep(wait_time);  // Wait for a certain time to arrive at the stop
    sem_wait(mutex);
    (*pole[wait_stop]) += 1;    // Increment the number of waiting passengers at the specified bus stop
    my_print("L %d: arrived to %d\n", id, wait_stop+1);
    sem_post(mutex);

    // Skiers wait to board the bus (waiting for signal from bus process)
    sem_wait(stops[wait_stop]);
    my_print("L %d: boarding\n", id);
    sem_wait(mutex);
    (*pole[(wait_stop)]) -= 1;
    sem_post(mutex);
    sem_post(bus);

    // Skiers wait to leave the bus (waiting for signal from bus process)
    sem_wait(konecna);
    my_print("L %d: going to ski\n", id);
    sem_post(bus);

}


/*##########################################################################################################################################*/


int main(int argc, char *argv[]){
    flags(argc, argv);

    if((file = fopen("proj2.out", "w")) == NULL){
        fprintf(stderr, "Can't open file proj2.out\n");
        return 1;
    }

    // Extracting arguments into variables
    int skier_count = atoi(argv[1]);
    int bus_stops = atoi(argv[2]);
    int bus_capacity = atoi(argv[3]);
    int max_time_to_wait = atoi(argv[4]);
    int max_time_between_stops = atoi(argv[5]);

    
    if(semaphore_init(bus_stops) == 1) return 1;

    pid_t bus_process, skiers_process;
    bus_process = fork();
    if(bus_process < 0){
        fprintf(stderr, "Failed to fork bus process");
        cleanup(bus_stops);
        exit(1);
    }
    else if(bus_process == 0){
        bussing(bus_stops, bus_capacity, max_time_between_stops, skier_count);
        cleanup(bus_stops);
        exit(0);
    }
    
    for(int i = 0; i < skier_count; i++){
        skiers_process = fork();
        if(skiers_process < 0){
            fprintf(stderr, "Failed to fork skier process\n");
            cleanup(bus_stops);
            exit(1);
        }
        else if(skiers_process == 0){
            srand((unsigned) i);
            skiers_doing_stuff(i + 1, rand() % bus_stops, rand() % (max_time_to_wait + 1));
            cleanup(bus_stops);
            exit(0);
        }
    }

    while(wait(NULL) > 0);
    
    my_print("BUS: finish\n");
    cleanup(bus_stops);
    return 0;
}
