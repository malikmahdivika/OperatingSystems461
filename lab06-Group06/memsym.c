#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include <string.h>
#include <sys/types.h>

#define TRUE 1
#define FALSE 0


// //gonna have 4 processes
// int currProcessID = 0;


int define_called = FALSE;
int *phys_memory;

typedef struct {
    int valid;
    int vpn;
    int pfn;
    int pid;
}TLB;

typedef struct{
    int valid;
    int pfn;

}page_table_entry;


//process queue
page_table_entry* totalProcesses[4];

int currPID = 0;

//tlb I think you need 8 entries and you make them like this?
TLB LookasideBuffer[8]; 
int tlbNext = 0;
// Output file
FILE* output_file;

// TLB replacement strategy (FIFO or LRU)
char* strategy;
unsigned int regs[4][2];  

int OFF_BITS = 0;
int PFN_BITS = 0;

char** tokenize_input(char* input) {
    char** tokens = NULL;
    char* token = strtok(input, " ");
    int num_tokens = 0;

    while (token != NULL) {
        num_tokens++;
        tokens = realloc(tokens, num_tokens * sizeof(char*));
        tokens[num_tokens - 1] = malloc(strlen(token) + 1);
        strcpy(tokens[num_tokens - 1], token);
        token = strtok(NULL, " ");
    }

    num_tokens++;
    tokens = realloc(tokens, num_tokens * sizeof(char*));
    tokens[num_tokens - 1] = NULL;

    return tokens;
}
int regIndex(char* r) {
    if (strcmp(r, "r1") == 0) return 0;
    if (strcmp(r, "r2") == 0) return 1;
    return -1;   // invalid
}

int isImmediate(char* s) {
    return s[0] == '#';
}

