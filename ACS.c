/*
    CSC 360
    Assignment 2
    Zexi Luo
    V00929142
*/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <pthread.h>
#include <sys/time.h>


/*------------- Typdef --------------*/
typedef struct Customer{
    int id;
    int class_type; // 0 = economy class and 1 = business class
    int arrive_time;
    int service_time;
} customer;


/*------------- Constants and global variables----------*/

#define NQUEUES 2
#define NCLERKS 5
#define MAX_INPUT 256
#define MAX_LINE_LEN 1024

// initial time
double init_time;

//overall waiting time for all customers
double overall_waiting_time[2];

//store the real-time queue length information
int queue_length[NQUEUES];

//signal
int clerk_signal = 0;
int queue_signal = -1;

//mutex for two queues and five clerks
pthread_mutex_t queue_mutex[NQUEUES];
pthread_mutex_t clerk_mutex;
// conditional variables for two queues and five clerks
pthread_cond_t queue_convar[NQUEUES]; //change to class
pthread_cond_t clerk_convar[NCLERKS];


/*------------- Function call --------------*/
// clerk thread
void *customer_entry(void *);
// customer thread
void *clerk_entry(void *);


/*
    Queue:
    - create Node
    - enqueue
    - dequeue
    - get size of the queue
    - Display the queue

*/

/*------------- Typdef --------------*/
typedef struct QueueNode{
    struct Customer customer;
    struct QueueNode *next;
}QueueNode;

/*------------- global variable----------*/
QueueNode *class_type_queue[NQUEUES];


//Create a queue
QueueNode* createQueue(struct Customer Info){

    QueueNode* queue = malloc(sizeof(QueueNode));
    queue->customer = Info;
    queue->next = NULL;
    return queue;
}

//insert customers into queue
void enqueue(QueueNode ** queue, struct Customer Info){

    QueueNode *curr = *queue;

    while(curr->next != NULL){
        curr = curr->next;
    }
    QueueNode *temp = createQueue(Info);
    curr->next = temp;
}

// get the size of the queue
int get_size(QueueNode *queue){
    QueueNode *curr = queue;
    int count = 0;
    while(curr!=NULL){
        count++;
        curr = curr->next;

    }
    return count;

}

// display the queue
void DisplayQueue(QueueNode **queue){
    QueueNode *curr = *queue;
    if(get_size(curr) == 0){
        printf("Erorr: empty queue!\n");
    }
    while(curr != NULL){
        printf("id(%d)\n", curr->customer.id);
        curr = curr->next;
    }
}


// delete the used node
void dequeue(QueueNode ** queue){
    QueueNode *curr = *queue;
    if(get_size(curr) == 0){
        printf("Erorr: empty queue!\n");
        exit(0);
    }
    QueueNode  *node = curr;
    *queue = curr->next;
    free(node);
}


/*
    get the current time
*/
double get_curr_time(){
    struct timeval current_time;
    double cur_s;
    gettimeofday(&current_time,NULL);
    cur_s = (current_time.tv_sec+ (double)current_time.tv_usec/1000000);
    return cur_s - init_time;
}


/*
    main function:
    - read file
    - create clerk threads
    - create customer threads
    - calculate average waiting time for both classes
*/
int main(int argc, char *argv[]){

    //calculate inti_time
    struct timeval init_tv;
    gettimeofday(&init_tv, NULL);
    init_time = (init_tv.tv_sec + (double) init_tv.tv_usec / 1000000);

    //Read customer information from txt file and store them into queue
    FILE *file = fopen(argv[1], "r");
    char fileContents[MAX_INPUT][MAX_INPUT];
    char c;
    int i;

    // store customers informations into array
    int id;
    int class_type; // 0 = economy class and 1 = business class
    int arrive_time;
    int service_time;

    if(file == NULL){
        fprintf(stderr, "Error: unable to open %s\n", argv[1]);
        exit(1);
    }

    while(fscanf(file,"%[^\n]%c",fileContents[i],&c)!=EOF){
		i++;
	}

    int NCustomers = atoi(fileContents[0]); // numbers of consumers
    struct Customer customer_queue[NCustomers];


    for(int j=1; j<=NCustomers; j++){
        //check for valid customer informations
        if((id = atoi(strtok(fileContents[j], ":")))>0){
            customer_queue[j-1].id = id;
        }else{
            printf("Error: invalid customer id\n");
            return -1;
        }
        if((class_type = atoi(strtok(NULL, ",")))>=0){
            customer_queue[j-1].class_type = class_type;

        }else{
            printf("Error: invalid customer class type\n");
            return -1;
        }
        if((arrive_time = atoi(strtok(NULL, ",")))>0){
            customer_queue[j-1].arrive_time = arrive_time;
        }else{
            printf("Error: invalid customer arrive time\n");
            return -1;
        }
        if((service_time = atoi(strtok(NULL, ",")))> 0){
            customer_queue[j-1].service_time = service_time;

        }else{
            printf("Error: invalid customer service time\n");
            return -1;
        }

    }

    //number of customers in each class
    int num_b = 0;
    int num_e = 0;



    //create clerk thread
    int clerk_id[NCLERKS];
    pthread_t clerk_thread[NCLERKS];
    for (i = 0; i<NCLERKS; i++){
        clerk_id[i] = i;
        pthread_create(&clerk_thread[i], NULL, clerk_entry, (void *)&clerk_id[i]);
    }

    //create customer thread
    pthread_t customer_thread[NCustomers];
    for( int i =0 ; i< NCustomers; i++){
        pthread_create(&customer_thread[i], NULL, customer_entry, (void *)&customer_queue[i]);

        if(customer_queue[i].class_type == 0){
            num_e++;
        }else if(customer_queue[i].class_type == 1){
            num_b++;
        }else{
            printf("Error: invalid class type\n");
            return -1;
        }
    }
    //sum of all the customers
    double all_customers = num_b+num_e;

    for(int i = 0; i<NCustomers; i++){

        if (pthread_join(customer_thread[i], NULL) != 0)
		{
			printf("Error: failed to join pthread.\n");
			exit(1);
		}
    }

	// calculate the average waiting time of all customers in the system
	double all_waiting_time = overall_waiting_time[0] + overall_waiting_time[1];
	fprintf(stdout, "The average waiting time for all customers in the system is: %.2f seconds. \n",
			all_waiting_time / all_customers);
	fprintf(stdout, "The average waiting time for all business-class customers is: %.2f seconds. \n",
			overall_waiting_time[1] / num_b);
	fprintf(stdout, "The average waiting time for all economy-class customers is: %.2f seconds. \n",
			overall_waiting_time[0] / num_e);

    fclose(file);
    return 0;
}

