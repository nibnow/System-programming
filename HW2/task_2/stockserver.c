/* 
 * echoserveri.c - An iterative echo server 
 */ 
/* $begin echoserverimain */
#include "csapp.h"

typedef struct item* item_pointer;
typedef struct item {//thread -> readCnt, mutex
    int ID;
    int left_stock;
    int price;
	int readcnt;
	sem_t mutex, w;
    item_pointer left_child;
    item_pointer right_child;
} item;

item_pointer root;
int client_num = 0;
static sem_t client_mutex;

void echo(int connfd);

item_pointer new_item(int id, int left_stock, int price);
item_pointer insert(item_pointer root, int id, int left_stock, int price, int n);
item_pointer search(item_pointer root, int id);
void inorder(item_pointer ptr, char* temp, char* cur);
void inorder2(item_pointer ptr, char* temp, char* cur);

void *thread(void *vargp);

int main(int argc, char **argv) 
{
    int listenfd, *connfdp;
    socklen_t clientlen;
    struct sockaddr_storage clientaddr;  /* Enough space for any address */  //line:netp:echoserveri:sockaddrstorage
    char client_hostname[MAXLINE], client_port[MAXLINE];

	FILE* fp = fopen("stock.txt", "r");
	int ID, left_stock, price, stock_num = 0;

	while (fscanf(fp, "%d %d %d\n", &ID, &left_stock, &price) != EOF) {
		stock_num++;
		root = insert(root, ID, left_stock, price, stock_num);
	}

	fclose(fp);

	pthread_t tid;
	Sem_init(&client_mutex, 0, 1);

    if (argc != 2) {
		fprintf(stderr, "usage: %s <port>\n", argv[0]);
		exit(0);
    }

    listenfd = Open_listenfd(argv[1]);
    while (1) {
		clientlen = sizeof(struct sockaddr_storage);
		connfdp = Malloc(sizeof(int));
		*connfdp = Accept(listenfd, (SA *)&clientaddr, &clientlen);
        Getnameinfo((SA *) &clientaddr, clientlen, client_hostname, MAXLINE, client_port, MAXLINE, 0);
		printf("Connected to (%s, %s)\n", client_hostname, client_port);
		Pthread_create(&tid, NULL, thread, connfdp);
    }
    exit(0);
}

item_pointer new_item(int id, int left_stock, int price) {
	item_pointer item_new = (item_pointer)malloc(sizeof(item));
	item_new->ID = id;
	item_new->left_stock = left_stock;
	item_new->price = price;
	item_new->readcnt = 0;
	Sem_init(&item_new->mutex, 0, 1);
	Sem_init(&item_new->w, 0, 1);
	item_new->left_child = NULL;
	item_new->right_child = NULL;
	return item_new;
}

item_pointer insert(item_pointer root, int id, int left_stock, int price, int n) {
	if (root == NULL)
		return new_item(id, left_stock, price);
	if (root->ID < id)
		root->right_child = insert(root->right_child, id, left_stock, price, n);
	if (root->ID > id)
		root->left_child = insert(root->left_child, id, left_stock, price, n);
	return root;
}

item_pointer search(item_pointer root, int id) {
	if (root == NULL || root->ID == id)
		return root;

	if (root->ID > id)
		return search(root->left_child, id);

	return search(root->right_child, id);
}

void inorder(item_pointer ptr, char* temp, char* cur)
{
	if (ptr)
	{
		inorder(ptr->left_child, temp, cur);

		P(&ptr->mutex);
		ptr->readcnt++;
		if(ptr->readcnt == 1)
			P(&ptr->w);
		V(&ptr->mutex);

		sprintf(temp, "%d %d %d\n", ptr->ID, ptr->left_stock, ptr->price);
		temp = temp + strlen(temp);

		P(&ptr->mutex);
		ptr->readcnt--;
		if (ptr->readcnt == 0)
			V(&ptr->w);
		V(&ptr->mutex);

		inorder(ptr->right_child, temp, cur);
	}
}

void inorder2(item_pointer ptr, char* temp, char* cur)
{
	if (ptr)
	{
		inorder(ptr->left_child, temp, cur);
		P(&ptr->mutex);
		ptr->readcnt++;
		if(ptr->readcnt == 1)
			P(&ptr->w);
		V(&ptr->mutex);

		sprintf(temp, "%d %d %d\n", ptr->ID, ptr->left_stock, ptr->price);
		temp = temp + strlen(temp);

		P(&ptr->mutex);
		ptr->readcnt--;
		if (ptr->readcnt == 0)
			V(&ptr->w);
		V(&ptr->mutex);
		inorder(ptr->right_child, temp, cur);
	}
}
void *thread(void *vargp)
{
	int connfd = *((int*)vargp);
	Pthread_detach(pthread_self());
	Free(vargp);

	int n;
	char buf[MAXLINE];
	rio_t rio;

	char request[20];
	int id, num;
	item_pointer temp;

	char cur_stock_info[10000];
	char* stock_temp;

	char res[30];

	P(&client_mutex);
	client_num++;
	V(&client_mutex);

	Rio_readinitb(&rio, connfd);
	while ((n = Rio_readlineb(&rio, buf, MAXLINE)) != 0) {
		printf("%s", buf);
		if (!strcmp(buf, "exit\n"))
		{
			P(&client_mutex);
			client_num--;
			if(client_num == 0){
				FILE* fp = fopen("stock.txt", "w");
				stock_temp = &cur_stock_info[0];
				inorder2(root, stock_temp, cur_stock_info);
				printf("%s", cur_stock_info);
				fprintf(fp, "%s", cur_stock_info);
				fclose(fp);
			}
			V(&client_mutex);
			break;
		}
		if (!strcmp(buf, "show\n"))
		{
			stock_temp = &cur_stock_info[0];
			inorder(root, stock_temp, cur_stock_info);
			printf("%s", cur_stock_info);
			Rio_writen(connfd, cur_stock_info, MAXLINE);
		}
		else
		{
			sscanf(buf, "%s %d %d", request, &id, &num);
			temp = search(root, id);

			if (!strcmp(request, "sell"))
			{
				P(&temp->w);
				temp->left_stock += num;
				V(&temp->w);
				strcpy(res, "[sell] success\n");
				Rio_writen(connfd, res, MAXLINE);
			}
			if (!strcmp(request, "buy"))
			{
				if (temp->left_stock < num)
				{
					strcpy(res, "Not enough left stock\n");
					Rio_writen(connfd, res, MAXLINE);
				}
				else
				{
					P(&temp->w);
					temp->left_stock -= num;
					V(&temp->w);
					strcpy(res, "[buy] success\n");
					Rio_writen(connfd, res, MAXLINE);
				}
			}
		}
	}
	Close(connfd);
	return NULL;
}
/* $end echoserverimain */
