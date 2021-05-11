/*
    BS Bonus-03
    
    Ref:
    Struct Buffer and functions #BufferIn and #BufferOut are based on the example from 
    https://www.mikrocontroller.net/articles/FIFO#Standard-Ringpuffer
    
*/

#define BUFFER_FAIL     0
#define BUFFER_SUCCESS  1
#define BUFFER_SIZE 23

#include <sys/mman.h>
#include <unistd.h>
#include <fcntl.h>
#include <stdio.h>
#include <taskLib.h>
#include <sys/stat.h>
#include <semLib.h>
/*#include <semSmLib.h>*/


/* Semaphore to manage the log messages.
    For simplification, it is declared as in global scope and not in shared memory. */
SEM_ID printSem;

struct Buffer 
{
  /* Array which contains the produced data. */
  int data[BUFFER_SIZE];
  /* Read position */
  int read; 
  /* Write position */
  int write; 
} typedef Buffer_t;

/* Logs the received data from the consumers to the console. */
void log(char *name, int msg) 
{
    /* Block all other prints. */
	semTake(printSem, WAIT_FOREVER);
    /* Print message to console. */
	printf("%s: %d\n", name, msg);
    /* Unblock the console again. */
	semGive(printSem);
}

/* Writes new data to the buffer. */
int BufferIn(Buffer_t* buffer, int byte)
{
  /* Check, if the buffer has some free space for new data. */
  if ( ( buffer->write + 1 == buffer->read ) ||
       ( buffer->read == 0 && buffer->write + 1 == BUFFER_SIZE ) ) 
  {   
    return BUFFER_FAIL;
  }

  /* Write new data to buffer. */
  buffer->data[buffer->write] = byte;

  /* Increment write position to indicate new data. */
  buffer->write++;
  
  /* Jump to first position if the end of the array has been reached. */
  if (buffer->write >= BUFFER_SIZE)
  {
    buffer->write = 0;
  }

  return BUFFER_SUCCESS;
}

/* Read data from buffer. */
int BufferOut(Buffer_t *buffer, int *pByte)
{
  /* Check, if new data is available. */
  if (buffer->read == buffer->write)
  {
    return BUFFER_FAIL;
  }

  /* Read new value from buffer. */
  *pByte = buffer->data[buffer->read];

  /* Increment read conter to indecate next consumer where to read. */
  buffer->read++;
  
  /* Jump to first position if the end of the array has been reached. */
  if (buffer->read >= BUFFER_SIZE)
  {
    buffer->read = 0;
  }
  
  return BUFFER_SUCCESS;
}

/* The producer create new data and adds it to the buffer. */
int producer()
{
	/* Access shared memory. */
		int mem = shm_open("/aufgabe3", O_RDWR, S_IRUSR|S_IWUSR); /* ! */
	    /* Error handeling. */
	    if(mem == -1) /* ! */
	    {
	       return 0;
	    }
     
	    /* Create memory for the buffer and two semaphores. (One for read control and
	        one for notification). */
		int size = sizeof(Buffer_t)+ sizeof(SEM_ID)*2;
		/*ftruncate(mem, size);*/
		char* data = mmap(NULL, size, PROT_READ|PROT_WRITE, MAP_SHARED, mem, 0); /* ! */ 
		close(mem);

	    /* Read all data structures from memory. */
		Buffer_t* buffer = (Buffer_t*) data;
		SEM_ID* sem = (SEM_ID*) (data + sizeof(Buffer_t));
		SEM_ID* notify = (SEM_ID*) (data + sizeof(Buffer_t) + sizeof(SEM_ID));
		printf("Sem in producer: %x\n", *sem);
		printf("Notify in producer: %x\n", *notify);
		taskDelay(60);		

    
    /* Start to produce new values forever. */
    int value = rand() % 1000;
	while(1)
	{
        /* Check, if there is enough space for the new value. */
		if(BufferIn(buffer, value) == BUFFER_FAIL)
		{
            /* Failed to deliver new data. Wait some time and try again. */
			taskDelay(50);
		} 
		else 
		{
            /* Delivery successful. Produce new value. */
            value = rand() % 1000; /* ! */
            /* Notify the next waiting consumer, that new data is available. */
			semGive(*notify);
		}
	}
    
    /* Producer finished working. Clean up. */
	munmap(data, size);
}

