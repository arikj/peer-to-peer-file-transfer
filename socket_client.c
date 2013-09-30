#include <sys/socket.h>
#include <sys/types.h>
#include <netinet/in.h>
#include <netdb.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include <errno.h>
#include <arpa/inet.h>




int port;
char ip[100], username[1024];
#define CHUNK_SIZE 10


int query_check(char *query, char *check, int portion)
{
	int i=0,j, curr = portion;

	while(portion>0)
	{
		j=0;	
		while(query[i]!= ' ' && query[i]!='\n' && i<strlen(query))
		{
			check[j] = query[i];
			i++;
			j++;
		}
		portion--;
		i++;		
	}		
	check[j]='\0';

	if(i>strlen(query))
		return -1;	// fail

	else
		return 0; // success
}


int recv_mssg(int connfd, char *recvbuff, int size){
	int n=0,tot=0, max=1024;
	char str[1024];
	recvbuff[0]='\0';
	while(tot<max)
	{
		if ((n = recv(connfd, str, size,0))== -1) {
			perror("recv");
			return n;
		}
		else if (n == 0) {
			printf("Connection closed\n");
			return n;
			//So I can now wait for another client
		}
		tot+=n;
		strcat(recvbuff,str);
	}  	
	recvbuff[tot-1] = '\0';        
	return n;

}


void download(char *recvBuffer, int sockfd, char *filename)
{
	int n;
	char sendbuff[1024];
	if (((n=recv_mssg(sockfd,recvBuffer,strlen(recvBuffer)))== -1)) {
		fprintf(stderr, "Failure receiving Message\n");
		close(sockfd);
		exit(1);
	}

	else if (n == 0) {
		printf("Connection closed\n");
	}
	else {
		FILE *fOut = fopen (filename, "w+");
		if (fOut != NULL) 
			fputs (recvBuffer, fOut);
		printf("Files successfully downloaded.....\n");
		fclose (fOut); 
		exit(0);

	}
}


int main(int argc, char * argv [])
{
	if(argc != 3)
	{
		printf("Wrong arguments!!!\n");
		exit(0);	
	}

	int sockfd = 0,listenfd=0,connfd=0,n = 0;
	char recvBuffer[1024];
	char sendBuff[1024], curr_query[1024];;
	int size = 1024, ret;
	struct sockaddr_in serv_addr;
	int bytetosend, byte_sent;
	int max_size = 1024,pid;
	memset(recvBuffer, '0' ,sizeof(recvBuffer));
	memset(sendBuff, '0' ,sizeof(sendBuff));
	if((sockfd = socket(AF_INET, SOCK_STREAM, 0))< 0)
	{
		printf("\n Error : Could not create socket \n");
		return 1;
	}

	serv_addr.sin_family = AF_INET;
	serv_addr.sin_port = htons(atoi(argv[2]));
	serv_addr.sin_addr.s_addr = inet_addr(argv[1]);

	if(connect(sockfd, (struct sockaddr *)&serv_addr, sizeof(serv_addr))<0)
	{
		printf("\n Error : Connect Failed!! \n");
		return 1;
	}

	struct sockaddr_in s_addr;
	struct sockaddr_in sin;
	listenfd = socket(AF_INET, SOCK_STREAM, 0);

	socklen_t len = sizeof(sin);
	s_addr.sin_family = AF_INET;    
	s_addr.sin_addr.s_addr = inet_addr(argv[1]); 
	s_addr.sin_port = htons(0); 

	bind(listenfd, (struct sockaddr*)&s_addr,sizeof(s_addr));

	if (getsockname(listenfd, (struct sockaddr *)&sin, &len) == -1)
		perror("getsockname");

	else{
		strcpy(ip,inet_ntoa(sin.sin_addr));
		port = (int)ntohs(sin.sin_port);
		printf("Your address: %s:%d\n",ip,port);
	}

	if ( (pid = fork()) == 0 )									// for handling peer file sharing
	{
		close(sockfd);	

		char sendbuff[1024];

		if(listen(listenfd, 10) == -1){
			printf("Failed to listen\n");
			return -1;
		}

		while(1)
		{
			connfd = accept(listenfd, (struct sockaddr*)NULL ,NULL); // accept awaiting request	
			if ( (pid = fork()) == 0 ) {
				close(listenfd);
				while(1){
					sendbuff[0] = '\0';
					strcpy(sendbuff, "Welcome Peer!!!\n For downloading a file (Syntax: \"download <filename> <filepath> to <filepath>\")\n");
					if((n=send(connfd,sendbuff,size,0))==-1)
					{
						close(connfd);
						break;	
					}

					if ((n=recv_mssg(connfd,recvBuffer,strlen(recvBuffer)))== -1) {
						fprintf(stderr, "Failure receiving Message\n");
						close(sockfd);
						exit(1);
					}


					else if (n == 0) {
						printf("Connection closed\n");

					}
					else
					{
						char name[100], path[100];
						int n;
						n = query_check(recvBuffer, name, 2);
						n = query_check(recvBuffer, path, 3);	

						FILE *file;
						char *file_data;
						file = fopen(path, "rb");
						int fileLen = 1024, sent;
						//Allocate memory
						file_data=(char *)malloc(fileLen+1);

						//Read file contents into buffer	
						fread(file_data, fileLen, 1, file);
						fclose(file);

						sent = send(connfd, file_data, fileLen, 0);
						exit(0);
					}		 
				}
			}
		}
	}

	else{	

		if(atoi(argv[2]) == 5000){	
			if ((n=recv_mssg(sockfd,recvBuffer,strlen(recvBuffer)))== -1) {
				fprintf(stderr, "Failure receiving Message\n");
				close(sockfd);
				exit(1);
			}


			else if (n == 0) {
				printf("Connection closed\n");

			}
			else 
				printf("%s\n",recvBuffer);

			sendBuff[0]='\0';														// for handling server
			snprintf(sendBuff, sizeof sendBuff, "%s %d", ip, port);
			n = send(sockfd, sendBuff, sizeof(sendBuff), 0);

			if ( n < 0 )
			{
				printf("Either Connection Closed or Error\n");
			}
		}	
		while(1){
			if (((n=recv_mssg(sockfd,recvBuffer,strlen(recvBuffer)))== -1)) {
				fprintf(stderr, "Failure receiving Message\n");
				close(sockfd);
				exit(1);
			}

			else if (n == 0) {
				printf("Connection closed\n");
				break;
			}
			else {
				printf("%s",recvBuffer);
				fgets(sendBuff,size,stdin);
				n = send(sockfd, sendBuff, sizeof(sendBuff),0);


				if ( n < 0 )
				{
					printf("Either Connection Closed or Error\n");
					//Break from the While
					break;
				}

				ret = query_check(sendBuff, curr_query, 1);
				if(strcmp(curr_query, "download")==0)
				{	
					ret = query_check(sendBuff, curr_query, 4);
					download(sendBuff, sockfd, curr_query);

				}

			}      
		}
	}

	return 0;
}
