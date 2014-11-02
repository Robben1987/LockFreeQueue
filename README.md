LockFreeQueue
=============

some lock free queues

1. linked-list queue:
   a. To insert new data into the queue a new node is allocated by memory
      buffer and it is inserted into the queue using CAS operations
   b. To remove an element from the queue it uses CAS operations to move
      the pointers of the linked-list and then it retrieves the removed node
      to access the data
2. array queue:
   ...
