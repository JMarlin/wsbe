#include <inttypes.h>
#include <stdlib.h>
#include "listnode.h"


//================| ListNode Class Implementation |================//

//Basic listnode constructor
ListNode* ListNode_new(void* payload) {

    //Malloc and/or fail null
    ListNode* list_node;
    if(!(list_node = (ListNode*)malloc(sizeof(ListNode))))
        return list_node;

    //Assign initial properties
    list_node->prev = (ListNode*)0;
    list_node->next = (ListNode*)0;
    list_node->payload = payload; 

    return list_node;
}

