#include <stdio.h>
#include <stdlib.h>
#include <omp.h>
#define NUM_CHARS 4
char *buffer;
int count[NUM_CHARS] = {0}; 
void computer_histogram(char *buffer, int file_size, int num_threads){
    int batch_size = file_size/num_threads;
    printf("batch_size: %d\n", batch_size);
    if (num_threads == 0){//sequential
        for (int i = 0; i < file_size; i++)
            count[(int)buffer[i] - (int)'a']++;
        
    } else {
        // Initialize all local counts to zero
        int **local_count = NULL;
        int rows = num_threads+1;
        int cols = NUM_CHARS;
        local_count = (int **)malloc(rows * sizeof(int *));
        for (int i = 0; i < rows; i++) {
            local_count[i] = (int *)malloc(cols * sizeof(int));
            for (int j = 0; j < cols; j++)
                local_count[i][j] = 0;
        }

        int tid = omp_get_thread_num();
        #pragma omp parallel for
        for (int batch_id = 0; batch_id < num_threads; batch_id++){
            for (int i = batch_id * batch_size; i < (batch_id + 1) * batch_size; i++)
                local_count[tid][(int)buffer[i] - (int)'a']++;
        }
        #pragma omp parallel for
        for (int i = 0; i < NUM_CHARS; i++){
            for (int t = 0; t < num_threads; t++)
                #pragma omp atomic
                count[i] += local_count[t][i];
        }
        
        #pragma omp parallel for
        for (int i = 0; i < rows; i++)
            free(local_count[i]);
        free(local_count);
    }


}

void check_valid(int argc){
    if(argc != 4 ){
	printf("usage: ./calculate N num filename\n");
    printf("N number of threads, can be 0 (purely sequential), 1 (OpenMP version with  one thread), or 4 (OpenMP with four threads). \n");
	printf("file_size: number of characters in the file\n");
	printf("filename: This is the name of the file that contains the characters. \n");
 	exit(1);
    }
}
void read_buffer(char *filename, long file_size){
    size_t result;
    FILE *file_ptr;
    file_ptr = fopen(filename, "rb");
    if (file_ptr == NULL){
        fputs("File error", stderr);
        exit(1);
    }
    buffer = (char*) malloc(sizeof(char) * (file_size + 1));
    if (buffer == NULL){
        fputs("Memory error", stderr);
        exit(2);
    }
    result = fread(buffer, 1, file_size, file_ptr);
    if (result != file_size){
        fputs("Reading error", stderr);
        exit(3);
    }
    buffer[file_size] = '\0';
    fclose(file_ptr);
}
int main(int argc, char *argv[]){
    check_valid(argc);
    int num_threads = strtol(argv[1], NULL, 10);
    long file_size = strtol(argv[2], NULL, 10);
    char *filename = argv[3];
    //Read a character
    read_buffer(filename, file_size);
    //Calculate the histogram parallely
    computer_histogram(buffer, file_size, num_threads);
    // Print out histogram
    int max_idx = 0;
    int max = 0;
    for (int i = 0; i < NUM_CHARS; i++) {
        if (count[i] > max){
            max = count[i];
            max_idx = i;
        }
        printf("Character '%c' appears %d times.\n", (char)(i+'a'), count[i]);
    }
    printf("%c occurred the most %d times of a total of %ld characters.\n",(char)(max_idx+'a'), count[max_idx], file_size);
    free(buffer);
    return 0;
}