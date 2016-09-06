#ifndef LISTNODE_H
#define LISTNODE_H


//================| ListNode Class Declaration |================//

//A type to encapsulate an individual item in a linked list
typedef struct ListNode_struct {
    void* payload;
    struct ListNode_struct* prev;
    struct ListNode_struct* next;
} ListNode;

//Methods
ListNode* ListNode_new(void* payload); 

#endif //LISTNODE_H
