#ifndef WORDSPROCESSORLL_H
#define WORDSPROCESSORLL_H

// Structure for a node in the linked list
struct WordNode {
    char* word;
    int count;
    struct WordNode* next;
};

struct Words{
	char* word;
	struct Words* next;
};

// Function prototypes
struct WordNode* countWordsWithLinkedList(FILE* file);
int printWordsLinkedList(struct WordNode* head);
void freeWordsLinkedList(struct WordNode* head);
int getLinkedListLength(struct WordNode* head);
struct Words* extractList(WordNode* node);
#endif
