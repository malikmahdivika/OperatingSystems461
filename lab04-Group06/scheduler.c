#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <assert.h>
#include <string.h>
#include <limits.h>
#include <stdbool.h>
#include <time.h>

#define min(a,b) (((a)<(b))?(a):(b))
#define max(a,b) (((a) > (b))?(a):(b))
#define numMax 64
// total jobs
int numofjobs = 0;

//average times 
int totalTaT = 0;
int totalWait = 0;
int totalRes = 0;

struct job {
    // job id is ordered by the arrival; jobs arrived first have smaller job id, always increment by 1
    int id;
    int arrival; // arrival time; safely assume the time unit has the minimal increment of 1
    int length;
    int tickets; // number of tickets for lottery scheduling
    // TODO: add any other metadata you need to track here
    int responseTime;
    int turnaroundTime;
    int waitTime;
    int completionTime;
    int startTime;

    int runTime; //for printing purposes. incriment everytime you decrement length
    bool visited; //to check if it has been visited.
    int originalLength;

    // ------ //
    int remainingTime;

    // ------ //
    int time_waited;
    int turnaround_time;
    int last_executed_time;
    int response_time;  // each job stores metadata about their individual metrics
    int responded_to;

    struct job *next;
};


//Pirority Queue from https://www.geeksforgeeks.org/c/c-program-to-implement-priority-queue/
typedef struct {
    struct job* items[numMax];
    int size;
} PriorityQueue;

void swap(struct job ** a, struct job ** b){
    struct job *tmp = *a;
    *a = *b;
    *b = tmp;
}

void heapifyUp(PriorityQueue* pq, int index)
{
    if (index
        && pq->items[(index - 1) / 2]->length > pq->items[index]->length) {
        swap(&pq->items[(index - 1) / 2],
             &pq->items[index]);
        heapifyUp(pq, (index - 1) / 2);
    }
}

void enqueue(PriorityQueue* pq, struct job * value)
{
    if (pq->size == numMax) {
        printf("Priority queue is full\n");
        return;
    }

    pq->items[pq->size++] = value;
    heapifyUp(pq, pq->size - 1);
}

void heapifyDown(PriorityQueue* pq, int index)
{
    int smallest = index;
    int left = 2 * index + 1;
    int right = 2 * index + 2;

    if (left < pq->size
        && pq->items[left]->length < pq->items[smallest]->length)
        smallest = left;

    if (right < pq->size
        && pq->items[right]->length < pq->items[smallest]->length)
        smallest = right;

    if (smallest != index) {
        swap(&pq->items[index], &pq->items[smallest]);
        heapifyDown(pq, smallest);
    }
}

struct job* dequeue(PriorityQueue* pq)
{
    if (!pq->size) {
        printf("Priority queue is empty\n");
        return NULL;
    }

    struct job * item = pq->items[0];
    pq->items[0] = pq->items[--pq->size];
    heapifyDown(pq, 0);
    return item;
}

int peek(PriorityQueue* pq)
{
    if (!pq->size) {
        return INT_MIN;
    }
    return pq->items[0]->id;
}

struct job* peekfull(PriorityQueue * pq){
    if (!pq->size) {
        return NULL;
    }
    return pq->items[0];
}



// the workload list
struct job *head = NULL;


void append_to(struct job **head_pointer, int arrival, int length, int tickets){
    struct job* new_job = (struct job*) malloc(sizeof(struct job));
    new_job->arrival = arrival;
    new_job->length = length;
    new_job->tickets = tickets;
    new_job->id = numofjobs++;
    new_job->next = NULL;
    
    //-----------------------------------
    new_job->visited = false;
    new_job->runTime = 0;
    new_job->originalLength = new_job->length;

    //----------------------------------
    new_job->remainingTime = length;
    new_job->responseTime = -1;
    new_job->turnaroundTime = 0;
    new_job->waitTime = 0;
    new_job->completionTime = 0;
    new_job->startTime = 0;

    //----------------------------------
    new_job->responded_to = 0;          // set "responded to" metadata to "false" (no bool datatype?)
    new_job->time_waited = 0;           // initialize time_waited with 0 value.
    new_job->last_executed_time = 0;    // job last executed on

    if(*head_pointer == NULL){
        *head_pointer = new_job;
        return;
    }else{
        struct  job * curr = *head_pointer;
        while(curr->next != NULL){
            curr = curr->next;
        }
        curr->next = new_job;
    }


    return;
}


