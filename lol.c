#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <pthread.h>

typedef struct metaData{
    size_t size;
    unsigned free; //Mempry speace if free = 0 space is occupied if free is = 1 sapce is free
    struct metaData *previous; //Pointer to previous element
    struct metaData *next;     //Pointer to next element MALLOC WILL RETURN THE POINTER OF THE MEMORY ADDRESS HERE

    struct metaData *previousFree;
    struct metaData *nextFree;
} metaData;

//Global
//int AllowedSizes[3];
metaData *head = NULL;
metaData *tail;
pthread_mutex_t memoryLock;

metaData *freeList;

struct metaData *findFreeBlock(size_t size){

    struct metaData *current = head;
    //Traverse through the list of  blocks

    while (current != NULL){

        if (current->free && current->size >= size){
            
            return current;
        }
        current = current->next; //continue onto the next block
    }
    return NULL;
}

/////////////////////////////////////////////////////////////
void *new_malloc(size_t size){

    //struct metaData *meta_dataBlock;
    struct metaData *header;
    void *block;
    size_t totalSize; //This is the total size including the metadata

    if (!size)
        return NULL;

    
    //Thread (Critical Section)
    pthread_mutex_lock(&memoryLock);
    header = findFreeBlock(size); //Returns the address of the free block called from the findFreeBlock() method if found, otherwise return NULL

    if (header != NULL){
        header->free = 0; //Changes the state of the memory block to 0 because it is no longer free
        if(header->size > size + sizeof(struct metaData)){
            void *wholeBlock = (void*) header ; //The memeory address returned by findfreeblock inclduing the metadata
            void *secondPart = wholeBlock + size+sizeof(struct metaData); //The second part is the whole block plus the metadata

            //printf("\nI AM THE WHOLE BLOCK %p", wholeBlock);
            //printf("\nI AM THE SECOND PART %p", secondPart);


            metaData* secondHeader = secondPart; //Create new metadata that stores the data of the previous block and its metadata (NOT SURE)
            secondHeader->free =1; 
            secondHeader->next = header->next;
            secondHeader->previous = header;
            secondHeader->size = header->size - size - sizeof(struct metaData);

            header->next = secondHeader;
            header->size = size;
            if (header == tail) {
                tail = secondHeader;
            }
        }

    
        
        pthread_mutex_unlock(&memoryLock);
        return (void *)(header + 1);
        /*
        +1 is the actual address the free space AFTER the metadata 
        so if requesting 5 bytes it will return a pointer to a memory address with  6 bytes
        */
    }

    //
    //totalSize = sizeof(struct metaData) + size;
    totalSize = size + sizeof(struct metaData); //Total size of the heap INCLUDING the metadata
    block = sbrk(8192);

    //If sbrk does not fail by returning -1 then execute this block of code
    if (block != (void *)-1){ 
        /*
        * Defines the header of the new acquired memory address
        * 
        * 
        * */
        header = block;
        header->size = size;
        header->free = 0;
    
        //void *secondPart = block + size+sizeof(struct metaData) -1 ;
        void *secondPart = block + size+sizeof(struct metaData);

        metaData* secondHeader = secondPart;
        secondHeader->free =1;
        secondHeader->next = NULL;
        secondHeader->previous = header;
        secondHeader->size = 8192- size - 2*sizeof(struct metaData); //Occupy the first header and second header 

        header->next = secondHeader;
        



        //If the linked list is empty then add the header as the first element
        if (head == NULL){
            head = header;
            head->previous = NULL;
        }

        if (tail){
            header->previous = tail;
            tail->next = header; //The next element in the linked becomes the new header (block)
        }

        tail = secondHeader;
        pthread_mutex_unlock(&memoryLock);
        return (void *)(header + 1); //+1 is the actual address the free space AFTER the metaData
    }

    //SBRK returns -1 when it fails and it entire memory is occupied (if sbrkb fails to increase size of heap)
    if (block == (void *)-1){
        pthread_mutex_unlock(&memoryLock);
        return NULL;
    }
}

///////////////////////////////////////////////////////////////////////////////////////////////////////////











//////////////////////////////////////////////////////////////////////////////////////////////////////////////

