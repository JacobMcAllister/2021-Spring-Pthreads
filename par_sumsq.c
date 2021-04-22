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
bool done = false;


/*  STEP 2 - Add a singly linked list for a queue of task
    These task will be collected from the input file and given out to our works */
//  Originally I had issues with a simple singly linked list not working
//  I assumed race conditions and I was being left in an infinite loop
//  I then adjusted to using Queue list that would be shared before the threads
//  The threads would then compete on this thread with new items being enqueued to the back
//  and as the work was being done dequeued from the front.
//  Competeing for work ->>>>>>  Front --- Back ->>>>> new items being added.



/* 2.a - Create the structures for our queue */

//  A basic data and pointer struct for a linked list
typedef struct queueNode{
	long data;
	struct queueNode* next;
}queueNode;

//  The queue struct
typedef struct Queue{
	queueNode* head;
}Queue;

/*  2.b - Add prototypes for creating a queue */

//  A constructor for the queue.
Queue* createQueue(void);

//  A deconstructor for the queue.
void deleteQueue(Queue* list);

//  Enqueue logic for adding task to the queue
bool enqueueWork(Queue* list, long inData);

//  Dequeue logic for removing task leading task
long dequeueWork(Queue* list);

//  For this we will make a global list, not my favorite method.

//Queue* workingQueue;// = createQueue();

/*  2.b will continue after the main */

/*  STEP 3 - Figure out how many MUTEXs we need 
    Lets think about what this program does... it takes in task
    Then it updates a number.
    So I will start with two mutexs one for updating the aggrerates
    and another for the task.  (As mentioned in hints) */

//  A lot of the resources I used on the how pthreads work was in the text
//  , supplemental lecture.
//  Some other resources I used can  be found at
//  cs.kent.edu/~ruttan/sysprog/lectures/multi-thread/pthread_mutex_init.html
//  "                                                "/pthread_cond_init.html
//  Along with many youtube videos.


//  Here are out two locks
pthread_mutex_t lockAgg = PTHREAD_MUTEX_INITIALIZER;
pthread_mutex_t lockWork = PTHREAD_MUTEX_INITIALIZER;

//  The conditional for our workers (suggested in the hints)
pthread_cond_t newWorkerCondition = PTHREAD_COND_INITIALIZER;

// Going to main and playing now....


// Step 4.3 - start_routine prototype

void* start_routine(void* routine);


//  Add a volatile bool for masterthread to broadcast when its done time!
//  This will be used in our start_routine!

volatile bool broadcast = false;



// function prototypes
void calculate_square(long number);

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
/*
  // check and parse command line options
  if (argc != 2) {
    printf("Usage: sumsq <infile>\n");
    exit(EXIT_FAILURE);
  }
  char *fn = argv[1];
  
  // load numbers and add them to the queue
  FILE* fin = fopen(fn, "r");
*/


//  STEP 4 - Collect task to add to queue
//  Alter above code to allow for reading of input and creating queue

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

	// Make our queue
	// originally tried global Queue*, changed to volitale 
	// after taking hint and reading what it does with plenty of trial and error
	// I make it volatile here as C does not allow us to make volatile objects there.
	volatile Queue* workingQueue;	
	workingQueue = createQueue();
	// 4.b we make the workers by making an array of pthreads!
	pthread_t worker[numWorkers];
	// pthread_create usuage pthread_create(pthread_t *thread, const pthread_attr_t *attr, void*(*start_routine) (void *), void *arg);
	// step 4.3 will be create the start_routine
	for(long i = 0; i<numWorkers; ++i){
		pthread_create(&worker[i], NULL, start_routine, (void*) workingQueue);
	//	pthread_create(&worker[i], NULL, start_routine, NULL);
	}