void read_job_config(const char* filename)
{
    FILE *fp;
    char *line = NULL;
    size_t len = 0;
    ssize_t read;
    int tickets  = 0;

    char* delim = ",";
    char *arrival = NULL;
    char *length = NULL;

    // TODO, error checking
    fp = fopen(filename, "r");
    if (fp == NULL)
        exit(EXIT_FAILURE);

    //------------------- Test if file is empty -------------------//
    fseek(fp, 0, SEEK_END);
    size_t size = ftell(fp);

    if(0 == size){
        //file is empty, exit
        exit(EXIT_FAILURE);
    }
    rewind(fp);
    //-------------------------------------------------------------//

    while ((read = getline(&line, &len, fp)) != -1)
    {
        if( line[read-1] == '\n' )
            line[read-1] =0;
        arrival = strtok(line, delim);
        length = strtok(NULL, delim);
        tickets += 100;
        // printf("successful addition to LinkedList\n");
        append_to(&head, atoi(arrival), atoi(length), tickets);
    }

    fclose(fp);
    if (line) free(line);
}


// almost useless I could just write them out.
int responseT(int Ts, int Ta){return Ts - Ta;}
int waitT(int Tt, int Ts){return Tt - Ts;}
int TurnaroundTime(int Tc, int Ta){return Tc - Ta;}

void policy_SJF()
{
    printf("Execution trace with SJF:\n");
    
    int currentTime = head->arrival; //since its sorted by arrival already.
    int jobsCompleted = 0;
    PriorityQueue jobQ = {0};
    struct job* currJob = head;
    while(jobsCompleted < numofjobs){
        
        //if Jobs came before the current time add them to the queue?
        while(currJob != NULL && currJob->arrival <= currentTime){
            enqueue(&jobQ, currJob);
            currJob = currJob->next;
        }

        if(jobQ.size > 0){
            struct job *  j = dequeue(&jobQ);

            j->startTime = currentTime;

            printf("t=%d: [Job %d] arrived at [%d], ran for: [%d]\n", currentTime, j->id, j->arrival, j->length);
            
            j->completionTime = currentTime + j->length;
            j->waitTime = j->startTime - j->arrival;
            j->turnaroundTime = j->completionTime - j->arrival;
            j->responseTime = j->startTime - j->arrival;
            currentTime += j->length;
            jobsCompleted++;

            //add to the totals :
            totalTaT += j->turnaroundTime;
            totalWait += j->waitTime;
            totalRes += j->responseTime;

        }else{
            currentTime +=1;
        }
    }


    printf("End of execution with SJF.\n");

}

void policy_STCF()
{
    // Almost its so close
    printf("Execution trace with STCF:\n");

    int currentTime = head->arrival; //since its sorted by arrival already.
    int jobsCompleted = 0;
    PriorityQueue jobQ = {0};
    struct job* currJob = head;
    struct job* running = NULL;
    int runtimePrev = 0; // to calculate t=[%d]
    //excecution:
    while(jobsCompleted < numofjobs){
        //if Jobs came before the current time add them to the queue?
        while(currJob != NULL && currJob->arrival <= currentTime){
            enqueue(&jobQ, currJob);
            currJob = currJob->next;
        }

        if(jobQ.size > 0){
            int timeElapsed = currentTime - runtimePrev;
            struct job * nextJob = peekfull(&jobQ);
            if(running != nextJob){
                if(running != NULL){
                    printf("t=%d: [Job %d] arrived at [%d], ran for: [%d]\n",timeElapsed,running->id, running->arrival, running->runTime);                }
                running = nextJob;
                running->runTime = 0;

                if(!running->visited){
                    running->visited = true;
                    running->startTime = currentTime;
                }
            }

            //run for one time slice
            running->length -= 1;
            running->runTime += 1;
            currentTime += 1;
            runtimePrev = running->runTime;
            if(running->length == 0){
                dequeue(&jobQ);
                running->completionTime = currentTime;
                printf("t=%d: [Job %d] arrived at [%d], ran for: [%d]\n",timeElapsed,running->id, running->arrival, running->runTime);
                running->turnaroundTime = running->completionTime - running->arrival;
                running->waitTime = running->turnaroundTime - running->originalLength;
                running->responseTime = running->startTime - running->arrival;
                jobsCompleted++;

                //calculation purposes
                totalRes += running->responseTime;
                totalTaT += running->turnaroundTime;
                totalWait += running->waitTime;

                //clear running because its done
                running = NULL;
            }
        }else{
            currentTime += 1;
        }

    }
    printf("End of execution with STCF.\n");
}

