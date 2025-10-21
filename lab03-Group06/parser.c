#include "parser.h"
#include <string.h>
#include <unistd.h>

//Function to trim whitespace and ASCII control characters from buffer
//[Input] char* inputbuffer - input string to tokenize
//[Input] size_t bufferlen - size of input and output string buffers
//[Output] char** outputbuffer - array of tokens excluding first token
//[Return] size_t - size of output string after trimming
size_t trimstring(char** outputbuffer, const char* inputbuffer, size_t bufferlen)
{   
    // char temp[bufferlen];
    // strcpy(temp, inputbuffer);

    // size_t count = 0;
    // char* token = strtok(temp, " ");
    // while (token != NULL && count < bufferlen) {
    //     outputbuffer[count++] = strdup(token);
    //     token = strtok(NULL, " ");
    // }
    // outputbuffer[count] = NULL;
    // return count;

    const char* ptr = inputbuffer;
    size_t count = 0;
    int isFirst = 1;

    while (*ptr != '\0') {
        // Skip leading whitespace
        while (*ptr == ' ' || *ptr == '\t' || *ptr == '\n') {
            ptr++;
        }
        if (*ptr == '\0') {
            break;
        }

        char* token = NULL;
        if (*ptr == '"' || *ptr == '\'') {
            // Quoted string
            char quote = *ptr++;
            const char* start = ptr;
            while (*ptr && *ptr != quote)
                ptr++;
            size_t len = ptr - start;
            token = malloc(len + 1);
            strncpy(token, start, len);
            token[len] = '\0';
            if (*ptr == quote) ptr++; // Skip closing quote
        } else {
            // Normal token
            const char* start = ptr;
            while (*ptr && *ptr != ' ' && *ptr != '\t' && *ptr != '\n')
                ptr++;
            size_t len = ptr - start;
            token = malloc(len + 1);
            strncpy(token, start, len);
            token[len] = '\0';
        }

        // Skip the first token (the command)
        // if (isFirst) {
        //     free(token);
        //     isFirst = 0;
        //     continue;
        // }

        outputbuffer[count++] = token;
    }

    outputbuffer[count] = NULL;
    return count;
}

//Function to check for existence/executable access of passed command
//[Input] char* command - input command to check
//[Input] size_t bufferlen - size of input buffer
//[Return] const char* full path of desired executable, returns NULL on unsuccessful search/access
const char* accessCheck(const char* command, size_t bufferlen) {
    char* path = getenv("PATH");
    char* pathcopy = strdup(path);
    char* token = strtok(pathcopy, ":");

    while (token != NULL) {
        char fullpath[bufferlen];
        snprintf(fullpath, sizeof(fullpath), "%s/%s", token, command);
        if (access(fullpath, X_OK) == 0) {
            // access returns 0 on executable OK
            return strdup(fullpath);
        }
        // move forwards on tokens
        token = strtok(NULL, ":");
    }
    return NULL;
}

//Function to trim the input command to just be the first word
//[Input] char* inputbuffer - input string to trim
//[Input] size_t bufferlen - size of input and output string buffers
//[Output] char* outputbuffer - output string after trimming 
//[Return] size_t - size of output string after trimming
size_t firstword(char* outputbuffer, const char* inputbuffer, size_t bufferlen)
{
    //TO DO: Implement this function
    char temp[bufferlen];
    strcpy(temp, inputbuffer);
    memcpy(outputbuffer, strtok(temp, " "), bufferlen);

    return strlen(outputbuffer);
}

//Function to test that string only contains valid ascii characters (non-control and not extended)
//[Input] char* inputbuffer - input string to test
//[Input] size_t bufferlen - size of input buffer
//[Return] bool - true if no invalid ASCII characters present
bool isvalidascii(const char* inputbuffer, size_t bufferlen)
{
    //TO DO: Correct this function so that the second test string fails
    size_t testlen = bufferlen;
    size_t stringlength = strlen(inputbuffer);
    if(strlen(inputbuffer) < bufferlen){
        testlen = stringlength;
    }

    bool isValid = true;
    for(size_t ii = 0; ii < testlen; ii++)
    {
        isValid &= ((unsigned char) inputbuffer[ii] <= '~'); //In (lower) ASCII '~' is the last printable character
    }

    return isValid;
}

//Function to find location of pipe character in input string
//[Input] char* inputbuffer - input string to test
//[Input] size_t bufferlen - size of input buffer
//[Return] int - location in the string of the pipe character, or -1 pipe character not found
int findpipe(const char* inputbuffer, size_t bufferlen){
    //TO DO: Implement this function
    for (size_t i = 0; i < bufferlen && inputbuffer[i] != '\0'; i++) {
        if (inputbuffer[i] == '|') {
            return (int)i; // return position of pipe
        }
    }
    return -1; // not found
}

