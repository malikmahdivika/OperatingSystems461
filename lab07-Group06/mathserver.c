#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <pthread.h>

#define MAX_CONTEXTS 16
#define MAX_OPS 1024
#define LOG_BATCH_SIZE 10


FILE *in;
FILE *out;

typedef struct {
    char *cmd;
    long long val;
    int seq;        // global sequence
} Operation;

// Threads arguments and lock
pthread_mutex_t log_lock = PTHREAD_MUTEX_INITIALIZER;
typedef struct {
    int ctx;
    Operation ops[MAX_OPS];
    int op_count;
} ThreadArgs;

long long contexts[MAX_CONTEXTS] = {0};
Operation ops[MAX_CONTEXTS][MAX_OPS];
int op_count[MAX_CONTEXTS] = {0};

// format string lines into heap for batch printing
// Note that main is responsible for formatting this; returns heap pointer/NULL.
static char *alloc_log(const char *line) {
    size_t len = strlen(line);
    char *buffer = malloc(len + 1);
    if (!buffer) return NULL;
    memcpy(buffer, line, len + 1);

    return buffer;
}

// ---------- Fibonacci ----------
long long fib(long long n) {
    if (n <= 0) return 0;
    if (n == 1) return 1;
    if (n > 92) n = 92;  // prevent overflow

    long long a = 0, b = 1, c;
    for (long long i = 2; i <= n; i++) {
        c = a + b;
        a = b;
        b = c;
    }
    return b;
}

// ---------- Primes ----------
int prime_list(int *out, int n) {
    int count = 0;
    for (int x = 2; x <= n; x++) {
        int isP = 1;
        for (int i = 2; i * i <= x; i++) {
            if (x % i == 0) { isP = 0; break; }
        }
        if (isP) out[count++] = x;
    }
    return count;
}

// ---------- Pi approximation ----------
double pia(int n) {
    double sum = 0.0;
    for (int k = 0; k < n; k++) {
        double t = 1.0 / (2.0*k + 1.0);
        if (k % 2 == 0) sum += t;
        else sum -= t;
    }
    return 4.0 * sum;
}

// Per context thread function
void *context_thread(void *arg) {
    ThreadArgs *args = (ThreadArgs *)arg;
    int ctx = args->ctx;
    char *log_batch[LOG_BATCH_SIZE];
    int batch_count = 0;

    // per thread context execution
    for (int i = 0; i < args->op_count; i++) {
        char line[512];
        Operation *op = &args->ops[i];

        if (strcmp(op->cmd, "set") == 0) {
            contexts[ctx] = op->val;
            snprintf(line, sizeof(line), "ctx %02d: set to value %lld\n", ctx, contexts[ctx]);
        }
        else if (strcmp(op->cmd, "add") == 0) {
            contexts[ctx] += op->val;
            snprintf(line, sizeof(line), "ctx %02d: add %lld (result: %lld)\n", ctx, op->val, contexts[ctx]);
        }
        else if (strcmp(op->cmd, "sub") == 0) {
            contexts[ctx] -= op->val;
            snprintf(line, sizeof(line), "ctx %02d: sub %lld (result: %lld)\n", ctx, op->val, contexts[ctx]);
        }
        else if (strcmp(op->cmd, "mul") == 0) {
            contexts[ctx] *= op->val;
            snprintf(line, sizeof(line), "ctx %02d: mul %lld (result: %lld)\n", ctx, op->val, contexts[ctx]);
        }
        else if (strcmp(op->cmd, "div") == 0 && op->val != 0) {
            contexts[ctx] /= op->val;
            snprintf(line, sizeof(line), "ctx %02d: div %lld (result: %lld)\n", ctx, op->val, contexts[ctx]);
        }
        else if (strcmp(op->cmd, "fib") == 0) {
            long long r = fib((long long)contexts[ctx]);
            snprintf(line, sizeof(line), "ctx %02d: fib (result: %lld)\n", ctx, r);
        }
        else if (strcmp(op->cmd, "pia") == 0) {
            double r = pia((int)contexts[ctx]);
            snprintf(line, sizeof(line), "ctx %02d: pia (result %.15f)\n", ctx, r);
        }
        else if (strcmp(op->cmd, "pri") == 0) {
            /* Do not set a hard limit here; the limit is simply the context value
             * However, we can't use line or else we run into stack smashing problems. Thus
             * we have to set a dynamic buffer to adjust to the (ridiculously) large primes stack.
            */
            int limit = (int)contexts[ctx];
            int *arr = malloc(sizeof(int) * (limit/2 + 5));
            int count = prime_list(arr, limit);

            // estimate needed size; 10chars/number + extra
            size_t buffersize = 50 + count * 12;
            char *line_buffer = malloc(buffersize);
            if (!line_buffer) exit(1);

            char *p = line_buffer;
            int n = snprintf(p, buffersize, "ctx %02d: primes (result:", ctx);
            p += n;
            for (int i = 0; i < count; i++) {
                n = snprintf(p, buffersize - (p - line_buffer), " %d,", arr[i]);
                p += n;
            }
            p[strlen(p) - 1] = ')'; //replace last comma with closing bracket
            snprintf(p, buffersize - (p - line_buffer), "\n");

            log_batch[batch_count++] = line_buffer;     // store for batch logging
            free(arr);
            // We have to skip alloc_log to avoid double incrementing batch_count
            continue;                                   
        }

        // now allocate copy on heap for logging
        log_batch[batch_count++] = alloc_log(line);
        if (batch_count == LOG_BATCH_SIZE) {
            // utilize pthread lock; it's a critical section after all.
            pthread_mutex_lock(&log_lock);
            for (int i = 0; i < batch_count; i++) {
                fputs(log_batch[i], out);
                free(log_batch[i]);
            }
            pthread_mutex_unlock(&log_lock);
            batch_count = 0;
        }

        free(op->cmd); // free strduped command
    }

    // flush any remaining entries
    if (batch_count > 0) {
        pthread_mutex_lock(&log_lock);
            for (int i = 0; i < batch_count; i++) {
                fputs(log_batch[i], out);
                free(log_batch[i]);
            }
        pthread_mutex_unlock(&log_lock);
        batch_count = 0;
    }
    return NULL;
}