void policy_RR(int slice)
{
    printf("Execution trace with RR:\n");
    // TODO: implement RR policy
    int currentTime = 0;
    struct job *curr = head;
    int numOfFinishedJobs = 0;    

    while (numOfFinishedJobs < numofjobs) {
        // these blocks maintain overall execution sanity
        
        if (curr == NULL) { // check if at end of list
            // move back to top of list
            curr = head;
            continue;
        }
        
        if (curr->length == 0) {  // job has completed already, run other jobs
            curr = curr->next;
            continue;
        }

        // check if there are jobs which can currently run
        if (currentTime < curr->arrival) {
            struct job *ready_check = head;
            int anyReady = 0;
            while (ready_check != NULL) {
                // there is a ready job (that curr is not on)
                if (ready_check->arrival <= currentTime && ready_check->length > 0) {
                    anyReady = 1;
                    break;
                }
                ready_check = ready_check->next;
            }
        
            // if none are ready, jump execution time to next arrival
            if (!anyReady) {
                struct job *temp = head;
                int nextArr = 0;
                while (temp != NULL) {
                    if (temp->length > 0 && temp->arrival > currentTime) {
                        if (nextArr == 0 || temp->arrival < nextArr) {
                            nextArr = temp->arrival;
                        }
                    }
                    temp = temp->next;
                }
                // ff to next arrival
                if (nextArr != 0) currentTime = nextArr;
            }
            curr = head;    //restart loop
            continue;
        }

        // these blocks are responsible for job-specific execution
        if (curr->length - slice <= 0) {
            if (curr->responded_to == 0) {   //first time execution, set response time metadata
                curr->responded_to = 1;
                curr->response_time = currentTime - curr->arrival;
                curr->time_waited = curr->response_time;         // wait includes time before first response
            } else if (curr->responded_to == 1) {
                // append time waited with time between current time and time of last execution.
                curr->time_waited += currentTime - curr->last_executed_time;
            }
            
            // job will complete after this run
            printf("t=%d: [Job %d] arrived at [%d], ran for: [%d]\n", currentTime, curr->id, curr->arrival, curr->length);
            numOfFinishedJobs++;

            currentTime += curr->length;
            curr->turnaround_time = currentTime - curr->arrival;
            curr->last_executed_time = currentTime + curr->length;  // job last ran at this time
            curr->length = 0;
            
            curr = curr->next;
            continue;
        } else {    // normal operation, job won't complete after this round.
            if (curr->responded_to == 0) {   //first time execution, set response time metadata
                curr->responded_to = 1;
                curr->response_time = currentTime - curr->arrival;
                curr->last_executed_time = currentTime + slice;  // job hasn't run to have a previous execution time yet (wait = response)
                curr->time_waited = curr->response_time;         // wait includes time before first response
            } else if (curr->responded_to == 1 && curr->length > 0) {
                curr->time_waited += currentTime - curr->last_executed_time;
                curr->last_executed_time = slice + currentTime;  // job will stop execution here
            }
            // if (curr->responded_to == 1) {curr->last_executed_time = currentTime + curr->length;}  // job last ran at this time
            printf("t=%d: [Job %d] arrived at [%d], ran for: [%d]\n", currentTime, curr->id, curr->arrival, slice);

            curr->length -= slice;
            currentTime += slice;
            
            curr = curr->next;
            continue;
        }
        curr =  curr->next;
    }

    printf("End of execution with RR.\n");
}

