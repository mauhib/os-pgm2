#include <stdio.h>
#include <fcntl.h>
#include <semaphore.h>
#include <unistd.h>
#include <sys/mman.h>
#include <time.h>
#include <stdlib.h>
#include <pthread.h>

#define SEM_S_NAME "sems"
#define SEM_N_NAME "semn"

#define SHM_SIZE 80000 //Items

#define DELAY_OF_OPERATIONS 0
#define MAX_ITEMS 50000

#define WAIT_WAY 0
#define DEBUG 0

//The buffer being shared
int32_t *ptr;
//Semaphores
sem_t *s;
sem_t *n;
//Filenames
char* producer_file_time;
char* producer_file_n;
char* consumer_file_time;
char* consumer_file_n;

void print_double_array(double *arr, size_t len, char* filename){
  FILE *fp;
  fp = fopen(filename, "w+");
  if(!fp){
    printf("Could not open file.\n");
    return;
  }
  int i;
  for(i=0;i<len;i++){
    fprintf(fp, "%.20f\n",arr[i]);
  }
  fclose(fp);
}

void print_int_array(int32_t *arr, size_t len, char* filename){
  FILE *fp;
  fp = fopen(filename, "w+");
  int i;
  for(i=0;i<len;i++){
    fprintf(fp, "%d\n",arr[i]);
  }
  fclose(fp);
}

void *producer_function(){
  printf("Producer Started\n");
  //Allocate storage for results
  double *time_storage = calloc(MAX_ITEMS,sizeof(double));
  if(!time_storage){
    printf("Couldn't allocate time_storage\n");
    return NULL;
  }
  int32_t *number_storage = calloc(MAX_ITEMS,sizeof(int32_t));
  if(!number_storage){
    printf("Couldn't allocate number_storage\n");
    return NULL;
  }

	struct timespec start,end; //For the timer
  int32_t produced_value = 0;
  srand(time(0)); //Seed Random

	//Loop and add items to the buffer
	//Use the first element of ptr as the loop index
  for(ptr[0]=MAX_ITEMS;ptr[0]>0;ptr[0]--){
    clock_gettime(CLOCK_MONOTONIC, &start); //start clock
    produced_value = rand();
#if DEBUG
    printf("Producer: produce() => %d \n", produced_value);
#endif
#if WAIT_WAY == 0
    sem_wait(s);
#else
    while(sem_trywait(s) < 0){
    }
#endif
    sem_getvalue(n,&(number_storage[MAX_ITEMS - ptr[0]]));
    if(DELAY_OF_OPERATIONS) sleep(DELAY_OF_OPERATIONS);
#if DEBUG
    printf("Producer: append(); n = %d\n",number_storage[MAX_ITEMS - ptr[0]]);
#endif
    ptr[number_storage[MAX_ITEMS - ptr[0]]+1] = produced_value;
    sem_post(s);
    sem_post(n);
    clock_gettime(CLOCK_MONOTONIC, &end); //end clock
    time_storage[MAX_ITEMS - ptr[0]] = (end.tv_sec - start.tv_sec) + ((end.tv_nsec - start.tv_nsec)/10e9);
  }
  //Output results
  printf("Producer: Outputting Time\n");
  print_double_array(time_storage, MAX_ITEMS, producer_file_time);
  printf("Producer: Outputting Items in Array\n");
  print_int_array(number_storage, MAX_ITEMS, producer_file_n);

  //Clean allocation
  free(time_storage);
  free(number_storage);

  printf("Producer Exitting\n");
	return NULL;
}

