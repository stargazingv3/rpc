#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include "wordsProcessorLL.h"

int v = 0;

// Function to create a new node in the linked list
struct WordNode* createWordNode(char* word) {
    struct WordNode* newNode = (struct WordNode*)malloc(sizeof(struct WordNode));
    if (newNode == NULL) {
        perror("Memory allocation failed");
        exit(-1);
    }
    newNode->word = strdup(word);  // Duplicate the word
    newNode->count = 1;
    newNode->next = NULL;
    return newNode;
}

// Function to insert a node into the linked list in alphabetical order
void insertNode(struct WordNode** head, char* word) {
    struct WordNode* current = *head;
    struct WordNode* prev = NULL;

    // Find the appropriate position in the list
   /*/ while (current != NULL && strcmp(word, current->word) > 0) {
        prev = current;
        current = current->next;
    }*/

    // If the word is already in the list, increment the count
    if (current != NULL && strcmp(word, current->word) == 0) {
        current->count++;
    } else {
        struct WordNode* newNode = createWordNode(word);

        if (prev == NULL) {
            // Insert at the beginning of the list
            newNode->next = *head;
            *head = newNode;
        } else {
            // Insert in the middle or end of the list
            newNode->next = current;
            prev->next = newNode;
        }
    }
}


// Function to check if a character is a valid word character
int isWordCharacter(char c) {
	if(((c == '-') || (c == '\'')) && (v == 1)){ //Using a global variable as a flag to detect if these 2 characters, which
	//individually can be part of a word, occur together.
		v = 0;
		return 0;
	}
	if((c == '-') || (c == '\'')) {
		v = 1;
	}
    return isalpha(c) || c == '-' || c == '\'';
}

struct WordNode* countWordsWithLinkedList(FILE* file) {
    struct WordNode* wordList = NULL;
    char buffer[256];

    while (fscanf(file, "%255s", buffer) == 1) {
		
        char* token = strtok(buffer, " ,.;:!?\t\n\"");
		
		//char cmp[] = "one-horse";
		
		//int result = strcmp(cmp, token);

    //if (result == 0) {
    //    printf("The strings are equal.\n");
   // }
		
		//printf(token);
		//printf("\n");
        while (token != NULL) {
			//printf(token);
			//printf("\n");
            // Check if the token contains only valid word characters
            int i;
            int isWord = 1;
			v = 0;
            for (i = 0; token[i] != '\0'; i++) {
				/*if (result == 0) {
						printf("%d\n",token[i]);
					}*/
                if (!isWordCharacter(token[i])) {
                    isWord = 0;
					/*if (result == 0) {
						printf("Rejected: %d\n",token[i]);
					}*/
                    break;
                }
            }

            if (isWord) {
				int l = strlen(token);
				if(token[l-1] == '\''){
					char* newt = malloc(l-1);
					newt[l-1] = '\0';
					strncpy(newt, token, l-1);
					insertNode(&wordList, newt);
					free(newt);
				}
				else if(token[0] == '\''){
					char* newt = malloc(l-1);
					newt[l-1] = '\0';
					strncpy(newt, token+1, l);
					insertNode(&wordList, newt);
					free(newt);
				}
				else{
					/*if (result == 0) {
						printf("Inserting one-horse \n");
					}*/
					insertNode(&wordList, token);
				}
            }

            token = strtok(NULL, " ,.;:!?\t\n");
        }
    }

    return wordList;
}

// Function to print the linked list and return the total number of words
int printWordsLinkedList(struct WordNode* head) {
    int totalWords = 0;

    while (head != NULL) {
        //printf("%s appears %d times\n", head->word, head->count);
		printf("%s \n", head->word);
        totalWords += head->count;
        head = head->next;
    }

    return totalWords;
}

// Function to calculate the length of a linked list
int getLinkedListLength(struct WordNode* head) {
    int length = 0;  // Initialize the length to 0

    // Traverse the list and count nodes
    struct WordNode* current = head;
    while (current != NULL) {
        length++;
        current = current->next;
    }

    return length;
}

// Function to free the memory used by the linked list
void freeWordsLinkedList(struct WordNode* head) {
    while (head != NULL) {
        struct WordNode* temp = head;
        head = head->next;
        free(temp->word);
        free(temp);
    }
}

struct Words* createWordsNode(char* word){
	struct Words* newNode = (struct Words*)malloc(sizeof(struct Words));
    newNode->word = strdup(word);
	newNode->next = NULL;
    return newNode;
}

struct Words* extractList(struct WordNode* node){
	struct Words* head = createWordsNode(node->word);
    struct WordNode* temp = node;
    struct Words* temp2 = head;
    while(temp->next){
        temp = temp->next;
        struct Words* temp3 = createWordsNode(temp->word);
        temp2->next = temp3;
        temp2 = temp3;
    }
    return head;
}

// Function to print the linked list and return the total number of words
int printWordssLinkedList(struct Words* head) {
    int totalWords = 0;

    while (head != NULL) {
        //printf("%s appears %d times\n", head->word, head->count);
		printf("%s \n", head->word);
        //totalWords += head->count;
        head = head->next;
    }

    return totalWords;
}