void policy_LT(int slice)
{
    printf("Execution trace with LT:\n");
    totalTaT = totalWait = totalRes = 0;
    srand(42);
    for (struct job *p = head; p; p = p->next) {
        p->remainingTime = p->length;
        p->responseTime  = -1;
    }

    int currentTime = 0;

    while (1) {
        int total_tickets = 0;
        int anyRemaining = 0;
        int anyRunnable  = 0;

        struct job *curr = head;
        while (curr != NULL) {
            if (curr->remainingTime > 0) {
                anyRemaining = 1;
                if (curr->arrival <= currentTime) {
                    anyRunnable = 1;
                    total_tickets += curr->tickets;
                }
            }
            curr = curr->next;
        }

        if (!anyRemaining) break;

        if (!anyRunnable) {
            int nextArrival = INT_MAX;
            struct job *p = head;
            while (p != NULL) {
                if (p->remainingTime > 0 && p->arrival > currentTime &&
                    p->arrival < nextArrival) {
                    nextArrival = p->arrival;
                }
                p = p->next;
            }
            currentTime = nextArrival;   // jump to first future arrival
            continue;
        }

        // --- run lottery as you already do ---
        int win = rand() % total_tickets;
        int count = 0;
        curr = head;
        while (curr != NULL) {
            if (curr->arrival <= currentTime && curr->remainingTime > 0) {
                count += curr->tickets;
                if (count > win) break;
            }
            curr = curr->next;
        }

        int run = (curr->remainingTime < slice) ? curr->remainingTime : slice;

        if (curr->responseTime == -1)
            curr->responseTime = currentTime - curr->arrival;

        printf("t=%d: [Job %d] arrived at [%d], ran for: [%d]\n",
            currentTime, curr->id, curr->arrival, run);

        currentTime += run;
        curr->remainingTime -= run;

        if (curr->remainingTime == 0) {
            curr->completionTime = currentTime;
            curr->turnaroundTime = curr->completionTime - curr->arrival;
            curr->waitTime = curr->turnaroundTime - curr->length;
            totalTaT += curr->turnaroundTime;
            totalWait += curr->waitTime;
            totalRes  += curr->responseTime;
        }
    }

    printf("End of execution with LT.\n");
}

void policy_FIFO(){
    printf("Execution trace with FIFO:\n");
    struct job * curr = head;
    int currentTime = 0;
    while(curr != NULL){
        if(currentTime < curr->arrival){
            currentTime = curr->arrival;
        }
        curr->startTime = currentTime;
        
        printf("t=%d: [Job %d] arrived at [%d], ran for: [%d]\n", currentTime, curr->id, curr->arrival, curr->length);
        
        currentTime += curr->length;
        curr->completionTime = currentTime;
        curr->turnaroundTime = TurnaroundTime(curr->completionTime, curr->arrival); 
        curr->waitTime = waitT(curr->startTime, curr->arrival);      
        curr->responseTime = responseT(curr->startTime, curr->arrival);  
        
        //add to total:
        totalTaT += curr->turnaroundTime;
        totalWait += curr->waitTime;
        totalRes += curr->responseTime;

        //test statments//
        // printf(" CompletionTime = %d ", curr->completionTime);
        // printf(" TAT = %d", curr->turnaroundTime);
        // printf(" waitTime = %d\n", curr->waitTime);
        curr = curr->next;

        //calculate the information here:

    }

    printf("End of execution with FIFO.\n");

}


