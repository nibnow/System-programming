/* 
 * echoserveri.c - An iterative echo server 
 */ 
/* $begin echoserverimain */
#include "csapp.h"

typedef struct {
	int maxfd;
	fd_set read_set;
	fd_set ready_set;
	int nready;
	int maxi;
	int clientfd[FD_SETSIZE];
	rio_t clientrio[FD_SETSIZE];
} pool;

int byte_cnt = 0;

typedef struct item *item_pointer;
typedef struct item{//thread -> readCnt, mutex
	int ID;
	int left_stock;
	int price;
	item_pointer left_child;
	item_pointer right_child;
} item;

item_pointer root;

char cur_stock_info[10000];
char *stock_temp;
int client_num = 0;

void echo(int connfd);

void init_pool(int listenfd, pool* p);
void check_clients(pool* p);
void add_client(int connfd, pool* p);

item_pointer new_item(int id, int left_stock, int price);
item_pointer insert(item_pointer root, int id, int left_stock, int price, int n);
item_pointer search(item_pointer root, int id);
void inorder(item_pointer ptr);
void inorder2(item_pointer ptr);

int main(int argc, char **argv) 
{
    int listenfd, connfd;
    socklen_t clientlen;
    struct sockaddr_storage clientaddr;  /* Enough space for any address */  //line:netp:echoserveri:sockaddrstorage
    char client_hostname[MAXLINE], client_port[MAXLINE];
	static pool pool;
	
	FILE *fp = fopen("stock.txt", "r");
	int ID, left_stock, price, stock_num = 0;

	while(fscanf(fp, "%d %d %d\n", &ID, &left_stock, &price) != EOF) {
		stock_num++;
		root = insert(root, ID, left_stock, price, stock_num);
	}

	fclose(fp);

    if (argc != 2) {
		fprintf(stderr, "usage: %s <port>\n", argv[0]);
		exit(0);
    }

    listenfd = Open_listenfd(argv[1]);
	init_pool(listenfd, &pool);

    while (1) {
		/* Wait for listening/connected descriptor(s) to become ready */
		pool.ready_set = pool.read_set;
		pool.nready = Select(pool.maxfd + 1, &pool.ready_set, NULL, NULL, NULL);

		/* If listening descriptor ready, add new client to pool */
		if (FD_ISSET(listenfd, &pool.ready_set)) {
			clientlen = sizeof(struct sockaddr_storage);
			connfd = Accept(listenfd, (SA*)&clientaddr, &clientlen);
			Getnameinfo((SA *) &clientaddr, clientlen, client_hostname, MAXLINE, client_port, MAXLINE, 0);
			printf("Connected to (%s, %s)\n", client_hostname, client_port);

			add_client(connfd, &pool);
		}
		/* Echo a text line from each ready connected descriptor */
		check_clients(&pool);
		if(client_num == 0)
		{
			FILE *fp = fopen("stock.txt", "w");
			stock_temp = &cur_stock_info[0];
			inorder2(root);
			printf("%s", cur_stock_info);
			fprintf(fp, "%s", cur_stock_info);
			fclose(fp);
		}
    }
    exit(0);
}

void init_pool(int listenfd, pool* p)
{
	/* Initially, there are no connected descriptors */
	int i;
	p->maxi = -1;
	for (i = 0; i < FD_SETSIZE; i++)
		p->clientfd[i] = -1;
	
	/* Initially, listenfd is only member of select read set */
	p->maxfd = listenfd;
	FD_ZERO(&p->read_set);
	FD_SET(listenfd, &p->read_set);
}

void add_client(int connfd, pool* p)
{
	int i;
	p->nready--;
	client_num++;
	for (i = 0; i < FD_SETSIZE; i++) /* Find an available slot */
		if (p->clientfd[i] < 0) {
			/* Add connected descriptor to the pool */
			p->clientfd[i] = connfd;
			Rio_readinitb(&p->clientrio[i], connfd);
		
			/* Add the descriptor to descriptor set */
			FD_SET(connfd, &p->read_set);
	
			/* Update max descriptor and pool high water mark */
			if (connfd > p->maxfd)
				p->maxfd = connfd;
			if (i > p->maxi)
				p->maxi = i;
			break;
		}
	if (i == FD_SETSIZE) /* Couldn¡¯t find an empty slot */
		app_error("add_client error: Too many clients");
}

void check_clients(pool* p)
{
	int i, connfd, n;
	char buf[MAXLINE];
	rio_t rio;
	char request[20];
	int id, num;
	item_pointer temp;

	for (i = 0; (i <= p->maxi) && (p->nready > 0); i++) {
		connfd = p->clientfd[i];
		rio = p->clientrio[i];

			/* If the descriptor is ready, echo a text line from it */
		if ((connfd > 0) && (FD_ISSET(connfd, &p->ready_set))) {
			p->nready--;
			if ((n = Rio_readlineb(&rio, buf, MAXLINE)) != 0) {
				printf("%s", buf);
				if (!strcmp(buf, "exit\n"))
				{
					//store stock
					Close(connfd);
					FD_CLR(connfd, &p->read_set);
					p->clientfd[i] = -1;
					client_num--;
					printf("Connection is closed\n");
				}
				if (!strcmp(buf, "show\n"))
				{
					stock_temp = &cur_stock_info[0];
					inorder(root);
					printf("%s", cur_stock_info);
					Rio_writen(connfd, cur_stock_info, MAXLINE);
				}
				else
				{
					sscanf(buf, "%s %d %d", request, &id, &num);
					temp = search(root, id);

					if (!strcmp(request, "sell"))
					{
						temp->left_stock += num;
						Rio_writen(connfd, "[sell] success\n", MAXLINE);
					}
					if (!strcmp(request, "buy"))
					{
						if (temp->left_stock < num)
						{							
							Rio_writen(connfd, "Not enough left stock\n", MAXLINE);
						}
						else
						{
							temp->left_stock -= num;
							Rio_writen(connfd, "[buy] success\n", MAXLINE);
						}
					}
				}

			}
			/* EOF detected, remove descriptor from pool */
			else {
				Close(connfd);
				FD_CLR(connfd, &p->read_set);
				p->clientfd[i] = -1;
			}
		}
	}
}

item_pointer new_item(int id, int left_stock, int price) {
	item_pointer item_new = (item_pointer)malloc(sizeof(item));
	item_new->ID = id;
	item_new->left_stock = left_stock;
	item_new->price = price;
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

void inorder(item_pointer ptr)
{
	if (ptr)
	{
		inorder(ptr->left_child);	
		sprintf(stock_temp, "%d %d %d\n", ptr->ID, ptr->left_stock, ptr->price);
		stock_temp = stock_temp + strlen(stock_temp);
		inorder(ptr->right_child);
	}
}

void inorder2(item_pointer ptr)
{
	if (ptr)
	{
		inorder(ptr->left_child);	
		sprintf(stock_temp, "%d %d %d\n", ptr->ID, ptr->left_stock, ptr->price);
		stock_temp = stock_temp + strlen(stock_temp);
		inorder(ptr->right_child);
	}
}
/* $end echoserverimain */
