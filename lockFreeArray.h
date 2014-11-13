#include <stdio.h>

#define CAS(pcur, oldval, newval)\
	(__sync_bool_compare_and_swap(pcur, oldval, newval))

template <typename T>
class LockFreeRing
{
public:
    LockFreeRing(unsigned _size):size(_size),
	                             readIndex(0),
								 writeIndex(0),
								 maximumReadIndex(0),
								 array(NULL){};

    ~LockFreeRing()
    {
        if(array != NULL) free(array);
    };

    bool init()
	{
		array = (T*)calloc(size, sizeof(T));
		if(array == NULL) return false;
		return true;
	}
    unsigned countToIndex(unsigned count) {return (count & (size - 1));}
    bool push(const T* data);
    bool pop(T* data);
    
private:
    unsigned    size;    /* maximum number of elements must be 2^n*/
    long long   readIndex;
    long long   writeIndex;
    long long   maximumReadIndex;
    T*          array;
};

template <typename T>
bool LockFreeRing<T>::push(const T* data)
{
    unsigned currentReadIndex;
    unsigned currentWriteIndex;
    
    do
    {
        currentWriteIndex = writeIndex;
        currentReadIndex  = readIndex;
        if (countToIndex(currentWriteIndex + 1) ==
            countToIndex(currentReadIndex))
        {
            // the queue is full
            return false;
        }
        
    } while (!CAS(&(writeIndex), currentWriteIndex, (currentWriteIndex + 1)));
    
    // We know now that this index is reserved for us. Use it to save the data
    array[countToIndex(currentWriteIndex)] = *data;
    
    // update the maximum read index after saving the data. It wouldn't fail if there is only one thread
    // inserting in the queue. It might fail if there are more than 1 producer threads because this
    // operation has to be done in the same order as the previous CAS
    
    while (!CAS(&(maximumReadIndex), currentWriteIndex, (currentWriteIndex + 1)))
    {
        // this is a good place to yield the thread in case there are more
        // software threads than hardware processors and you have more
        // than 1 producer thread
        // have a look at sched_yield (POSIX.1b)
        sched_yield();
    }
    
    return true;
}

template <typename T>
bool LockFreeRing<T>::pop(T* data)
{
    unsigned currentMaximumReadIndex;
    unsigned currentReadIndex;
    
    do
    {
        // to ensure thread-safety when there is more than 1 producer thread
        // a second index is defined (maximumReadIndex)
        currentReadIndex        = readIndex;
        currentMaximumReadIndex = maximumReadIndex;
        
        if (countToIndex(currentReadIndex) ==
            countToIndex(currentMaximumReadIndex))
        {
            // the queue is empty or
            // a producer thread has allocate space in the queue but is
            // waiting to commit the data into it
            return false;
        }
        
        // retrieve the data from the queue
        *data = array[countToIndex(currentReadIndex)];
        
        // try to perfrom now the CAS operation on the read index. If we succeed
        // a_data already contains what readIndex pointed to before we
        // increased it
        if (CAS(&(readIndex), currentReadIndex, (currentReadIndex + 1)))
        {
            // it failed retrieving the element off the queue. Someone else must
            // have read the element stored at countToIndex(currentReadIndex)
            // before we could perform the CAS operation
            break;
        }
        
    } while(1); // keep looping to try again!
    
    return true;
}
