#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <assert.h>
#include <string.h>
#include <limits.h>
#include <time.h>

#define min(a,b) (((a)<(b))?(a):(b))

// total jobs
int numofjobs = 0;

struct job {
    // job id is ordered by the arrival; jobs arrived first have smaller job id, always increment by 1
    int id;
    int arrival; // arrival time; safely assume the time unit has the minimal increment of 1
    int length;
    int tickets; // number of tickets for lottery scheduling
    int time_waited;
    int turnaround_time;
    int response_time;  // each job stores metadata about their individual metrics
    // TODO: add any other metadata you need to track here
    struct job *next;
};

// the workload list
struct job *head = NULL;


void append_to(struct job **head_pointer, int arrival, int length, int tickets) {
    // TODO: create a new job and init it with proper data
    struct job* new_job = (struct job*) malloc(sizeof(struct job));
    new_job->arrival = arrival;
    new_job->length = length;
    new_job->tickets = tickets;
    new_job->id = numofjobs++;
    new_job->next = NULL;

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

    // TODO: if the file is empty, we should just exit with error
    if (fp == NULL) {
        perror("Failed to open file");
        exit(EXIT_FAILURE);
    }

    fseek(fp, 0, SEEK_END);
    size_t size = ftell(fp);

    if(0 == size){
        //file is empty, exit
        exit(EXIT_FAILURE);
    }
    rewind(fp);

    while ((read = getline(&line, &len, fp)) != -1)
    {
        if( line[read-1] == '\n' )
            line[read-1] =0;
        arrival = strtok(line, delim);
        length = strtok(NULL, delim);
        tickets += 100;

        append_to(&head, atoi(arrival), atoi(length), tickets);
    }

    fclose(fp);
    if (line) free(line);
}


void policy_SJF()
{
    printf("Execution trace with SJF:\n");

    // TODO: implement SJF policy

    printf("End of execution with SJF.\n");

}


void policy_STCF()
{
    printf("Execution trace with STCF:\n");

    // TODO: implement STCF policy

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
        if (curr != NULL && currentTime < curr->arrival) {  // if there is a time gap, jump to execution time.
            currentTime = curr->arrival;
        } else if (curr == NULL) {
            // move back to top of list
            curr = head;
            continue;
        }

        if (curr->length == 0) {
            curr = curr->next;
            continue;
        } else if (curr->length - slice <= 0) {
            // job will complete after this run
            printf("t=%d: [Job %d] arrived at [%d], ran for: [%d]\n", currentTime, curr->id, curr->arrival, curr->length);
            numOfFinishedJobs++;
            currentTime += curr->length;
            curr->length = 0;
            curr = curr->next;
            continue;
        } else {
            // normal operation, job won't complete after this round.
            curr->length -= slice;
            printf("t=%d: [Job %d] arrived at [%d], ran for: [%d]\n", currentTime, curr->id, curr->arrival, slice);
            currentTime += slice;
            curr = curr->next;
            continue;
        }
        
    }

    printf("End of execution with RR.\n");
}


void policy_LT(int slice)
{
    printf("Execution trace with LT:\n");

    // Leave this here, it will ensure the scheduling behavior remains deterministic
    srand(42);

    // In the following, you'll need to:
    // Figure out which active job to run first
    // Pick the job with the shortest remaining time
    // Considers jobs in order of arrival, so implicitly breaks ties by choosing the job with the lowest ID

    // To achieve consistency with the tests, you are encouraged to choose the winning ticket as follows:
    // int winning_ticket = rand() % total_tickets;
    // And pick the winning job using the linked list approach discussed in class, or equivalent

    printf("End of execution with LT.\n");

}


void policy_FIFO(){
    printf("Execution trace with FIFO:\n");

    // TODO: implement FIFO policy
    struct job * curr = head;
    int currentTime = 0;
    while(curr != NULL){
        if (currentTime < curr->arrival) {  // if there is a time gap, jump to execution time.
            currentTime = curr->arrival;
        }
        printf("t=%d: [Job %d] arrived at [%d], ran for: [%d]\n", currentTime, curr->id, curr->arrival, curr->length);
        curr->response_time = currentTime - curr->arrival;
        curr->time_waited = curr->response_time;    //non-preemptive, wait time = response time.

        currentTime += curr->length;    // job runs to completion.
        curr->turnaround_time = currentTime - curr->arrival;

        curr = curr->next;
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
            // TODO: perform analysis
            int tot_resp = 0;
            int tot_turn = 0;
            int tot_wait = 0;   // variables for average metric

            struct job * curr = head;
            // traverse linked list and acquire metrics
            printf("Begin analyzing FIFO:\n");
            while (curr != NULL) {
                printf("Job %d -- Response time: %d  Turnaround: %d  Wait: %d\n", curr->id, curr->response_time,
                        curr->turnaround_time, curr->time_waited);
                tot_resp += curr->response_time;
                tot_turn += curr->turnaround_time;
                tot_wait += curr->time_waited;

                curr = curr->next;
            }

            printf("Average -- Response: %.2lf  Turnaround %.2lf  Wait %.2lf\n",  tot_resp/(double)numofjobs, tot_turn/(double)numofjobs, tot_wait/(double)numofjobs);
            printf("End analyzing FIFO.\n");
        }
    }
    else if (strcmp(pname, "SJF") == 0)
    {
        // TODO
    }
    else if (strcmp(pname, "STCF") == 0)
    {
        // TODO
    }
    else if (strcmp(pname, "RR") == 0)
    {
        // TODO
        policy_RR(slice);
        if (analysis == 1) {
            // TODO: Analysis
        }
    }
    else if (strcmp(pname, "LT") == 0)
    {
        // TODO
    }

    // traverse linked list and free all dynamic items
    while(head != NULL){
        struct job * temp = head;
        head = head->next;
        free(temp);
    }

	exit(0);
}