// ---------- Tokenizer ----------
char **tokenize(char *str) {
    char **toks = NULL;
    int count = 0;
    char *tok = strtok(str, " ");
    while (tok != NULL) {
        toks = realloc(toks, (count + 1) * sizeof(char *));
        toks[count] = malloc(strlen(tok) + 1);
        strcpy(toks[count], tok);
        count++;
        tok = strtok(NULL, " ");
    }
    toks = realloc(toks, (count + 1) * sizeof(char *));
    toks[count] = NULL;
    return toks;
}

void free_tokens(char **t) {
    for (int i = 0; t[i] != NULL; i++) free(t[i]);
    free(t);
}

int main(int argc, char *argv[]) {
    if (argc != 3) {
        printf("Usage: mathserver.out <input> <output>\n");
        return 1;
    }

    in = fopen(argv[1], "r");
    out = fopen(argv[2], "w");
    if (!in || !out) {
        printf("File error\n");
        return 1;
    }

    char line[512];

    // ---------- Read all operations and store per context ----------
    while (fgets(line, sizeof(line), in)) {
        line[strcspn(line, "\n")] = 0;
        char **t = tokenize(line);
        if (!t[0]) { free_tokens(t); continue; }

        int ctx = atoi(t[1]);
        long long val = (t[2] ? atoll(t[2]) : 0);

        if (op_count[ctx] < MAX_OPS) {
            ops[ctx][op_count[ctx]].cmd = strdup(t[0]);
            ops[ctx][op_count[ctx]].val = val;
            op_count[ctx]++;
        }

        free_tokens(t);
    }

    pthread_t threads[MAX_CONTEXTS];
    ThreadArgs thread_args[MAX_CONTEXTS];

    for (int ctx = 0; ctx < MAX_CONTEXTS; ctx++) {
        thread_args[ctx].ctx = ctx;
        thread_args[ctx].op_count = op_count[ctx];
        memcpy(thread_args[ctx].ops, ops[ctx], sizeof(Operation) * op_count[ctx]);
        pthread_create(&threads[ctx], NULL, context_thread, &thread_args[ctx]);
    }

    // wait for threads to finish, and join
    for (int ctx = 0; ctx < MAX_CONTEXTS; ctx++) {
        pthread_join(threads[ctx], NULL);
    }


    fclose(in);
    fclose(out);
    pthread_mutex_destroy(&log_lock);
    return 0;
}