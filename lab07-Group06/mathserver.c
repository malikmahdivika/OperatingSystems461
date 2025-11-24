#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define MAX_CONTEXTS 16
#define MAX_OPS 1024

typedef struct {
    char *cmd;
    long long val;
} Operation;

long long contexts[MAX_CONTEXTS] = {0};
Operation ops[MAX_CONTEXTS][MAX_OPS];
int op_count[MAX_CONTEXTS] = {0};

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

    FILE *in = fopen(argv[1], "r");
    FILE *out = fopen(argv[2], "w");
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

    // ---------- Execute per context ----------
    for (int ctx = 0; ctx < MAX_CONTEXTS; ctx++) {
        for (int i = 0; i < op_count[ctx]; i++) {
            char *cmd = ops[ctx][i].cmd;
            long long val = ops[ctx][i].val;

            if (strcmp(cmd, "set") == 0) {
                contexts[ctx] = val;
                fprintf(out, "ctx %02d: set to value %lld\n", ctx, contexts[ctx]);
            }
            else if (strcmp(cmd, "add") == 0) {
                contexts[ctx] += val;
                fprintf(out, "ctx %02d: add %lld (result: %lld)\n", ctx, val, contexts[ctx]);
            }
            else if (strcmp(cmd, "sub") == 0) {
                contexts[ctx] -= val;
                fprintf(out, "ctx %02d: sub %lld (result: %lld)\n", ctx, val, contexts[ctx]);
            }
            else if (strcmp(cmd, "mul") == 0) {
                contexts[ctx] *= val;
                fprintf(out, "ctx %02d: mul %lld (result: %lld)\n", ctx, val, contexts[ctx]);
            }
            else if (strcmp(cmd, "div") == 0 && val != 0) {
                contexts[ctx] /= val;
                fprintf(out, "ctx %02d: div %lld (result: %lld)\n", ctx, val, contexts[ctx]);
            }
            else if (strcmp(cmd, "fib") == 0) {
                long long r = fib(contexts[ctx]);
                fprintf(out, "ctx %02d: fib (result: %lld)\n", ctx, r);
            }
            else if (strcmp(cmd, "pia") == 0) {
                double r = pia((int)contexts[ctx]);
                fprintf(out, "ctx %02d: pia (result %.15f)\n", ctx, r);
            }
            else if (strcmp(cmd, "pri") == 0) {
                int limit = (contexts[ctx] > 1000000 ? 1000000 : contexts[ctx]);
                int *arr = malloc(sizeof(int) * (limit/2 + 5));
                int count = prime_list(arr, limit);

                fprintf(out, "ctx %02d: primes (result:", ctx);
                for (int j = 0; j < count; j++) {
                    if (j == 0) fprintf(out, " %d", arr[j]);
                    else fprintf(out, ", %d", arr[j]);
                }
                fprintf(out, ")\n");
                free(arr);
            }

            free(cmd); // free strdup
        }
    }

    fclose(in);
    fclose(out);
    return 0;
}