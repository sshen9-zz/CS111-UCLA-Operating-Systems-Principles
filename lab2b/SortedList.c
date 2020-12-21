/* NAME: Samuel Shen */
/* EMAIL: sam.shen321@gmail.com */
/* ID: 405325252 */

#include <stdio.h>
#include <stdlib.h>
#include "SortedList.h"
#include <sched.h>
#include <string.h>

void SortedList_insert(SortedList_t* list, SortedListElement_t* element){
    if(list==NULL || element==NULL || element->key==NULL){
        return;
    }

    if(opt_yield & INSERT_YIELD){
        sched_yield();
    }
    
    if(list->next==NULL){
        //Empty list
        list->next = element;
        element->next = list;
        element->prev = list;
        list->prev = element;

        return;
        
    }else{
        SortedListElement_t* temp = list;
        //move to the node right before where you want to insert new node
        while(temp->next->key!=NULL && (strcmp(temp->next->key, element->key) < 0)){
            temp = temp->next;
        }

        SortedListElement_t* nextnode = temp->next;
        temp->next = element;
        nextnode->prev = element;
        element->next = nextnode;
        element->prev = temp;
        return;
    }
}

int SortedList_delete(SortedListElement_t* element){
    if(element==NULL || element->next==NULL || element->prev==NULL){
        return 1;
    }

    if(element->next->prev != element->prev->next){
        return 1;
    }

    if(opt_yield & DELETE_YIELD){
        sched_yield();
    }
    
    element->next->prev = element->prev;
    element->prev->next = element->next;

    return 0;
    
}

SortedListElement_t* SortedList_lookup(SortedList_t* list, const char* key){
    if(list==NULL){
        return NULL;
    }

    if(opt_yield & LOOKUP_YIELD){
        sched_yield();
    }
    
    SortedListElement_t* temp = list->next;
    while(temp!=NULL && temp!=list){
        if(strcmp(temp->key, key)==0){
            return temp;
        }
        temp = temp->next;
    }

    return NULL;
}

int SortedList_length(SortedList_t* list){
    if(list==NULL){
        return -1;
    }

    if(opt_yield & LOOKUP_YIELD){
        sched_yield();
    }
    
    int count = 0;
    SortedListElement_t* temp = list->next;
    while(temp!=NULL && temp!=list){
        count+=1;
        temp = temp->next;
    }

    return count;
}
