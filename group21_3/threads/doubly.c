typedef struct listnode node;
typedef struct listnode * nodept;
typedef struct dllist list;
typedef struct dllist * listpt;
#define  pint(n) printf("%d",n) 
struct listnode 							//define the list node structure
{
	
	listnode(int n)
	{
		val=n;
		prev=NULL;
		next=NULL;
	}
	int val;
	struct listnode * prev;
	struct listnode * next;
};
struct dllist								// doubly list containing the head
{
	// dllist()
	// {
	// 	head=NULL;
	// 	tail=NULL;
	// }
	nodept head;
	nodept tail;
};
void printlist(listpt free)						// function to print the list
{
	nodept curr=free->head;
	while(curr!=NULL)
	{
		printf("%d \n",(curr->val));
		curr=curr->next;
	}
}
nodept createnode(int len)					// returns the node pointer with given fields
{
	nodept temp;
	temp=new node(len);
	// temp->val=len;
	// temp->next=NULL;
	// temp->prev=NULL;
	return temp;
}
int deleted(listpt free,nodept curr)					// deletes a node from doubly linked list
{
	if (curr->next ==NULL && curr->prev==NULL)			//check if it has only one node
	{
	free->head=NULL;
	}
	else if (curr==free->head)					//check if the head is deleted
	{
	free->head=curr->next;
	(free->head)->prev=NULL;
	curr->next=NULL;
	} 
	else
	{
		nodept prev=curr->prev;
		nodept next=curr->next;
		curr->prev=NULL;
		curr->next=NULL;
		if (next!=NULL)
		{
			next->prev=prev;
			prev->next=next;
		}
		else
		{
			prev->next=NULL;
			free->tail=prev;
		}
	}
}
int insertd(listpt free,nodept curr)					// insert a new node in the doubly linked list
{
	nodept temp=free->head;
	nodept prevtemp=NULL;
	if (free->head==NULL)						// check if no node exists
	{
		free->head=curr;
		free->tail=curr;
		
	}
	else 				//check if the node is to be inserted in front
	{
		free->head=curr;
		curr->next=temp;
		temp->prev=curr;
	}
	return 1;							// return 1 for valid insertion
}