#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <pthread.h>

typedef struct metaData{
    size_t size; // I think this is length
    //boolean free;
    unsigned free; //Mempry speace if free = 0 space is occupied if free is = 1 sapce is free

    //Double linked list previous and next pointers (DOUBLE CHECK THIS )
    struct metaData *previous; //Pointer to previous element
    struct metaData *next;     //Pointer to next element MALLOC WILL RETURN THE POINTER OF THE MEMORY ADDRESS HERE
} metaData;

metaData *head = NULL;
metaData *tail;

pthread_mutex_t memoryLock;

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

    /**
     * Critical Section -- Allow one thread at a time */
    pthread_mutex_lock(&memoryLock);
    header = findFreeBlock(size); //Returns the address of the free block called from the findFreeBlock() method if found, otherwise return NULL

    if (header != NULL){
        header->free = 0; //Changes the state of the memory block to 0 because it is no longer free
        

        if(header->size > size + sizeof(struct metaData)){
            void *wholeBlock = (void*) header ; //The memeory address returned by findfreeblock inclduing the metadata
            void *secondPart = wholeBlock + size+sizeof(struct metaData);

            metaData* secondHeader = secondPart;
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
    struct metaData *tmp;

    void *programbreak;

    /*
     * If the memory address is NULL then it means that free is trying to free memory that does not exist
     * or is not occupied so it must return out of the program as it can't do anything
    */

    if (!memoryBlockPtr)
        return;

    pthread_mutex_lock(&memoryLock);

    //get pointer to meta-data header 
    header = (struct metaData *)memoryBlockPtr - 1;

    //Get the current address
    programbreak = sbrk(0);

    /* the block that we want to remove is the last block in the list */
    if ((char *)memoryBlockPtr + header->size == programbreak){
        /* If this is the only block left in linked list then remove pointer 
        by saying head == tail and making the tail and head null
        */
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





int main(){

    while (1) {

         char userInput[255];
        printf("\nEnter input: A<bytes> or F<addr>");
        scanf("%s", userInput);
    
        

        if (userInput[0] == 'A' || userInput[0] == 'a'){

            
            char userInputNew[254]; 
            for (int i = 0; i < sizeof(userInputNew) -1 ; i++){
                userInputNew[i] = userInput[i+1];
            }
            void* q = new_malloc(atoi(userInputNew));
            displayFreeMemroy();
        
            //printf("%p", q);
            
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