//printf("HERE!");
/*

  char action;
  long num;

  while (fscanf(fin, "%c %ld\n", &action, &num) == 2) {
    if (action == 'p') {            // process, do some work
      calculate_square(num);
    } else if (action == 'w') {     // wait, nothing new happening
      sleep(num);
    } else {
      printf("ERROR: Unrecognized action: '%c'\n", action);
      exit(EXIT_FAILURE);
    }
  }
  fclose(fin);
*/

//  STEP 4.D - Add the task to the queue
//  Edit given code to make it happen

	char action;
	long num;

	while(fscanf(fin, "%c %ld\n", &action, &num) == 2){

		if(action == 'p'){
			//  printf(" %ld ", num);
			//  Got process, so lets work
			//  First grab our mutex
			pthread_mutex_lock(&lockWork);
			
			//  Now we put in on the queue
			enqueueWork((Queue*)workingQueue, num);
			
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
while(workingQueue->head){
  //  Buzz Buzz
}

  // print results
//  printf("%ld %ld %ld %ld\n", sum, odd, min, max);
  
  // clean up and return
  //  Alright now, lets free everything and delete the queue

//  Lets singal that all the work is done
broadcast = true;

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

//deleteQueue((Queue*)workingQueue);

printf("%ld %ld %ld %ld\n", sum, odd, min, max);

  return (EXIT_SUCCESS);
}



//  Logic for createQueue()

Queue* createQueue(){
	Queue* newQueue = (Queue*)malloc(sizeof(struct Queue));
	
	newQueue->head = NULL;

return newQueue;
}

//  Logic for deleteQueue(Queue* list)

void deleteQueue(Queue* list){
	if(!list)
		return;
	while(list->head) dequeueWork(list);

	free(list);
}

//  Logic for dequeueWork(Queue* list)

long dequeueWork(Queue* list){
	queueNode* node = list->head;
	if(node)
		list->head = node->next;

	if(!node)
		return 0;
	
	long pointer = node->data;
	free(node);
return pointer;
}

//  Logic for enqueueWork(Queue* list, long inData)
//  Here we are adding to the back of the list

bool enqueueWork(Queue* list, long inData){
	queueNode* node = (queueNode*)malloc(sizeof(struct queueNode));
	if(!node)
		return false;
	node->data = inData;
	node->next = NULL;

	if(list->head == NULL){
		list->head = node;
	}
	else{
		queueNode* nodeB = list->head;
		while(nodeB->next)
			nodeB = nodeB->next;
		nodeB->next = node;
	}
return true;
}

//  Logic for start_routine

void* start_routine(void* routine){

Queue* tempQueue = (Queue*)routine;

//  How are we doing to handle our routine?
//  Create a while loop that will need to wait until it is told by the master thread that its time to work.
//  (Took some digging to realize I needed to use volitile bool for my while loop)
//  We will give it the lock for lockWork


while(broadcast == false){

	//  First we get our lock
	//  pthread_mutex_lock usuage pthread_mutex_lock(pthread_mutex_t *mutex)

	pthread_mutex_lock(&lockWork);

	//  This is going to be our block that waits on the master thread for new input!
	//  pthread_cond_wait usuage pthreaD_cond_wait(pthread_cond_t *cond, pthread_mutex_t *mutex)
	//  While we are NOT the master thread and we are NOT done we will wait.
	while(!tempQueue->head && broadcast == false)
		pthread_cond_wait(&newWorkerCondition, &lockWork);

	//  Now if the master thread broadcast true that we are done with all the work, free the mutex and break out of the while loop	
	if(broadcast){
		//  pthread_mutex_unlock usuage pthread_mutex_unlock(pthread_mutex_t *mutex)
		pthread_mutex_unlock(&lockWork);
		break;
	}

	//  If we are not done and not waiting, let us do some work!
	long number = dequeueWork(tempQueue);
	
	//  Free the mutex for other workers
	pthread_mutex_unlock(&lockWork);
	
	// Work
//printf(" in worker routine");
	calculate_square(number);	
	
}


return EXIT_SUCCESS;

}

