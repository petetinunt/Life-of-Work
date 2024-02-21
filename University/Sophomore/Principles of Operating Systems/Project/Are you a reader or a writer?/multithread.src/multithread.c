#include<stdio.h>
#include<stdlib.h>
#include<time.h>
#include<string.h>
#include<stdbool.h>
#include <pthread.h>

int argWriter;          // Writer argument command line
int argReader;          // Reader argument command line
char argInputfile[50];  // Input file argument command line (.txt)
char argOutputfile[50]; // Output file argument command line
int argSeed ;           // Seed argument command line
int arg_N;              // N s argument command line

int AR = 0;     //active reader
int AW = 0;     //active writer
int WR = 0;     //wait reader
int WW = 0;     //wait write
int max;

typedef struct node List;

pthread_mutex_t lock = PTHREAD_MUTEX_INITIALIZER;               // Lock variable
pthread_cond_t okToRead = PTHREAD_MUTEX_INITIALIZER;            // okToRead variable used to check that it is good to get in critical region or not
pthread_cond_t okToWrite = PTHREAD_MUTEX_INITIALIZER;           // okToWrite variable used to check that it is good to get in critical region or not

#define DATABASE_SIZE 10                // Define the initiate of the size to be 10 
int database[DATABASE_SIZE];            // Database variable use to store the buffer

void AccessDatabase(int type, int r, int i) {
    if(type==0) {}                          // If it is a reader thread(read), print
    else if(type==1) database[r]++;         // If it is a writer thread(readwrite), increase data and print
}

void write_to_file(char* filename, int* str, int size) {
    FILE* fp = fopen(filename, "w");                                // File pointer use to open the file
    if (fp == NULL) {                                               // If the pointer equal to NULL
        printf("Error: Failed to open file %s\n", filename);        // Show that it can't open
        return;                                                     // Return to stop the function
    }
    for (int i = 0; i < size; i++) fprintf(fp, "%d\n", str[i]);     // Looping to print all of the values in buffer.
    fclose(fp);                                                     // Close file
}


void *Reader(void *args) {
    int thread_no = *(int*)args;                // Thread number(ID)
    pthread_mutex_lock(&lock);                  // Lock the mutex
    while ((AW + WW) > 0) {                     // Is it safe to read?
        WR++;                                   // Wait Reader
        pthread_cond_wait(&okToRead, &lock);    // Sleep on cond var
        WR--;                                   // No longer waiting
    }
    AR++;                                       // Active reader on
    pthread_mutex_unlock(&lock);                // Unlock the mutex
    int r = rand() % 9;                         // random the number in range of 0-8
    AccessDatabase(0,r,(long)_threadid);        // Access to see whether it is reader or writer
    pthread_mutex_lock(&lock);                  // Lock the mutex
    AR--;                                       // Stop active reader

    if (AR == 0 && WW > 0)                      // No other active readers
        pthread_cond_signal(&okToWrite);        // Wake up one writer
        pthread_mutex_unlock(&lock);            // Unlock the mutex
    // There is the problem with writing in the file, so I'm using this to randomly input it first.
    srand(time(NULL));
    int rannum = rand()%9;                      // I use random input with 9 modulo to keep the value in the range of 9
    int size = (rand()%arg_N + 1);              // And the size is from the project assignment
    // The write_to_file function will use the outpufile's parameter to assign the value in it.
    write_to_file(argOutputfile, &rannum, size);
    printf("Read %d DONE\n",thread_no);         //thread done
}
    