void *consumer_function(){
  printf("Consumer Started\n");
  //Allocate storage for results
  double *time_storage = calloc(MAX_ITEMS+1,sizeof(double));
  if(!time_storage){
    printf("Couldn't allocate time_storage\n");
    return NULL;
  }
  int32_t *number_storage = calloc(MAX_ITEMS,sizeof(int32_t));
  if(!number_storage){
    printf("Couldn't allocate number_storage\n");
    return NULL;
  }

  struct timespec start,end; //For the timer
  int32_t taken_value;
  int count;

	//Loop until all items are gone and the producer is done adding
  for(count = 0; ; count++){
    clock_gettime(CLOCK_MONOTONIC, &start); //start clock
#if WAIT_WAY == 0
    sem_wait(n);
    sem_wait(s);
#else
    while(sem_trywait(n) < 0);
    while(sem_trywait(s) < 0);
#endif
    sem_getvalue(n,&(number_storage[count]));
    if(DELAY_OF_OPERATIONS) sleep(DELAY_OF_OPERATIONS);
#if DEBUG
    printf("\t\tConsumer: take(); n = %d\n",number_storage[count]);
#endif
    //Take
    taken_value = ptr[number_storage[count]+1];

    sem_post(s);
    //Consume
#if DEBUG
    printf("\t\tConsumer: consume() => %d\n", taken_value);
#endif
      //Doing a NOP here just so the time isn't zero
      taken_value = taken_value / 5;

    clock_gettime(CLOCK_MONOTONIC, &end); //end clock
    time_storage[MAX_ITEMS - ptr[0]] = (end.tv_sec - start.tv_sec) + ((end.tv_nsec - start.tv_nsec)/10e9);
    if(!(number_storage[count] || ptr[0])) break;
  }
  //Output results
  printf("Consumer: Outputting Time\n");
  print_double_array(time_storage, MAX_ITEMS, consumer_file_time);
  printf("Consumer: Outputting Items in Array\n");
  print_int_array(number_storage, MAX_ITEMS, consumer_file_n);

  //Clean Allocation
  free(time_storage);
  free(number_storage);

  printf("Consumer Exitting\n");
	return NULL;
}

int main(int argc, char **argv)
{
	//Print Usage
  if(argc < 5){
    printf("Usage: ./pgm1 file1 file2 file3 file4\n");
    printf("file1: Producer Time Output\n");
    printf("file2: Producer Items in Array Output\n");
    printf("file3: Consumer Time Output\n");
    printf("file4: Consumer Items in Array Output\n");
    return -2;
  }

  //Initialize Semaphores and output starting values
  int temp = 0;
  //s = sem_open(SEM_S_NAME, O_CREAT | O_EXCL, 0644, 1);
	s = calloc(1,sizeof(sem_t));
	sem_init(s,0,1);
	sem_getvalue(s,&temp);
  printf("Main: value of s = %d\n",temp);
  if(s == SEM_FAILED){
    printf("Can't create sem s\n");
    return -1;
  }
  //n = sem_open(SEM_N_NAME, O_CREAT | O_EXCL, 0644, 0);
	n = calloc(1,sizeof(sem_t));
	sem_init(n,0,0);
	sem_getvalue(n,&temp);
  printf("Main: value of n = %d\n",temp);

  if(n == SEM_FAILED){
    printf("Can't create sem n\n");
    return -1;
  }
  //Done Initializing Semaphores

  //Allocate memory
	ptr = calloc(SHM_SIZE,sizeof(int32_t));

	//Assign filename pointers
	producer_file_time = argv[1];
	producer_file_n = argv[2];
	consumer_file_time = argv[3];
	consumer_file_n = argv[4];

	//New Threads
	pthread_t consumer_thread;
	pthread_t producer_thread;
	pthread_create(&consumer_thread, NULL, consumer_function, NULL);
	pthread_create(&producer_thread, NULL, producer_function, NULL);

	//Join threads to this one
	pthread_join(producer_thread,NULL);
	pthread_join(consumer_thread,NULL);

	//Cleanup
	printf("Now Cleaning\n");
	free(ptr);
	//sem_unlink(SEM_S_NAME);
	//sem_unlink(SEM_N_NAME);
	sem_destroy(s);
	sem_destroy(n);
	free(s);
	free(n);

  return 0;
}