void new_free(void *memoryBlockPtr){

    struct metaData *header;
    void *request;

    /*
     * If the memory address is NULL then it means that free is trying to free memory that does not exist
     * or is not occupied so it must return out of the program as it can't do anything
    */

    if (!memoryBlockPtr)
        return;

    pthread_mutex_lock(&memoryLock);

     
    header = (struct metaData *)memoryBlockPtr - 1; //Return a pointer to the address containing the metedata 

    request = sbrk(0); //Return a pointer to the current address (increase heap by 0)

    if ((char *)memoryBlockPtr + header->size == request){ 
        //If this is the only block left in linked list then remove pointer by saying head == tail and making the tail and head null
        if (head == tail){
            head = tail = NULL;
           
        }
        /* Else remove last block from list */
        else{
            tail = tail->previous;
            tail->next = NULL;    
        }

        //Negative number to sbrkb decreases heap size by total value of that passed
        sbrk(0 - sizeof(struct metaData) - header->size);
        pthread_mutex_unlock(&memoryLock);
        return;
    }
    //Free the memory = 1
    header->free = 1;
    //pthread_mutex_unlock(&memoryLock);

    //Coalessing the memory blocks 
    if (header -> next != NULL && header->next->free == 1 && (void *)(header -> next )== (void *)(header) + sizeof(struct metaData) + header -> size){

        header -> size = header -> size + header -> next->size + sizeof(struct metaData); //The header is equal 

        if (header -> next -> next != NULL){  //the block next to the next block 
            header -> next -> next -> previous = header; //Make the block next to the next block equal to header
        }

        header -> next = header -> next -> next;
    }

    if(header -> previous != NULL && header->previous->free == 1 && (void *)header == (void *)(header -> previous) + sizeof(struct metaData) + header -> previous -> size){
        
        header = header -> previous;
        header -> size = header -> size + header -> next -> size + sizeof(struct metaData);

          if (header -> next -> next != NULL){  //the block next to the next block 
            header -> next -> next -> previous = header; //Make the block next to the next block equal to header
        }

       header -> next = header -> next -> next;
    }
    
    pthread_mutex_unlock(&memoryLock);


}


void printHeap(){

    metaData * mCurrent = head;
    int idx = 0;

    printf("\n head %p" ,(void *) (head + 1));
    while(mCurrent != NULL){
        printf("\nblock number %d : address : %p   state is %s and size = %zu",idx,(void *) (mCurrent + 1),mCurrent->free?"free   ":"occupied",mCurrent->size);
        mCurrent = mCurrent->next;
        idx++;
    }
    printf("\n tail %p" ,(void *) (tail + 1));
    printf("\n");
}


void displayFreeMemroy(){
    metaData *current = head;
    int totalMemory = 0;

     while(current != NULL){
         if (current->free == 1){
             printf("\nAddress: %p - Bytes Free: %zu",(void *) (current + 1), current->size);
             totalMemory = totalMemory + current->size;      
         }
        current = current->next;        
     }
    printf("\nTotal Free Memory: %d bytes\n", totalMemory);   
}





/*
void myFunction(int x,int c){
    if (x <= 1) 
        return;
    AllowedSizes[c] = x;
    c++;
    x = x / 2;
    myFunction(x, c);
}
*/


int main(){

    //myFunction(8192,0);
    /*
    for(int i=0; i<sizeof(AllowedSizes); i++)
    {
       printf("\n%d", AllowedSizes[i]);
    }
    */

    while (1) {

        char userInput[255];
        printf("\nEnter input: A<bytes> or F<addr>");
        scanf("%s", userInput);

        if (userInput[0] == 'A' || userInput[0] == 'a'){

            char userInputNew[254]; 
            for (int i = 0; i < sizeof(userInputNew) -1 ; i++){
                userInputNew[i] = userInput[i+1];
            }
            //void* acquireBytes = new_malloc(atoi(userInputNew));
            
            char* acquireBytes = (char*)new_malloc(atoi(userInputNew));
            for (int i = 0; i < atoi(userInputNew); i++){
                acquireBytes[i] = i;
            }
            displayFreeMemroy();  
            
        }

        if(userInput[0] == 'P' || userInput[0] == 'p'){
            printHeap();     
        }

        if (userInput[0] == 'F' || userInput[0] == 'f'){
           
            char userInputNew[254]; 
            //printf("test line");
            for (int i = 0; i < sizeof(userInputNew) -1 ; i++){
                userInputNew[i] = userInput[i+1];
            }
            //printf("\n to delete %p",(void*)((size_t)strtol(userInputNew, NULL,0)));
            new_free((void*)((size_t)strtol(userInputNew, NULL,0)));
            displayFreeMemroy();
           
            //printf("Memory Freed");
        }


    }

    
   



}