void *Writer(void *threadid){                   // called the threads that want to write
    int thread_no = *(int*)threadid;
    int i=0;
    while(i<10){                                // Making 10 operation of writer
        pthread_mutex_lock(&lock);              // Lock the mutex
        while((AW+ AR) > 0) {                   // Check whether there are any active writers or readers or not
            WW++;                               // Waiting Writer increase
            pthread_cond_wait(&okToWrite, &lock);   // Waiting for okToWrite
            WW--;                               // Waiting Writer decrease
        }
        AW++;                                   // Active Writer access in critical region 
        pthread_mutex_unlock(&lock);            // Unlock the mutex
        int r = rand() % 9;                     // Random number 0-9 for index of Database
        AccessDatabase(1,r,(long)_threadid);    // Access Database
        pthread_mutex_lock(&lock);              // Lock the mutex
        AW--;                                   // no longer active in critical region
        if (WW > 0)  pthread_cond_signal(&okToWrite);       // If there is waiting writer, wake up one writer
        else if(WR > 0) pthread_cond_signal(&okToRead);     // If there is waiting reader, wake up all readers
        pthread_mutex_unlock(&lock);            // Unlock the mutex
        i++;                                    // Increase the index of i by 1 to check the while loop
    }
    printf("Write %d DONE\n",thread_no);        // Writer thread done
    return NULL;
}
void readFromFile(char* filename) {
    FILE* ptr;                                  // File Pointer
    char ch;                                    // Content in file
 
    ptr = fopen("test.txt", "r");                           // Open the file
    if (NULL == ptr) printf("file can't be opened \n");     // If the pointer is emtpy
    // Show the content in file
    printf("content of this file are \n");                  // Show the content inside the file
    do {
        ch = fgetc(ptr);                                    // Assign the content in each pointer
        printf("%c", ch);                                   // Print the content
    } while (ch != EOF);                                    // While condition the if ch not empty
    fclose(ptr);                                // Close the file
}

void removelist(List *root) {
    while(root != NULL) free(root);             // Removing list
    return;
}

int main(int argc, char *argv[]) {
    argWriter = atoi(argv[1]);              // Writer argument command line
    argReader = atoi(argv[2]);              // Reader argument command line
    strcpy(argInputfile , argv[3]);         // Input file argument command line (.txt)
    strcpy(argOutputfile , argv[4]);        // Output file argument command line
    argSeed = atoi(argv[5]) ;               // Seed argument command line
    arg_N = atoi(argv[6]);                  // N s argument command line
    
    pthread_mutex_init(&lock, NULL);        // Initailize the value of lock
    pthread_cond_init(&okToRead, NULL);     // Initailize the value of okToRead
    pthread_cond_init(&okToWrite, NULL);    // Initailize the value of okToWrite

    int maxWriterReader = 10;               // Assign the value for maxWriterReader
    pthread_t readers[maxWriterReader];     // Create thread for reader 
    pthread_t writers[maxWriterReader];     // Create thread for writer
    int reader_args[maxWriterReader];       // Create variable argurment for reader 
    int writer_args[maxWriterReader];       // Create variable argurment for writer

    // If the number of both is more than the limit; return 0;
    if( (argWriter + argReader > (maxWriterReader * 2)) ) {     
        printf("# of writer & # of reader must in range [1..%d].\n" , maxWriterReader);
        return 0;
    }

    int iResult = 0;                        // Create variable iResult which is integer
    List *root, *last, *current;            // Create list of root, last and current to store list
    List *temp;                             // Temporary list for store the pointer
    int num_writers=0, num_readers = 0;     // Number of writer and reader to count

    if(iResult == 0) {                      // If the result equal to zero
        srand(argSeed);                     // Random Agrument seed
        // Reader
        for (int i = 0; i < argReader; i++) {                                                   // Then loop for the index of Reader number
            reader_args[num_readers] = num_readers;                                             // Assign the value into the buffer
            pthread_create(&readers[num_readers], NULL, Reader, &reader_args[num_readers]);     // Create the pthread by using the assign agrs
            num_readers++;                                                                      // Increase the number of readers
        }
        // Writer 
        for (int i = 0; i < argWriter; i++) {                                                   // Loop for the index of Writer number
            writer_args[num_writers] = num_writers;                                             // Assign the value into the buffer 
            pthread_create(&writers[num_writers], NULL, Writer, &writer_args[num_writers]);     // Create the pthread by using the assign agrs
            num_writers++;                                                                      // Increase the number of writer
        }
    }
    for (int i = 0; i < argWriter; i++) pthread_join(writers[i], NULL);     // Wait for exit
    for (int i = 0; i < argReader; i++) pthread_join(readers[i], NULL);     // Wait for exit
    
    removelist(root);                       // Remove the root from the list

    pthread_mutex_destroy(&lock);           // Destroy the lock after finish all
    pthread_cond_destroy(&okToRead);        // Destroy the okToRead after finish all
    pthread_cond_destroy(&okToWrite);       // Destroy the okToWrite after finish all

    return 0;

}