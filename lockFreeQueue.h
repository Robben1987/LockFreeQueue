#include <stdio.h>
#include <memory.h>

#define CAS(pcur, oldval, newval)\
	(__sync_bool_compare_and_swap(pcur, oldval, newval))

template<typename T>
class LockFreeQueue
{
public:
    typedef struct _Node
	{
        const static unsigned invalid = 0xFFFFFFFF;
        const static unsigned valid   = 0;
        T             value;
        struct _Node*    next;
        unsigned         tag;
    }Node;

    bool EnQueue (T val)
    {
        Node *ptail, *pnext, *pnode;
        pnode = allocNode();
		if(pnode == NULL) 
		{
			printf("allcNode failed !\n");
			return false;
		}
        pnode->value = val;
        pnode->next = NULL;

		do
		{
            ptail = tail;
            pnext = tail->next;

            if(ptail != tail) continue;
            if(pnext != NULL)
            {
                CAS(&tail, ptail, pnext);
                continue;
            }

			// add the the new node to the tail
			if(CAS(&(ptail->next), NULL, pnode)) break;
		} while(1);

        //may be failed
        CAS(&tail, ptail, pnode);//move the tail pointer
	
        return true; 
    }

    bool DeQueue (T* val)
    {
        Node *pnext, *phead, *ptail;
		do
		{
            phead = head;
            ptail = tail;
            pnext = head->next;

            if(phead != head) continue;

            //empty
            if(pnext == NULL) return false;

            if(phead == ptail)
            {
                CAS(&tail, ptail, pnext);
                continue;
            }
            
			if(CAS(&head, phead, pnext)) break;
		} while(1);

        //get the next value, but free the old head, the old next become the new dummy head.    
        if(val != NULL) *val = pnext->value;
        freeNode(phead);
        
        return true;
    }
        
    LockFreeQueue(unsigned _size):
                           size(_size),
                           cur(0),
                           buf(NULL),
                           tail(NULL){}
    ~LockFreeQueue()
	{
		if(buf != NULL) free(buf);
	}   
    bool init()
    {
        buf = (Node*)malloc(sizeof(Node) * size);
		if(buf == NULL)
		{
            printf("init queue failed !\n");
			return false;
		}
        tail = head = allocNode();
		return true;
    }

    Node* allocNode()
    {
        unsigned i = cur;
        unsigned old = i;
		do
		{
            i = (i+1)&(size-1);
		}while(CAS(&(buf[i].tag), Node::valid, Node::invalid) != true);

        CAS(&cur, old, i);//move the cur
        return &buf[i];
    }
    void freeNode(Node* node)
    {
        memset(node, 0, sizeof(Node));
    }

private:
    unsigned            size;
    unsigned volatile   cur;
	Node*    volatile   buf;
    Node*    volatile   head;
    Node*    volatile   tail; 
};

