/*
 * sumsq.c
 *
 * CS 446.646 Project 1 (Pthreads)
 *
 * Compile with --std=c99
 * Note:  During make, errors will appear in regards to the volatile Queue, 
 *	  this is due to the queue being a global.
 */

/*******  Edited by Jacob McAllister for the purpose of CS 446.646 Project 5    *****************/
/*
*	Didn't do a modifcation history, sorry.  Just casually tapped away here and their till I was done.
*
*
*/
//  Had to do a lot of brushing up on C coding, primary code in C++
//  The Steps are related to my personal thought process and do not mean a direct correlation
//  to how I solved every problem.
//  I have multiple methods within my code commented out as I was messing with various ways
//  of using pthreads.  For example:  I original was passing my Queue by argument and then 
//  switched to a global because of the suggest hints.
//  This program also uses almost no error checking, so be kind.


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

//  We will add a volatile bool value to track when all collection of data is complete.
volatile bool done = false;

/* STEP 2 - Build my queue list for work. */
//  Queue List Logic
//  This is a common struct set up for a Queue list.

/*
*		QueueNode-Struct 
*
*/
typedef struct QueueNode{
	long key;
	struct QueueNode* next;
}QueueNode;

/*
*		Queue Struct
*
*/
typedef struct Queue{
	QueueNode *front, *rear;
}Queue;

/*
*		QueueNode - NewNode
*		Constructor
*
*/
QueueNode* newNode(long k){
	QueueNode* temp = (struct QueueNode*)malloc(sizeof(struct QueueNode));
	temp->key = k;
	temp->next = NULL;
	return temp;
}

/*
*		Queue* - createQueue
*		Copy Constructor
*		
*/
Queue* createQueue(){
	Queue* q = (struct Queue*)malloc(sizeof(struct Queue));
	q->front = q->rear = NULL;
	return q;
}

/*
*		Function - enQueue
*		Used in conjuction with Queue Struct to enqueue into the queue.
*/
void enQueue(Queue* q, long k){

	QueueNode* temp = newNode(k);

	if(q->rear == NULL){
		q->front = q->rear = temp;
		return;
	}

	q->rear->next = temp;
	q->rear = temp;
}

/*
*		Function - deQueue
*		Used in conjuction with Queue struct to dequeue from the queue.
*/
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
//  End of Queue list

//  We will make our list volatile so the compiler doesn't try to optimize
volatile Queue* workingQueue;

//  our given function
void calculate_square(long number);

//  These are going to be our two mutex locks, one for the Aggregates and the other for the workers
pthread_mutex_t lockAgg = PTHREAD_MUTEX_INITIALIZER;
pthread_mutex_t lockWork = PTHREAD_MUTEX_INITIALIZER;

//  The conditional for our workers
pthread_cond_t newWorkerCondition = PTHREAD_COND_INITIALIZER;

//  This is our start routine for the workers.
//  I have two different logics in here, both work... was trying to think of which is 
//  the more efficient method.

/*
*		Function - start_routine
*		Works in conjuction with pthreads as start routine.
*/
void* start_routine(void* routine){
	
	// Loop runs until there is no more work
	while(!done){

		//  First we grab our lock
		pthread_mutex_lock(&lockWork);

		//  If the front of the queue is not NULL and we are not done... guess we have work to do.
		if(workingQueue->front && !done){
			long number = deQueue(workingQueue);
			pthread_mutex_unlock(&lockWork);
			calculate_square(number);
			
		}
		//  Else if the work has been signaled by master to be done, we break from loop
		else if(done){
			pthread_mutex_unlock(&lockWork);
			break;
		}
		//  Else we just wait with our conditional wait
		else{
			pthread_cond_wait(&newWorkerCondition, &lockWork);
		}
	pthread_mutex_unlock(&lockWork);
	}
/*

//  Similar logic as above, instead we use a while loop to hold our condition
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
//  End of start routine.

//void calculate_square(long number);

/*
 * update global aggregate variables given a number
 */

/*  STEP 5 - Update calculate_square (I don't know why but, this was the roughest spot for me) */
//  Originally tried a more complex approach where I was grabbing a lock for every variable
//  Then I tried only when it was updating....
//  Eventually I just added two simple lines of code.

/*
*		Function - calculate_square
*		Calculates the square of passed parameter value.
*/
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

	/*  STEP 3 - lets us build our workers */
	//  3.1 - Open the input file. 
	char *fn = argv[1];
	FILE* fin = fopen(fn,"r");
	
	//  3.2 - Lets collect how many workers we need with our default will be 1
	long numWorkers = 1;
	if(argc == 3){
		// strtol usage strtol(const char *start, char **end, int base);
		const long commandInput = strtol(argv[2], NULL, 10);
		numWorkers = commandInput;
	 }

	//  3.3 - Create our workers
	//  Method of  passing as arg has been commented out.
	
	//  Creating an array of pthreads named worker.
	pthread_t worker[numWorkers];
	
	//  Use this if you wish to pass an argument.
//	volatie Queue* workingQueue;

	//  Note:  You have to make the Queue before the workers as they
	//  will be referencing it in there routine.  It will not work
	//  if you create the Queue after.
	workingQueue = createQueue();

	// pthread_create usuage pthread_create(pthread_t *thread, const pthread_attr_t *attr, void*(*start_routine) (void *), void *arg);
	for(long i = 0; i<numWorkers; ++i){
	//	pthread_create(&worker[i], NULL, start_routine, (void*) workingQueue);
		pthread_create(&worker[i], NULL, start_routine, NULL);
	}
	//  STEP 3 - Done	

	/* STEP 4 - Build our queue */
	

	//  Parse the infile and enQueue when encountering a P
	//  Parsed data will be encountered as  Character + long number.  Example: "P 2".
	//  Edit given code to make it happen

	char action;
	long num;

	while(fscanf(fin, "%c %ld\n", &action, &num) == 2){

		if(action == 'p'){
			//  printf(" %ld ", num);
			//  Got process, so lets work
			//  First grab our lock
			pthread_mutex_lock(&lockWork);
			
			//  Now we put in on the queue
			enQueue((Queue*)workingQueue, num);
			
			//  Now we singal the threads
			//  pthread_cond_signal usuage pthread_cond_signal(pthread_cond_t *cond) 
			//  Calls unblocks at least one of the threads that are blocked on the condition variable, if any are blocked
			//  To stop us from hitting a deadlock.
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
//  This will allow our workers to finish their work.
//  Note:  If the queue is not volitale, the busy wait will
//  NOT function properly and the program will be stuck here.
while(workingQueue->front){
  //  Buzz Buzz
//printf(" HUG ");
}

//  Busy wait has finished, thus work is all done!
//  Our master thread sends the signal.
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
//  The clean up, no need for calling pthread_exit()
//  as all threads use return as their exit status
//  back to main.
//  pthread_join usuage - pthread_join(pthread_t thread, void **value_ptr);
//  "The pthread_join() function shall suspend execution of the valling thread until the target thread terminates,
//   unless the target thread has already terminated."
for(long i = 0; i < numWorkers; ++i){
	pthread_join(worker[i], NULL);
}


printf("%ld %ld %ld %ld\n", sum, odd, min, max);

  return (EXIT_SUCCESS);
}