int main(int argc, char **argv){

    static char usage[] = "usage: %s analysis policy slice trace\n";

    int analysis;
    char *pname;
    char *tname;
    int slice;


    if (argc < 5)
    {
        fprintf(stderr, "missing variables\n");
        fprintf(stderr, usage, argv[0]);
		exit(1);
    }

    // if 0, we don't analysis the performance
    analysis = atoi(argv[1]);

    // policy name
    pname = argv[2];

    // time slice, only valid for RR
    slice = atoi(argv[3]);

    // workload trace
    tname = argv[4];

    read_job_config(tname);

    if (strcmp(pname, "FIFO") == 0){
        policy_FIFO();
        if (analysis == 1){

            float avgTaT = (float)totalTaT / (float)numofjobs;
            float avgWait = (float)totalWait / (float)numofjobs;
            float avgRes = (float)totalRes/ (float)numofjobs;
            struct job * curr = head;


            printf("Begin analyzing FIFO:\n");
            while(curr != NULL){
                printf("Job %d -- Response time: %d  Turnaround: %d  Wait: %d\n", curr->id, curr->responseTime, curr->turnaroundTime, curr->waitTime);

                curr = curr->next;
            }
            printf("Average -- Response: %.2f  Turnaround %.2f  Wait %.2f\n", avgRes, avgTaT, avgWait);
            printf("End analyzing FIFO.\n");
        }
    }
    else if (strcmp(pname, "SJF") == 0)
    {
        policy_SJF();
        if(analysis == 1){
            
            float avgTaT = (float)totalTaT / (float)numofjobs;
            float avgWait = (float)totalWait / (float)numofjobs;
            float avgRes = (float)totalRes/ (float)numofjobs;
            struct job * curr = head;


            printf("Begin analyzing SJF:\n");
            while(curr != NULL){
                printf("Job %d -- Response time: %d  Turnaround: %d  Wait: %d\n", curr->id, curr->responseTime, curr->turnaroundTime, curr->waitTime);

                curr = curr->next;
            }
            printf("Average -- Response: %.2f  Turnaround %.2f  Wait %.2f\n", avgRes, avgTaT, avgWait);
            printf("End analyzing SJF.\n");
        }
    }
    else if (strcmp(pname, "STCF") == 0)
    {
        policy_STCF();
        if(analysis == 1){
            
            float avgTaT = (float)totalTaT / (float)numofjobs;
            float avgWait = (float)totalWait / (float)numofjobs;
            float avgRes = (float)totalRes/ (float)numofjobs;
            struct job * curr = head;


            printf("Begin analyzing STCF:\n");
            while(curr != NULL){
                printf("Job %d -- Response time: %d  Turnaround: %d  Wait: %d\n", curr->id, curr->responseTime, curr->turnaroundTime, curr->waitTime);

                curr = curr->next;
            }
            printf("Average -- Response: %.2f  Turnaround %.2f  Wait %.2f\n", avgRes, avgTaT, avgWait);
            printf("End analyzing STCF.\n");
        }
    }
    else if (strcmp(pname, "RR") == 0)
    {
        policy_RR(slice);
        if (analysis == 1) {
            // TODO: perform analysis
            int tot_resp = 0;
            int tot_turn = 0;
            int tot_wait = 0;   // variables for average metric

            struct job * curr = head;
            printf("Begin analyzing RR:\n");

            while (curr != NULL) {
                printf("Job %d -- Response time: %d  Turnaround: %d  Wait: %d\n", 
                    curr->id, curr->response_time, curr->turnaround_time, curr->time_waited);
                tot_resp += curr->response_time;
                tot_turn += curr->turnaround_time;
                tot_wait += curr->time_waited;
                curr = curr->next;
            }

            printf("Average -- Response: %.2lf  Turnaround %.2lf  Wait %.2lf\n",  tot_resp/(double)numofjobs, tot_turn/(double)numofjobs, tot_wait/(double)numofjobs);
            printf("End analyzing RR.\n");
        }

    }
    else if (strcmp(pname, "LT") == 0)
    {
        policy_LT(slice);
        if(analysis == 1){
            
            float avgTaT = (float)totalTaT / (float)numofjobs;
            float avgWait = (float)totalWait / (float)numofjobs;
            float avgRes = (float)totalRes/ (float)numofjobs;
            struct job * curr = head;


            printf("Begin analyzing LT:\n");
            while(curr != NULL){
                printf("Job %d -- Response time: %d  Turnaround: %d  Wait: %d\n",
                    curr->id, curr->responseTime, curr->turnaroundTime, curr->waitTime);
                curr = curr->next;
            }
            printf("Average -- Response: %.2f  Turnaround %.2f  Wait %.2f\n",
                avgRes, avgTaT, avgWait);
            printf("End analyzing LT.\n");;
        }
    }

    //free data
    while(head != NULL){
        struct job * temp = head;
        head = head->next;
        free(temp);
    }

	exit(0);
}