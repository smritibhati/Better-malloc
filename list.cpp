//#include <bits/stdc++.h>
#include <iostream>
#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>
#include <sys/mman.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <stdint.h>

using namespace std;

typedef struct block_t {
	size_t		size;
	struct block_t   *next;
	struct block_t   *prev;
}		block_t;

struct list {
	int count;
	int size;
	struct block_t *head;
	struct block_t *tail;
}*freelist,* alloclist;
void *ptr=NULL;

#define BLOCK_MEM(ptr) ((void *)((unsigned long)ptr + sizeof(block_t)))
#define BLOCK_HEADER(ptr) ((void *)((unsigned long)ptr - sizeof(block_t)))

int Mem_Init(int sizeOfRegion)			// initializations
{
    if(sizeOfRegion<=0)
    {
    	perror("invalid size of region");
    	return -1;

    }

    int fd = open("/dev/zero", O_RDWR);
    ptr = mmap(NULL, 7*sizeOfRegion, PROT_READ | PROT_WRITE, MAP_PRIVATE, fd, 0);
    if (ptr == MAP_FAILED) {
        perror("mmap");
        return -1;
    }

		freelist = (struct list *)ptr;
		alloclist = ((struct list *)ptr+sizeof(struct list));
		freelist->head = (block_t *)( alloclist + sizeof(struct list));
		freelist->head->size = 7*sizeOfRegion - 2*sizeof(struct list) - sizeof(block_t);
		freelist->size = freelist->head->size;
		alloclist->head= NULL;
		freelist->tail = freelist->head;
		freelist->count = 1;
		alloclist->tail= NULL;

    close(fd);
    return 1;
}
block_t * split(block_t * b, size_t size)
{
	void  *mem_block = BLOCK_MEM(b);
	block_t *newptr = (block_t *) ((unsigned long)mem_block + size);
	newptr->size = b->size - (size + sizeof(block_t));
	b->size = size;
	return newptr;
}
void addtoalloc(block_t * ptr, int size){
	if(alloclist->head==NULL){
		alloclist ->head =ptr;
	}

	alloclist ->tail->next =ptr;
	alloclist->tail->next->prev = alloclist->tail;
	alloclist->tail= alloclist->tail->next;

	alloclist->size += size;
}
void freelistmodify(block_t *biggest,block_t *newptr, int size){
	if(biggest->prev)
		biggest->prev->next=newptr;
	else{
		freelist->head = newptr;
	}
	if(biggest->next)
		biggest->next->prev=newptr;
	else{
		freelist->tail = newptr;
	}
	freelist->size-=size;
}
void deletefromfree(block_t *ptr, int size){
	if(ptr->prev==NULL){
		freelist->head= ptr->next;
	}
	else{
		ptr->prev->next= ptr->next;
	}
	if(ptr->next){
		ptr->next->prev= ptr-> prev;
	}
	else{
		freelist->tail=ptr->prev;
	}
	freelist->size-=ptr->size;
}
void addtofree(block_t *ptr, int size){
	ptr->prev=NULL;
	ptr->size= size;
	if(freelist->head){
		freelist->tail->next = ptr;
		ptr->prev=freelist->tail;
		freelist->tail= freelist->tail->next;
	}
	else{
		freelist->head=freelist->tail= ptr;
	}
	ptr->next=NULL;
	freelist->size+=size;
}
void deletefromallocate(block_t *ptr, int size){
	if(ptr->prev==NULL){
		alloclist->head= ptr->next;
	}
	else{
		ptr->prev->next= ptr->next;
	}
	if(ptr->next){
		ptr->next->prev= ptr-> prev;
	}
	else{
		alloclist->tail=ptr->prev;
	}
	alloclist->size-=size;
}
void *Mem_Alloc(int size)
{
	if(size>freelist->size)			// if free space is less than required
	{
		cout<<"not enough memory\n";
		return NULL;
	}

	block_t *biggest=freelist->head;
	block_t *current=biggest;
//	void *userptr=biggestnode->size;			// pointer which will be returned to user
//	int biggestsize=biggestnode->size;

	while(current->next)				// worst fit
	{
		if(biggest->size < current->size)
			biggest=current;
		current=current->next;
	}

	void * userptr=BLOCK_MEM(biggest);

	block_t *newptr;

	if(size<biggest->size)				// partial allocation
	{
		newptr = split(biggest, size);
		addtoalloc(biggest, size);
		freelistmodify(biggest,newptr,size);
	}
	else if(size==biggest->size)		// full allocation
	{
		addtoalloc(biggest, size);
		deletefromfree(biggest, size);
	}
	else							// if biggest node is less than size required
	{
		cout<<"not enough memory\n";
		return NULL;
	}
	return userptr;
}
int isValid(void *user_ptr)
{
	block_t *currentnode=alloclist->head;
	unsigned long long int low,high;			//traverse the allocated list to find the particular alloc header
	while(currentnode)
	{
		low= (uintptr_t)BLOCK_MEM(currentnode);
		high=low + currentnode->size;
		if((uintptr_t)low<=(uintptr_t)user_ptr && (uintptr_t)user_ptr<(uintptr_t)high)
			return 1;
		currentnode= currentnode->next;
	}
	return 0;
}