/* The consumer reads the data from the buffer and prints it to the console. */
int consumer(char* name)
{
	/* Access shared memory. */
	int mem = shm_open("/aufgabe3", O_RDWR, S_IRUSR|S_IWUSR); /* ! */
	/* Error handeling. */
	if(mem == -1) /* ! */
	{
	   return 0;
	}
	        
	    /* Create memory for the buffer and two semaphores. (One for read control and
	        one for notification). */
	int size = sizeof(Buffer_t)+ sizeof(SEM_ID)*2;
	char* data = mmap(NULL, size, PROT_READ|PROT_WRITE, MAP_SHARED, mem, 0); /* ! */
	close(mem);
		
    /* Read all data structures from memory. */
	Buffer_t* buffer = (Buffer_t*) data;
	SEM_ID* sem = (SEM_ID*) (data + sizeof(Buffer_t));
	SEM_ID* notify = (SEM_ID*) (data + sizeof(Buffer_t) + sizeof(SEM_ID));

    
    /* Start to fetch data from the producer. Won't terminate in this implementation. */
	while(1)
	{
        /* Take semaphore to inshure, that only one consumer can access one produced value. */
		semTake(*sem, WAIT_FOREVER);
        
		int result = 0;
        int success = BufferOut(buffer, &result); /* ! */
        /* Read finished. Give semaphore back to allow next consumer to continue. */
        semGive(*sem); /* ! */
        
        /* Check, if the read was successful. */
		if(success == BUFFER_FAIL)
		{
            /* The buffer reached the writing positioin. 
               The semaphore is taken to wait until the producer has new data produced.
               In case this consumer is the first one to wait, take the semaphore twice in
               order to block until the producer notifies the consumber. */
            
            semTake(*notify, NO_WAIT); /* ! */
            semTake(*notify, WAIT_FOREVER);
            
              
		}
        else /* ! */
        {
            /* Log the received data. */
            log(name, result);
        }
	}
	
    /* Consumer finished working. Clean up. */
	munmap(data, size);
}

int initApp()
{
    /* Create semaphore to controle the console output. The variable is declared in global
        scope for simplification. */
	printSem = semBCreate(SEM_Q_FIFO, SEM_FULL);
	
	
	/* Create shared memory. */
	int mem = shm_open("/aufgabe3", O_CREAT|O_RDWR, S_IWUSR|S_IRUSR); /* ! */
	/* Create memory for the buffer and two semaphores. (One for read control and
	   one for notification, when there is new stuff for the consumers). */
	int size = sizeof(Buffer_t) + sizeof(SEM_ID) * 2;
	ftruncate(mem, size);
	char* data = mmap(NULL, size, PROT_WRITE, MAP_SHARED, mem, 0);
	close(mem);
	    
	/* Add data structure to shared memory. */
		
	/* The buffer stores the produced values and controls the read and write access. */
	Buffer_t* buffer = (Buffer_t*) data;
	buffer->read = 0;
	buffer->write = 0;
	
	SEM_ID sem = semBCreate(SEM_Q_FIFO, SEM_FULL);
	printf("Sem Adresse: %p\n", sem);
	* ((SEM_ID*) (data + sizeof(Buffer_t))) = sem;


	/* A second semaphore is used to notify the consumer about new data in case the consumers
	   are faster than the consumer. 
	   The second semaphore is introduced to allow the producer to produce independent from
	   the consumers and to allow the consumers to catch up with the producer. */
	SEM_ID notify = semBCreate(SEM_Q_FIFO, SEM_FULL);
	printf("Notify Adresse: %p\n", notify);
	* ((SEM_ID*) (data + sizeof(Buffer_t)+ sizeof(SEM_ID))) = notify;

	/* Create all tasks. */
	TASK_ID producerg = taskCreate("Producer", 100, 0, 0, (FUNCPTR)producer,0,0,0,0,0,0,0,0,0,0);
	TASK_ID consumer1 = taskCreate("Consumer1", 100, 0, 0, (FUNCPTR)consumer,"Consumer-1",0,0,0,0,0,0,0,0,0);
	TASK_ID consumer2 = taskCreate("Consumer2", 100, 0, 0, (FUNCPTR)consumer,"Consumer-2",0,0,0,0,0,0,0,0,0);
				
	/* Activate Tasks. */
	taskActivate(consumer1);
	taskActivate(consumer2);
	taskActivate(producerg);	
	
    /* Wait for tasks to finish. */
	taskWait(producerg, WAIT_FOREVER);
	taskWait(consumer1, WAIT_FOREVER);
	taskWait(consumer2, WAIT_FOREVER);

    /* Clean up. */
	semDelete(printSem);
	semDelete(sem);
	semDelete(notify);
    
    munmap(data, size);
    
    shm_unlink("/aufgabe3");
	
}
