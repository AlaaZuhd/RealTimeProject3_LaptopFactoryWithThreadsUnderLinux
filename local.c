/* Alaa Zuhd - 1180865
Rawan Yassin - 1182224
*/
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <pthread.h>

struct thread_args {
       int line_num; 
       int step_num; 
};

//laptop struct, stores the following:
struct laptop { 
    int line;               //line number in which this laptop has been manufactured
    int step[10];           //the steps that are already over
    int counter;            //the laptop number in regard to the whole factory
    struct laptop* next;    
};
 
//the queue, front stores the front node of LL and rear stores the last node of LL
//each queue represents a line, in which each line, would make 10 queues, one for each employee
struct Queue {
    struct laptop *front, *rear;
    struct Queue* step[10];
};
 
//a utility function to create a new linked list node.
struct laptop* newNode(int line,int step,int counter)
{
    struct laptop* temp = (struct laptop*)malloc(sizeof(struct laptop));
    temp->line = line;
    for(int i=0; i<10; i++){
        if(i == step)
            temp->step[i]=1;
        else
            temp->step[i]=0;
    }
    temp->counter = counter;
    
    temp->next = NULL;
    return temp;
}
//a utility function to create a new linked list node with the passed parameters.
struct laptop* newNode2(int line,int step0,int step1,int step2,int step3, int step4, int step5, int step6, int step7, int step8, int step9, int counter)
{
    struct laptop* temp = (struct laptop*)malloc(sizeof(struct laptop));
    temp->line = line;
    temp->step[0]=step0;
    temp->step[1]=step1;
    temp->step[2]=step2;
    temp->step[3]=step3;
    temp->step[4]=step4;
    temp->step[5]=step5;
    temp->step[6]=step6;
    temp->step[7]=step7;
    temp->step[8]=step8;
    temp->step[9]=step9;
    temp->counter = counter;
    
    temp->next = NULL;
    return temp;
} 
//a utility function to create an empty queue
struct Queue* createQueue()
{
    struct Queue* q = (struct Queue*)malloc(sizeof(struct Queue));
    q->front = q->rear = NULL;
    return q;
}
//the function to add a key k to q
void enQueue(struct Queue* q, int line,int step0,int step1,int step2,int step3, int step4, int step5, int step6, int step7, int step8, int step9, int counter)
{
    //create a new LL node
    struct laptop* temp = newNode2(line,step0 , step1, step2, step3, step4, step5, step6, step7, step8, step9, counter);
 
    //if queue is empty, then new node is front and rear both
    if (q->rear == NULL) {
        q->front = q->rear = temp;
        return;
    }
 
    //add the new node at the end of queue and change rear
    q->rear->next = temp;
    q->rear = temp;
} 
//function to remove a key from given queue q
struct laptop *deQueue(struct Queue* q)
{
    //if queue is empty, return NULL.
    if (q->front == NULL)
        return;
 
    //store previous front and move front one node ahead
    struct laptop* temp = q->front;
 
    q->front = q->front->next;
 
    //if front becomes NULL, then change rear also as NULL
    if (q->front == NULL)
        q->rear = NULL;
 
    return temp;
}