int Mem_Free(void *ptr1)
{
	if(ptr1==NULL)
		return -1;
	if(!isValid(ptr1))				//check whether ptr provided by user is within the valid range
	{
		cout<<"Not a valid pointer\n";
		return -1;
	}
	block_t *currentnode=alloclist->head;
	unsigned long long int low,high;

	while(currentnode)		//traverse the allocated list to find the particular alloc header
	{

		low=(uintptr_t)BLOCK_MEM(currentnode);
		high=low + currentnode->size;

		if((uintptr_t)low<=(uintptr_t)ptr1 && (uintptr_t)ptr1<(uintptr_t)high)
			break;
		currentnode=currentnode->next;
	}

	int actual_size=currentnode->size;
	currentnode->size=(uintptr_t)ptr1 - (uintptr_t)low;

	block_t *newfree;
	if(currentnode->size>0)						//partially freed
	{
		addtofree((block_t *)ptr1,actual_size - currentnode->size );
	}
	else								//fully freed
	{
		addtofree(currentnode, actual_size);
		deletefromallocate(currentnode, actual_size);
	}
	// mergeRight((void*)newfree);					//merge 2 adjacent free block into one bigger free block
	// mergeLeft((void*)newfree);					//merge 2 adjacent free block into one bigger free block
	return 1;
}
int main()
{
	if(!Mem_Init(8))
	{
		cout<<"init failed";
		return 1;
	}
	void *ptr=Mem_Alloc(4);
	//Mem_Free(ptr+20);
	Mem_Free(ptr);
	/*void *ptr1=Mem_Alloc(4);
	cout<<"hello";
	void *ptr2=Mem_Alloc(1);
	cout<<"hi";
	Mem_Free(ptr1+3);
	Mem_Free(ptr1+1);
	ptr2=Mem_Alloc(5);
	Mem_Free(ptr+1);
	cout<<"hi";
	ptr2=Mem_Alloc(5);*/
	//cout<<ptr1<<"-----------\n";
	/*void *ptr2=Mem_Alloc(16);
	cout<<"Is Valid of Ptr2 "<<Mem_IsValid(ptr2+1)<<endl;
	Mem_Free(ptr1+4);
	cout<<"Is Valid of Ptr1+3 "<<Mem_IsValid(ptr1+3)<<endl;
	cout<<"Is Valid of Ptr1+4 "<<Mem_IsValid(ptr1+4)<<endl;
	Mem_Free(ptr2+8);
	cout<<"Is Valid of Ptr2 "<<Mem_IsValid(ptr2+1)<<endl;
	void *ptr3=Mem_Alloc(15);
	Mem_Free(ptr3+8);
	// void *ptr4=Mem_Alloc(16);
	// Mem_Free(ptr4);
	// void *ptr5=Mem_Alloc(16);
	// Mem_Free(ptr5+8);
	//cout<<"Is Valid of Ptr1 "<<Mem_IsValid(ptr1+5)<<endl;

	cout<<"Is Valid of Ptr2+3 "<<Mem_IsValid(ptr2+3)<<endl;
	cout<<"Is Valid of Ptr2+4 "<<Mem_IsValid(ptr2+4)<<endl;
	cout<<"Is Valid of Ptr2+5 "<<Mem_IsValid(ptr2+5)<<endl;*/
	return 0;
}
