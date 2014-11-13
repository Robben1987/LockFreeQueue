#include <stdio.h>

#define CAS(pcur, oldval, newval)\
	(__sync_bool_compare_and_swap(pcur, oldval, newval))

#define AAF(pcur, val)\
	(__sync_add_and_fetch(pcur, val))

template <typename T>
class LockFreeRingSingleProducer
{
public:
    LockFreeRingSingleProducer(unsigned _size):
		                          size(_size),
								  readIndex(0),
								  writeIndex(0),
								  array(NULL){};

    ~LockFreeRingSingleProducer()
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
    T*          array;
};

template <typename T>
bool LockFreeRingSingleProducer<T>::push(const T* data)
{
    unsigned currentReadIndex;
    unsigned currentWriteIndex;
    
	currentWriteIndex = writeIndex;
	currentReadIndex  = readIndex;
	if (countToIndex(currentWriteIndex + 1) ==
			countToIndex(currentReadIndex))
	{
		// the queue is full
		return false;
	}
        
    // save the data
    array[countToIndex(currentWriteIndex)] = *data;
   
	// increment atomically write index. Now a consumer thread can read
   // the piece of data that was just stored 
   AAF(&writeIndex, 1);
    
    return true;
}

template <typename T>
bool LockFreeRingSingleProducer<T>::pop(T* data)
{
    unsigned currentMaximumReadIndex;
    unsigned currentReadIndex;
    
    do
    {
		// m_maximumReadIndex doesn't exist when the queue is set up as
		// single-producer. The maximum read index is described by the current
		// write index
		currentReadIndex        = readIndex;
        currentMaximumReadIndex = writeIndex;
        
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
