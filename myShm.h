/* myShm.h */
/* Header file to be used with master.c and slave.c
*/

struct SHARED_MEM_CLASS {
	int index;			// index to next available response slot
	int response[10];	// each child writes its child number here
};
