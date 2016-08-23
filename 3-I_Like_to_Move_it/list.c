#include <inttypes.h>
#include <stdlib.h>
#include "list.h"


//================| ListNode Class Implementation |================//

//Basic list constructor
List* List_new() {
    
    //Malloc and/or fail null
    List* list;
    if(!(list = (List*)malloc(sizeof(List))))
        return list;

    //Fill in initial property values
    //(All we know for now is that we start out with no items) 
    list->count = 0;
    list->root_node = (ListNode*)0;

    return list;
}

//Insert a payload at the end of the list
//Zero is fail, one is success
int List_add(List* list, void* payload) {

    //Try to make a new node, exit early on fail 
    ListNode* new_node;
    if(!(new_node = ListNode_new(payload))) 
        return 0;

    //If there aren't any items in the list yet, assign the
    //new item to the root node
    if(!list->root_node) {
 
        list->root_node = new_node;        
    } else {

        //Otherwise, we'll find the last node and add our new node after it
        ListNode* current_node = list->root_node;

        //Fast forward to the end of the list 
        while(current_node->next)
            current_node = current_node->next;

        //Make the last node and first node point to each other
        current_node->next = new_node;
        new_node->prev = current_node; 
    }

    //Update the number of items in the list and return success
    list->count++;

    return 1;
}

//Get the payload of the list item at the given index
//Indices are zero-based
void* List_get_at(List* list, unsigned int index) {

    //If there's nothing in the list or we're requesting beyond the end of
    //the list, return nothing
    if(list->count == 0 || index >= list->count) 
        return (void*)0;

    //Iterate through the items in the list until we hit our index
    ListNode* current_node = list->root_node;

    //Iteration, making sure we don't hang on malformed lists
    for(unsigned int current_index = 0; (current_index < index) && current_node; current_index++)
        current_node = current_node->next;

    //Return the payload, guarding against malformed lists
    return current_node ? current_node->payload : (void*)0;
}