unsigned int immValue(char* s) {
    return atoi(s + 1);
}
// =========== PART 3 TLB TRANSLATION ===========
int translateVPN(int vpn, int *p_pfn, int print) {

    // TLB lookup
    for (int i = 0; i < 8; i++) {
        if (LookasideBuffer[i].valid &&
            LookasideBuffer[i].pid == currPID &&
            LookasideBuffer[i].vpn == vpn) {

            if (print)
                fprintf(output_file,
                        "Current PID: %d. Translating. Lookup for VPN %d hit in TLB entry %d. PFN is %d\n",
                        currPID, vpn, i, LookasideBuffer[i].pfn);

            *p_pfn = LookasideBuffer[i].pfn;
            return TRUE;
        }
    }

    // TLB miss
    if (print)
        fprintf(output_file,
                "Current PID: %d. Translating. Lookup for VPN %d caused a TLB miss\n",
                currPID, vpn);

    // Page table lookup
    if (!totalProcesses[currPID][vpn].valid) {
        if (print)
            fprintf(output_file,
                    "Current PID: %d. Translating. Translation for VPN %d not found in page table\n",
                    currPID, vpn);
        return FALSE;
    }

    int pfn = totalProcesses[currPID][vpn].pfn;

    // Insert into TLB under FIFO
    int entry = tlbNext;
    tlbNext = (tlbNext + 1) % 8;

    LookasideBuffer[entry].valid = TRUE;
    LookasideBuffer[entry].pid = currPID;
    LookasideBuffer[entry].vpn = vpn;
    LookasideBuffer[entry].pfn = pfn;

    if (print)
        fprintf(output_file,
                "Current PID: %d. Translating. Loaded VPN %d into TLB entry %d. PFN is %d\n",
                currPID, vpn, entry, pfn);

    *p_pfn = pfn;
    return TRUE;
}
// ==============================================
int main(int argc, char* argv[]) {
    const char usage[] = "Usage: memsym.out <strategy> <input trace> <output trace>\n";
    char* input_trace;
    char* output_trace;
    char buffer[1024];

    // Parse command line arguments
    if (argc != 4) {
        printf("%s", usage);
        return 1;
    }
    strategy = argv[1];
    input_trace = argv[2];
    output_trace = argv[3];

    // Open input and output files
    FILE* input_file = fopen(input_trace, "r");
    output_file = fopen(output_trace, "w");  

    while ( !feof(input_file) ) {
        // Read input file line by line
        char *rez = fgets(buffer, sizeof(buffer), input_file);
        if ( !rez ) {
            fprintf(stderr, "Reached end of trace. Exiting...\n");
            return -1;
        } else {
            // Remove endline character
            buffer[strlen(buffer) - 1] = '\0';
        }
        char** tokens = tokenize_input(buffer);

        // TODO: Implement your memory simulator
        //printf("Memory instantiation complete.");

        //check for comments
        if(tokens[0][0] == '%'){
            //just a comment, continue and leave it
            // Deallocate tokens
            for (int i = 0; tokens[i] != NULL; i++)
                free(tokens[i]);
            free(tokens);
            continue;
        }

        if(define_called && 0 == strcmp(tokens[0], "ctxswitch")){
            
             if(atoi(tokens[1]) < 0 ||  atoi(tokens[1]) > 3){
                fprintf(output_file, "Current PID: %d. Invalid context switch to process %d\n", currPID, atoi(tokens[1]));
                exit(1);
            }

            currPID = atoi(tokens[1]);
            fprintf(output_file, "Current PID: %d. Switched execution context to process: %d\n", currPID, currPID);
        }
        // define not called guard
        if (!define_called && 0 != strcmp(tokens[0], "define")) {        
            fprintf(output_file,"Current PID: %d. Error: attempt to execute instruction before define\n", currPID);
            printf("Current PID: %d. Error: attempt to execute instruction before define\n", currPID);
            exit(1);
        }
        if (define_called && 0 == strcmp(tokens[0], "define")) {    // define double call guard
            fprintf(output_file,"Current PID: %d. Error: multiple calls to define in the same trace\n", currPID);
            printf("Current PID: %d. Error: multiple calls to define in the same trace\n", currPID);
            exit(1);
        }


        if (!define_called && 0 == strcmp(tokens[0], "define")) {   // define instantiation
            define_called = TRUE;
            OFF_BITS = atoi(tokens[1]);  
            PFN_BITS = atoi(tokens[2]); 
            int OFF = OFF_BITS;
            int PFN = PFN_BITS;
            int size = 1 << (OFF + PFN);
            int num_pages = 1 << (PFN);
            phys_memory = malloc(size * sizeof(int));
            for (int i = 0; i < size; i++)
                phys_memory[i] = 0;

            //initialize processes
            for (int i = 0; i < 4; i++) {
                totalProcesses[i] = malloc(num_pages * sizeof(page_table_entry));
                for (int j = 0; j < num_pages; j++) {
                    totalProcesses[i][j].valid = FALSE;
                    totalProcesses[i][j].pfn = 0;
                }
            }   

            // //initialize tlb? 
            for (int i = 0; i < 8; i++) {
                LookasideBuffer[i].valid = FALSE;
                LookasideBuffer[i].pid = -1;
                LookasideBuffer[i].vpn = -1;
                LookasideBuffer[i].pfn = -1;
            }
            tlbNext = 0;

            // }
            fprintf(output_file,"Current PID: %d. ", currPID);
            fprintf(output_file,"Memory instantiation complete. OFF bits: %s. PFN bits: %s. VPN bits: %s\n",tokens[1], tokens[2], tokens[3]);   
            goto NEXT;         
        }
        //============PART 2 — load instruction============

        if (strcmp(tokens[0], "load") == 0 && isImmediate(tokens[2])) {

            int r = regIndex(tokens[1]);
            if (r == -1) {
                fprintf(output_file,
                        "Current PID: %d. Error: invalid register operand %s\n",
                        currPID, tokens[1]);
                exit(1);
            }

            unsigned int val = immValue(tokens[2]);
            regs[currPID][r] = val;

            fprintf(output_file,
                    "Current PID: %d. Loaded immediate %u into register %s\n",
                    currPID, val, tokens[1]);

            goto NEXT;
        }
        // ============PART 2 — add instruction============
        if (strcmp(tokens[0], "add") == 0) {

            unsigned int v1 = regs[currPID][0];
            unsigned int v2 = regs[currPID][1];
            unsigned int sum = v1 + v2;
            regs[currPID][0] = sum;

            fprintf(output_file,
                    "Current PID: %d. Added contents of registers r1 (%u) and r2 (%u). Result: %u\n",
                    currPID, v1, v2, sum);

            goto NEXT;
        }
        // ============ PART 3: MAP ===============
        if (strcmp(tokens[0], "map") == 0) {

            int vpn = atoi(tokens[1]);
            int pfn = atoi(tokens[2]);

            totalProcesses[currPID][vpn].valid = TRUE;
            totalProcesses[currPID][vpn].pfn = pfn;

            fprintf(output_file,
                "Current PID: %d. Mapped virtual page number %d to physical frame number %d\n",
                currPID, vpn, pfn);

            // ** FIX: immediately update TLB **
            LookasideBuffer[tlbNext].valid = TRUE;
            LookasideBuffer[tlbNext].pid = currPID;
            LookasideBuffer[tlbNext].vpn = vpn;
            LookasideBuffer[tlbNext].pfn = pfn;
            tlbNext = (tlbNext + 1) % 8;

            goto NEXT;
        }

        // ============ PART 3: UNMAP ===============
        if (strcmp(tokens[0], "unmap") == 0) {

            int vpn = atoi(tokens[1]);

            totalProcesses[currPID][vpn].valid = FALSE;

            // invalidate TLB entries
            for (int i = 0; i < 8; i++) {
                if (LookasideBuffer[i].valid &&
                    LookasideBuffer[i].pid == currPID &&
                    LookasideBuffer[i].vpn == vpn) {

                    LookasideBuffer[i].valid = FALSE;
                }
            }

            fprintf(output_file,
                    "Current PID: %d. Unmapped virtual page number %d\n",
                    currPID, vpn);

            goto NEXT;
        }

        // ============ PART 3: STORE ============
        if (strcmp(tokens[0], "store") == 0) {
            int addr = atoi(tokens[1]);
            
            int PAGE_SIZE = 1 << OFF_BITS;           
            int vpn = addr >> OFF_BITS;             
            int off = addr & (PAGE_SIZE - 1);  

            int pfn;
            if (!translateVPN(vpn, &pfn, TRUE))
                goto NEXT;

            int phys = (pfn * PAGE_SIZE) + off;
            // store immediate
            if (isImmediate(tokens[2])) {
                int val = immValue(tokens[2]);
                phys_memory[phys] = val;

                fprintf(output_file,
                        "Current PID: %d. Stored immediate %d into location %d\n",
                        currPID, val, addr);
            }
            // store from register
            else {
                int r = regIndex(tokens[2]);
                int val = regs[currPID][r];

                phys_memory[phys] = val;

                fprintf(output_file,
                        "Current PID: %d. Stored value of register %s (%d) into location %d\n",
                        currPID, tokens[2], val, addr);
            }

            goto NEXT;
        }

        // ============ PART 3: LOAD FROM MEMORY ============
        if (strcmp(tokens[0], "load") == 0) {

            int r = regIndex(tokens[1]);
            int addr = atoi(tokens[2]);

            int PAGE_SIZE = 1 << OFF_BITS;            
            int vpn = addr >> OFF_BITS;              
            int off = addr & (PAGE_SIZE - 1);

            int pfn;
            if (!translateVPN(vpn, &pfn, TRUE))
                goto NEXT;

            int phys = (pfn * PAGE_SIZE) + off; 
            int val = phys_memory[phys];

            regs[currPID][r] = val;

            fprintf(output_file,
                    "Current PID: %d. Loaded value of location %d (%d) into register %s\n",
                    currPID, addr, val, tokens[1]);

            goto NEXT;
        }

        // ============ PART 4: READ FROM MEMORY ============
        if (strcmp(tokens[0], "rinspect") == 0) {
            int r = regIndex(tokens[1]);
            int val = regs[currPID][r];

            fprintf(output_file,
                    "Current PID: %d. Inspected register %s. Content: %d\n",
                    currPID, tokens[1], val);

            goto NEXT;
        }
        if (strcmp(tokens[0], "pinspect") == 0) {
            int vpn = atoi(tokens[1]);

            int pfn;
            if (!translateVPN(vpn, &pfn, FALSE)) {
                fprintf(output_file,
                        "Current PID: %d. Inspected page table entry %d. Physical frame number: 0. Valid: %d\n",
                        currPID, vpn, totalProcesses[currPID][vpn].valid);
                goto NEXT;
            }

            fprintf(output_file,
                    "Current PID: %d. Inspected page table entry %d. Physical frame number: %d. Valid: %d\n",
                    currPID, vpn, pfn, totalProcesses[currPID][vpn].valid);

            goto NEXT;
        }
        if (strcmp(tokens[0], "linspect") == 0) {
            int phys_addr = atoi(tokens[1]);
            int val = phys_memory[phys_addr];

            fprintf(output_file,
                    "Current PID: %d. Inspected physical location %d. Value: %d\n",
                    currPID, phys_addr, val);
            goto NEXT;
        }
        
        

NEXT:
        for (int i = 0; tokens[i] != NULL; i++)
            free(tokens[i]);
        free(tokens);
    }

    // Close input and output files
    free(phys_memory);
    fclose(input_file);
    fclose(output_file);

    return 0;
}