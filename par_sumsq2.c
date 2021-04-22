/*
 * sumsq.c
 *
 * CS 446.646 Project 1 (Pthreads)
 *
 * Compile with --std=c99
 */

/*******  Edited by Jacob McAllister for the purpose of CS 446.646 Project 5    *****************/
//  Had to do a lot of brushing up on C coding, primary code in C++
//  The Steps are related to my personal thought process and do not mean a direct correlation
//  to how I solved every problem.



/*  STEP 1 - Lets add the pthread library */

#include <pthread.h>

//  STEP 1 - Done

#include <limits.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

// aggregate variables
long sum = 0;
long odd = 0;
long min = INT_MAX;
long max = INT_MIN;
volatile bool done = false;
//  Queue List Logic

typedef struct QueueNode{
	long key;
	struct QueueNode* next;
}QueueNode;

typedef struct Queue{
	QueueNode *front, *rear;
}Queue;

QueueNode* newNode(long k){
	QueueNode* temp = (struct QueueNode*)malloc(sizeof(struct QueueNode));
	temp->key = k;
	temp->next = NULL;
	return temp;
}

Queue* createQueue(){
	Queue* q = (struct Queue*)malloc(sizeof(struct Queue));
	q->front = q->rear = NULL;
	return q;
}

void enQueue(Queue* q, long k){

	QueueNode* temp = newNode(k);

	if(q->rear == NULL){
		q->front = q->rear = temp;
		return;
	}

	q->rear->next = temp;
	q->rear = temp;
}

long deQueue(Queue* q){

	if(q->front == NULL)
		return 0;
	
	QueueNode* temp = q->front;
	
	long key = q->front->key;
	q->front = q->front->next;

	if(q->front == NULL)
		q->rear = NULL;	
	free(temp);
return key;
}

volatile Queue* workingQueue;

void calculate_square(long number);

pthread_mutex_t lockAgg = PTHREAD_MUTEX_INITIALIZER;
pthread_mutex_t lockWork = PTHREAD_MUTEX_INITIALIZER;

//  The conditional for our workers (suggested in the hints)
pthread_cond_t newWorkerCondition = PTHREAD_COND_INITIALIZER;

void* start_routine(void* routine){
	
	while(!done){

		pthread_mutex_lock(&lockWork);

		if(workingQueue->front && !done){
			long number = deQueue(workingQueue);
			pthread_mutex_unlock(&lockWork);
			calculate_square(number);

//			pthread_cond_signal(&newWorkerCondition);	
			
		}
		else if(done){
			pthread_mutex_unlock(&lockWork);
			break;
		}
		else{
			pthread_cond_wait(&newWorkerCondition, &lockWork);
		}
	pthread_mutex_unlock(&lockWork);
	}
/*

while(!done){

	pthread_mutex_lock(&lockWork);
	while(!workingQueue->front && !done){
		pthread_cond_wait(&newWorkerCondition, &lockWork);
	}
	
	if(done){
		pthread_mutex_unlock(&lockWork);
		break;
	}
	
	long number= deQueue(workingQueue);
	pthread_mutex_unlock(&lockWork);
	
	calculate_square(number);
}

*/
}


//void calculate_square(long number);

/*
 * update global aggregate variables given a number
 */

/*  STEP 5 - Update calculate_square (I don't know why but, this was the roughest spot for me) */
//  Originally tried a more complex approach where I was grabbing a lock for every variable
//  Then I tried only when it was updating....
//  Eventually I just added two simple lines of code.
void calculate_square(long number)
{

  // calculate the square
  long the_square = number * number;

  // ok that was not so hard, but let's pretend it was
  // simulate how hard it is to square this number!
  sleep(number);


//  Line 1 - we grab the lock
pthread_mutex_lock(&lockAgg);

  // let's add this to our (global) sum
  sum += the_square;

  // now we also tabulate some (meaningless) statistics
  if (number % 2 == 1) {
    // how many of our numbers were odd?
    odd++;
  }

  // what was the smallest one we had to deal with?
  if (number < min) {
    min = number;
  }

  // and what was the biggest one?
  if (number > max) {
    max = number;
  }

//  Line 2 - we set it free
pthread_mutex_unlock(&lockAgg);
}


int main(int argc, char* argv[])
{
	// check and parse command line options
	if (!(argc == 2 || argc == 3)){
		printf("Usage: par_sumsq <infile> <number of workers>\n");
		exit(EXIT_FAILURE);
	}

	// load numbers and add them to the queue
	char *fn = argv[1];
	FILE* fin = fopen(fn,"r");
	
	//  Lets collect how many workers we need our default will be 1
	long numWorkers = 1;
	if(argc == 3){
		// strtol usage strtol(const char *start, char **end, int base);
		const long commandInput = strtol(argv[2], NULL, 10);
		numWorkers = commandInput;
	 }

//	volatile Queue* workingQueue;	
	workingQueue = createQueue();
	// 4.b we make the workers by making an array of pthreads!
	pthread_t worker[numWorkers];
	// pthread_create usuage pthread_create(pthread_t *thread, const pthread_attr_t *attr, void*(*start_routine) (void *), void *arg);
	// step 4.3 will be create the start_routine
	for(long i = 0; i<numWorkers; ++i){
	//	pthread_create(&worker[i], NULL, start_routine, (void*) workingQueue);
		pthread_create(&worker[i], NULL, start_routine, NULL);
	}

//  STEP 4.D - Add the task to the queue
//  Edit given code to make it happen

	char action;
	long num;

	while(fscanf(fin, "%c %ld\n", &action, &num) == 2){

		if(action == 'p'){
			//  printf(" %ld ", num);
			//  Got process, so lets work
			//  First grab our mutex
//			printf(" %ld", num);
			pthread_mutex_lock(&lockWork);
			
			//  Now we put in on the queue
			enQueue((Queue*)workingQueue, num);
			
			//  Now we singal the threads
			//  pthread_cond_signal usuage pthread_cond_signal(pthread_cond_t *cond) 
			//  Calls unblocks at least one of the threads that are blocked on the condition variable, if any are blocked
			pthread_cond_signal(&newWorkerCondition);
		
			//  Now we free the mutex
			pthread_mutex_unlock(&lockWork);
		}
		else if(action == 'w'){
			//  Got sleep, so nap time
			sleep(num);
		}
		else{
			printf("Got Unrecognized action:  %c.  Doing nothing with this action!\n", action);
		}
	}

fclose(fin);

//  STEP 4 done!

//  Lets put a busy wait in for our master thread;
while(workingQueue->front){
  //  Buzz Buzz
//printf(" HUG ");
}

done = true;

//  Lets grab the mutex
pthread_mutex_lock(&lockWork);

//  Now we will unblock all threads
//  pthread_cond_broadcast usuage pthread_cond_broadcast(pthread_cond_t *cond)
//  Calls unblocks to all threads that are currently blocked with condition variable
pthread_cond_broadcast(&newWorkerCondition);

//  Let the mutex be freeeee
pthread_mutex_unlock(&lockWork);

//  Let all threads finish and terminate
for(long i = 0; i < numWorkers; ++i){
	pthread_join(worker[i], NULL);
}


printf("%ld %ld %ld %ld\n", sum, odd, min, max);

  return (EXIT_SUCCESS);
}

