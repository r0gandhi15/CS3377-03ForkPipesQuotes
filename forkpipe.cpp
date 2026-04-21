#include <iostream>
#include <fstream>
#include <iomanip>
#include <string>
#include <string.h>
#include <stdio.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>
#include <cstdlib>
#include <time.h>

#include "LineInfo.h"

using namespace std;

static const char* const QUOTES_FILE_NAME = "quotes.txt";

int const READ = 0;
int const WRITE = 1;
int const PIPE_ERROR = -1;
int const FORK_ERROR = -1;
int const CHILD_PID = 0;

int const MAX_PIPE_MESSAGE_SIZE = 1000;
int const MAX_QUOTE_LINE_SIZE = 1000;

//read quotes
void getQuotesArray(char *lines[], unsigned &noLines) {
    FILE *inputFilePointer;

    inputFilePointer = fopen(QUOTES_FILE_NAME, "r");
    if (inputFilePointer == NULL)
        throw domain_error(LineInfo("file open error", __FILE__, __LINE__));

    char input[MAX_QUOTE_LINE_SIZE];
    int size = sizeof(input);

    noLines = 0;

    while (fgets(input, size, inputFilePointer) != NULL) {
        lines[noLines] = strdup(input);
        noLines++;
    }

    fclose(inputFilePointer);
}

//parent process
void executeParentProcess(int pipeParentWriteChildReadfds[], int pipeParentReadChildWritefds[], int noOfParentMessages2Send) {

    //close unused ends
    close(pipeParentWriteChildReadfds[READ]);
    close(pipeParentReadChildWritefds[WRITE]);

    char buffer[MAX_PIPE_MESSAGE_SIZE] = {0};

    for (int i = 0; i < noOfParentMessages2Send; i++) {

        cout << "In Parent: Write to pipe getQuoteMessage sent Message: Get Quote" << endl;

        if (write(pipeParentWriteChildReadfds[WRITE], "Get Quote", MAX_PIPE_MESSAGE_SIZE) == PIPE_ERROR)
            throw domain_error(LineInfo("write pipe error", __FILE__, __LINE__));

        memset(buffer, 0, MAX_PIPE_MESSAGE_SIZE);

        if (read(pipeParentReadChildWritefds[READ], buffer, MAX_PIPE_MESSAGE_SIZE) == PIPE_ERROR)
            throw domain_error(LineInfo("read pipe error", __FILE__, __LINE__));

        cout << "In Parent: Read from pipe pipeParentReadChildMessage read Message:" << endl
            << buffer << endl
             << "-----------------------------------" << endl;
    }

    //send exit
    if (write(pipeParentWriteChildReadfds[WRITE], "Exit", MAX_PIPE_MESSAGE_SIZE) == PIPE_ERROR)
        throw domain_error(LineInfo("write pipe error", __FILE__, __LINE__));

    cout << "In Parent: Write to pipe ParentWriteChildExitMessage sent Message: Exit" << endl;

    close(pipeParentWriteChildReadfds[WRITE]);
    close(pipeParentReadChildWritefds[READ]);

    cout << "Parent Done" << endl;
}

//child process
void executeChildProcess(int pipeParentWriteChildReadfds[], int pipeParentReadChildWritefds[], char *lines[], unsigned noLines) {

    //close unused ends
    close(pipeParentWriteChildReadfds[WRITE]);
    close(pipeParentReadChildWritefds[READ]);

    srand(time(0));

    char receivedMessage[MAX_PIPE_MESSAGE_SIZE];

    while (true) {

        memset(receivedMessage, 0, MAX_PIPE_MESSAGE_SIZE);

        if (read(pipeParentWriteChildReadfds[READ], receivedMessage, MAX_PIPE_MESSAGE_SIZE) == PIPE_ERROR)
            throw domain_error(LineInfo("read pipe error", __FILE__, __LINE__));

        cout << "In Child : Read from pipe pipeParentWriteChildMessage read Message: "
             << receivedMessage << endl;

        //exit
        if (strcmp(receivedMessage, "Exit") == 0)
            break;

        //get quote
        if (strcmp(receivedMessage, "Get Quote") == 0) {

            int randomLineChoice = rand() % noLines;

            char quoteMessage[MAX_PIPE_MESSAGE_SIZE] = {0};
            strcpy(quoteMessage, lines[randomLineChoice]);

            cout << "In Child : Write to pipe pipeParentReadChildMessage sent Message:" << endl
                 << quoteMessage << endl;

            if (write(pipeParentReadChildWritefds[WRITE], quoteMessage, MAX_PIPE_MESSAGE_SIZE) == PIPE_ERROR)
                throw domain_error(LineInfo("write pipe error", __FILE__, __LINE__));
        }
        else {
            //invalid message
            cout << "In Child : Invalid message received: " << receivedMessage << endl;

            if (write(pipeParentReadChildWritefds[WRITE], receivedMessage, MAX_PIPE_MESSAGE_SIZE) == PIPE_ERROR)
                throw domain_error(LineInfo("write pipe error", __FILE__, __LINE__));
        }
    }

    close(pipeParentWriteChildReadfds[READ]);
    close(pipeParentReadChildWritefds[WRITE]);

    cout << "Child Done" << endl;
}

int main(int argc, char* argv[]) {

    try {

        if (argc != 2)
            throw domain_error(LineInfo("Usage: ./forkpipe <number>", __FILE__, __LINE__));

        int noOfParentMessages2Send = atoi(argv[1]);

        char *lines[1000];
        unsigned noLines;

        getQuotesArray(lines, noLines);

        int pipeParentWriteChildReadfds[2];
        int pipeParentReadChildWritefds[2];

        if (pipe(pipeParentWriteChildReadfds) == PIPE_ERROR)
            throw domain_error(LineInfo("pipe error", __FILE__, __LINE__));

        if (pipe(pipeParentReadChildWritefds) == PIPE_ERROR)
            throw domain_error(LineInfo("pipe error", __FILE__, __LINE__));

        pid_t pid = fork();

        if (pid == FORK_ERROR)
            throw domain_error(LineInfo("fork error", __FILE__, __LINE__));

        else if (pid != CHILD_PID) {
            //parent process
            executeParentProcess(pipeParentWriteChildReadfds,pipeParentReadChildWritefds, noOfParentMessages2Send);
        }
        else {
            //child process
            executeChildProcess(pipeParentWriteChildReadfds,pipeParentReadChildWritefds,lines,noLines);
        }

    } catch (exception& e) {
        cout << e.what() << endl;
        cout << endl << "Press the enter key once or twice to leave..." << endl;
        cin.ignore(); cin.get();
        exit(EXIT_FAILURE);
    }

    exit(EXIT_SUCCESS);
}