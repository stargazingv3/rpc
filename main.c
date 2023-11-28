#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/wait.h>
#include "wordsProcessorLL.h"

int main(int argc, char* argv[]) {
    if (argc < 2) {
        fprintf(stderr, "Usage: %s <input-file-1.txt> <input-file-2.txt> ... <input-file-n.txt>\n", argv[0]);
        return -1;
    }

    int totalWordsInAllFiles = 0;
	int pipefd[2]; // Create a pipe for communication

    // Create the pipe
    if (pipe(pipefd) == -1) {
        perror("Pipe creation failed");
        return -1;
    }
	
	int i;

    for (i = 1; i < argc; i++) {
        pid_t child_pid = fork();

        if (child_pid == -1) {
            perror("Fork failed");
            return -1;
        }

        if (child_pid == 0) {
            // Child process
			close(pipefd[0]); // Close the reading end of the pipe

            FILE* file = fopen(argv[i], "r");
            if (file == NULL) {
                perror("Failed to open file");
                return -1;
            }

            struct WordNode* wordList = countWordsWithLinkedList(file);
            fclose(file);
			//printf("\n\nThe file %s has: \n",argv[i]);
            //int wordsInFile = printWordsLinkedList(wordList);
            //struct Words* temp = extractList(wordList);
            //printWordssLinkedList(temp);
            printWordsLinkedList(wordList);
            int wordsInFile = 10;
			int uniqueWords = getLinkedListLength(wordList);
			
            //printf("\nThe total number of words is %d; the number of different words is %d.\n",
                   //wordsInFile, uniqueWords);
            freeWordsLinkedList(wordList);
			
			// Send the word count to the parent process through the pipe
            close(pipefd[0]); // Close the reading end again to avoid issues
            write(pipefd[1], &wordsInFile, sizeof(int));
            close(pipefd[1]); // Close the writing end of the pipe
			
			//printf("Trying to add %d to parent \n", wordsInFile);
			
            return 0	;
        }
    }

    // Parent process: wait for all child processes to finish
    for (i = 1; i < argc; i++) {
        int status;
        wait(&status);
    }
	
	close(pipefd[1]); // Close the writing end of the pipe
	
	if(argc > 2){

		// Read word counts from child processes and accumulate them
		int childWordCount;
		while (read(pipefd[0], &childWordCount, sizeof(int)) > 0) {
			totalWordsInAllFiles += childWordCount;
		}
		close(pipefd[0]); // Close the reading end of the pipe

		//printf("\nThe total number of words in all files is %d.\n", totalWordsInAllFiles);
	}
	
    return 0;
}