/*
    clerk thread
*/
void *clerk_entry(void *param){
	int clerk_id = *(int *)param+1;
	int class;

    while(1){
        // lock queue
        while(queue_signal != -1){
            for(int i=0;i<NQUEUES;i++){
                pthread_mutex_lock(&queue_mutex[i]);
            }
            pthread_cond_signal(&queue_convar[queue_signal]);
        }
        // lock clerk
        if(pthread_mutex_lock(&clerk_mutex) != 0 ){
          fprintf(stdout, "Error: unable to lock the mutex!\n");
			exit(1);
        }
        // assign customer with a clerk by class: economy or business
        while(1){
            if(queue_length[1]== 0 && queue_length[0]==0){
                continue;
            }else if(queue_length[1] > 0 && clerk_signal == 0){
                pthread_mutex_lock(&queue_mutex[1]);
                class = 1;
                clerk_signal = clerk_id;
                break;
            }else if(queue_length[0] > 0 && clerk_signal == 0 ){
                pthread_mutex_lock(&queue_mutex[0]);
                class = 0;
                clerk_signal  = clerk_id;
                break;
            }

        }
        //clerk unlock
        pthread_mutex_unlock(&clerk_mutex);
        //signal and wait
        pthread_cond_signal(&queue_convar[class]);
        pthread_cond_wait(&clerk_convar[clerk_id-1], &queue_mutex[class]);
        // class unlock
		pthread_mutex_unlock(&queue_mutex[class]);

    }
	pthread_exit(NULL);
	return NULL;
}

// For customer threads
void *customer_entry(void *param){

    struct Customer Info = *(struct Customer*)param;
    // use usleep() to simulate customer arrival time
    usleep(Info.arrive_time *100000);


    int user_id = Info.id;
    fprintf(stdout,"A customer arrives: customer ID %2d. \n",user_id);

    // queue lock
    while(queue_signal != -1){
        for(int i=0;i<NQUEUES;i++){
            pthread_mutex_lock(&queue_mutex[i]);
        }
        pthread_cond_signal(&queue_convar[queue_signal]);
    }

    /* -------------------- Critial section --------------------*/
    //Enqueue, get into either business queue or economy queue
    int class = Info.class_type;
    if (pthread_mutex_lock(&queue_mutex[class]) != 0){
		fprintf(stdout, "Error: unable to lock the mutex!\n");
		exit(1);
	}
    // get queue enter time
    float queue_enter_time = get_curr_time();

    // insert customer into queue
    if(class_type_queue[class]== NULL){
        class_type_queue[class] = createQueue(Info);
    }
    else{
        enqueue(&class_type_queue[class], Info);
        queue_length[class] = get_size(class_type_queue[class]);
    }
    fprintf(stdout, "A customer enters a queue: the queue ID %1d, and length of the queue %2d. \n",
	 		class, get_size(class_type_queue[class]));

    // length of the queue
    queue_length[class] = get_size(class_type_queue[class]);

    // customer waiting for the clerk to signal
    do{
        pthread_cond_wait(&queue_convar[class], &queue_mutex[class]);
    }while(user_id != class_type_queue[class]->customer.id || class != class_type_queue[class]->customer.class_type || clerk_signal == 0);

    // delete the customer finished the service
    dequeue(&class_type_queue[class]);
    // get the length of the class
    queue_length[class] = get_size(class_type_queue[class]);

    queue_signal = -1;
    pthread_mutex_unlock(&queue_mutex[class]);

    //read clerk signal
    int clerk_woke_me_up = clerk_signal;
    clerk_signal = 0;


    float clerk_enter_time = get_curr_time();
	overall_waiting_time[class] += clerk_enter_time - queue_enter_time;
	fprintf(stdout, "A clerk starts serving a customer: start time %.2f, the customer ID %2d, the clerk ID %1d. \n",
			clerk_enter_time, user_id, clerk_woke_me_up);

	usleep(Info.service_time * 100000);

	float end_time = get_curr_time();
	fprintf(stdout, "A clerk finishes serving a customer: end time %.2f, the customer ID %2d, the clerk ID %1d. \n",
			end_time, user_id, clerk_woke_me_up);


    pthread_cond_signal(&clerk_convar[clerk_woke_me_up-1]);

    pthread_exit(NULL);
    return NULL;

}
