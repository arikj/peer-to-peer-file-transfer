#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <errno.h>
#include <string.h>
#include <sys/types.h>
#include <mysql.h>
#include <my_global.h>

int logged_in; 
char curr_user[100];
char ip[100], port[20];

void finish_with_error(MYSQL *con)
{
	fprintf(stderr, "%s\n", mysql_error(con));
	mysql_close(con);      
}

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


int register_user(char *buffer)
{
	MYSQL *conn;
	MYSQL_ROW row;
	int n;
	char *server = "localhost";
	char *user = "root";
	char *password = "root";
	char *database = "socket";

	char username[1024], passwd[1024], query[1024];

	conn = mysql_init(NULL);
	/* Connect to database */
	if (!mysql_real_connect(conn, server,
				user, password, database, 0, NULL, 0)) {
		fprintf(stderr, "%s\n", mysql_error(conn));
		exit(0);
	}
	n = query_check(buffer, username, 2);
	n = query_check(buffer, passwd, 3);

	snprintf(query, sizeof query, "INSERT INTO clients VALUES ('%s','%s', 0,'0','0')", username, passwd);
	//printf("query = %s\n", query);
	if (mysql_query(conn,query)) {
		finish_with_error(conn);
		n = 0;
	}

	else
	{
		n = 1;
		snprintf(query, sizeof query, "CREATE TABLE %s(filename TEXT, filepath TEXT)",username);
		if (mysql_query(conn, query)) {      
			finish_with_error(conn);
			n = 0;
		}
	}
	/* close connection */
	mysql_close(conn);
	return n;	
} 



int login_user(char *buffer, int connfd)
{
	MYSQL *conn;
	MYSQL_RES *res;
	MYSQL_ROW row;
	int n;
	char *server = "localhost";
	char *user = "root";
	char *password = "root";
	char *database = "socket";

	char username[1024], passwd[1024], query[1024];


	conn = mysql_init(NULL);
	/* Connect to database */
	if (!mysql_real_connect(conn, server,
				user, password, database, 0, NULL, 0)) {
		fprintf(stderr, "%s\n", mysql_error(conn));
		exit(0);
	}
	n = query_check(buffer, username, 2);
	n = query_check(buffer, passwd, 3);


	snprintf(query, sizeof query, "SELECT * FROM clients WHERE username = '%s' AND password = '%s'", username, passwd);
	//printf("query = %s\n", query);
	if (mysql_query(conn,query)) {
		finish_with_error(conn);
	}

	res = mysql_use_result(conn);
	if ((row = mysql_fetch_row(res)) != NULL)
	{
		logged_in = 1;
		strcpy(curr_user, username);	
	}

	mysql_free_result(res);	
	if(logged_in == 1)
	{
		snprintf(query, sizeof query, "UPDATE clients SET active = 1, ip = '%s', port = %s WHERE username = '%s'",ip,port,username);
		if(mysql_query(conn,query)){
			finish_with_error(conn);	
		}
	}

	else
		logged_in = 0;

	mysql_close(conn);
	return logged_in;	
} 


int share_file(char *buffer)
{
	MYSQL *conn;
	MYSQL_ROW row;
	int n;
	char *server = "localhost";
	char *user = "root";
	char *password = "root";
	char *database = "socket";

	char file_name[1024], file_path[1024], query[1024];

	conn = mysql_init(NULL);
	/* Connect to database */
	if (!mysql_real_connect(conn, server,
				user, password, database, 0, NULL, 0)) {
		fprintf(stderr, "%s\n", mysql_error(conn));
		exit(0);
	}
	n = query_check(buffer, file_name, 2);
	n = query_check(buffer, file_path, 3);

	snprintf(query, sizeof query, "INSERT INTO %s VALUES ('%s','%s')", curr_user, file_name, file_path);
	//printf("query = %s\n", query);
	if (mysql_query(conn,query)) {
		finish_with_error(conn);
		n = 0;
	}
	else
		n = 1;
	mysql_close(conn);

	return n;	
}

int search_file(char *buffer, char *final_list)
{
	MYSQL *conn;
	MYSQL_RES *res;
	MYSQL_ROW row, r;
	char *server = "localhost";
	char *user = "root";
	char *password = "root";
	char *database = "socket";
	char query[1024], file_name[1024];
	char temp[1024];
	char list[100][100];
	conn = mysql_init(NULL);
	int ret;
	/* Connect to database */
	if (!mysql_real_connect(conn, server,
				user, password, database, 0, NULL, 0)) {
		fprintf(stderr, "%s\n", mysql_error(conn));
		exit(0);
	}
	/* send SQL query */
	/*if (mysql_query(conn, "show tables")) {
	  fprintf(stderr, "%s\n", mysql_error(conn));
	  return 0;
	  }*/

	snprintf(query, sizeof query, "SELECT username, ip, port FROM clients WHERE active = 1");
	if (mysql_query(conn, query)) {
		fprintf(stderr, "%s\n", mysql_error(conn));
		return 0;
	}
	res = mysql_use_result(conn);
	char list2[100][100];
	ret = query_check(buffer, file_name, 2);
	int num=0,i,len=0;
	while ((row = mysql_fetch_row(res)) != NULL)
	{
		strcpy(list[num], row[0]);
		snprintf(list2[num], sizeof list2[num], "%s %s %s",row[0],row[1],row[2]);
		num++;
	}		
	final_list[0] = '\0';	
	char cat[1024];
	for(i=0;i<num;i++){
		if(strcmp(list[i], "clients")!=0 && strcmp(list[i], curr_user)!=0)
		{
			mysql_free_result(res);
			snprintf(query, sizeof query, "SELECT * FROM %s WHERE filename = '%s'",list[i], file_name);	

			if (mysql_query(conn,query)) {
				finish_with_error(conn);
				return 0;
			}

			res = mysql_use_result(conn);
			if ((r = mysql_fetch_row(res)) != NULL)
			{
				snprintf(cat, sizeof cat, "%s %s %s", list2[i], r[0], r[1]);	
				strcat(final_list, cat);
				len+=strlen(cat);
				final_list[len++]='\n';
				final_list[len]='\0';
			}

		}
	}

	/* close connection */
	mysql_free_result(res);
	mysql_close(conn);	
	return 1;
}

int send_mssg(int s, char *buf, int len)
{
	int total = 0;        // how many bytes we've sent
	int bytesleft = len; // how many we have left to send
	int n;

	while(total < len) {
		n = send(s, buf+total, bytesleft,0);
		if (n == -1) { printf("sendall error"); break; }
		total += n;
		bytesleft -= n;
	}
	len = total; // return number actually sent here

	return n==-1?-1:0; // return -1 on failure, 0 on success
}

int recv_mssg(int connfd, char *recvbuff, int size)
{
	int n;
	if ((n = recv(connfd, recvbuff, size,0))== -1) {
		perror("recv");
	}
	else if (n == 0) {
		printf("Connection closed\n");
		//So I can now wait for another client
	}
	recvbuff[n] = '\0';
	if(logged_in == 0)
		printf("New Client: %s\n", recvbuff);

	else
		printf("%s: %s\n",curr_user, recvbuff);
	return n;
}

void log_out()
{

	MYSQL *conn;
	MYSQL_RES *res;
	MYSQL_ROW row, r;
	char *server = "localhost";
	char *user = "root";
	char *password = "root";
	char *database = "socket";
	char query[1024];
	conn = mysql_init(NULL);
	if (!mysql_real_connect(conn, server,
				user, password, database, 0, NULL, 0)) {
		fprintf(stderr, "%s\n", mysql_error(conn));
		exit(0);
	}

	snprintf(query, sizeof query, "UPDATE clients SET active = 0 WHERE username = '%s'", curr_user);
	if(mysql_query(conn,query)){
		finish_with_error(conn);	
	}
	mysql_close(conn);	
}


int main(void)
{
	int listenfd = 0,connfd = 0, n=0;
	struct sockaddr_in serv_addr;
	struct sockaddr_in peer;
	logged_in = 0;
	char check[1024];
	char sendbuff[1024];
	char buffer[1024];
	char str1[1024], str2[1024];
	int numrv,size, result;
	int max_size = 1024; 
	int closed = 0; 
	int peer_len = sizeof(peer);
	pid_t pid;
	strcpy(str2,"Welcome to Online File Sharing service.\nPlease follow the instruction\n\n");
	listenfd = socket(AF_INET, SOCK_STREAM, 0);
	printf("SERVER successfully launched\n\n");

	memset(&serv_addr, '0', sizeof(serv_addr));
	memset(buffer, '0', sizeof(buffer));    
	serv_addr.sin_family = AF_INET;    
	serv_addr.sin_addr.s_addr = htonl(INADDR_ANY); 
	serv_addr.sin_port = htons(5000);    

	bind(listenfd, (struct sockaddr*)&serv_addr,sizeof(serv_addr));

	if(listen(listenfd, 10) == -1){
		printf("Failed to listen\n");
		return -1;
	}


	while(1)
	{
		if(closed == 1)
			break;

		connfd = accept(listenfd, (struct sockaddr*)NULL ,NULL); // accept awaiting request
		if ( (pid = fork()) == 0 ) {
			close(listenfd);

			sendbuff[0] = '\0'; 
			strcpy(sendbuff,"Connected with server\n");
			size = max_size;	
			sendbuff[22] = '\0';

			if((n=send_mssg(connfd,sendbuff,size))==-1)
			{
				close(connfd);
				break;	
			} 
			if((n=recv_mssg(connfd, buffer, size))==-1)
				exit(1);

			else if(n==0)
				break;			

			query_check(buffer, ip, 1);
			query_check(buffer, port, 2);
			printf("%s: %s", ip, port);					

			while(1){

				if(logged_in == 0) 
				{
					strcpy(str1,"1. New Client: Register (Syntax: \"register <username> <password>\")\n2. Existing Client: Login (Syntax: \"login <username> <password>\")\n3. For closing connection(Syntax: \"close\")\n\n");
					sendbuff[0] = '\0'; 
					strcat(sendbuff,str2);
					strcat(sendbuff,str1);
					size = max_size;	
					sendbuff[strlen(str1)+strlen(str2)-1] = '\0';

					if((n=send_mssg(connfd,sendbuff,size))==-1)
					{
						close(connfd);
						break;	
					} 

					if((n=recv_mssg(connfd, buffer, size))==-1)
						exit(1);

					else if(n==0)
						break;


					if(query_check(buffer, check, 1)<0)
						strcpy(str2,"No such query\n\n");		

					else
					{
						if(strcmp(check,"register")==0)
						{
							result = register_user(buffer);
							if(result == 1)
							{
								strcpy(str2, "Successfully Registered\n\n");
							}

							else
							{
								strcpy(str2, "Unable to Register. Please try again\n\n");

							}

						}

						else if(strcmp(check,"login")==0)
						{
							result = login_user(buffer,connfd);
							if(result == 1)
							{
								strcpy(str2, "Successfully Logged in\n\n");

							}


							else
							{
								strcpy(str2, "Unable to Login. Please try again or register\n\n");

							}
						}	


						else if(strcmp(check,"close")==0)
						{
							close(connfd);
							closed = 1;
							break;	
						}


						else
							strcpy(str2, "No such query\n\n");
					}

				}

				else
				{
					strcpy(str1,"1. Share File (Syntax: \"share <filename> <filepath>\")\n2. Search a file(Syntax: \"search <filename>\")\n3. Logout from server (Syntax: \"logout\")\n\n");
					sendbuff[0] = '\0'; 
					strcat(sendbuff,str2);
					strcat(sendbuff,str1);
					size = max_size;	
					sendbuff[strlen(str1)+strlen(str2)-1] = '\0';
					if((n=send_mssg(connfd,sendbuff,size))==-1)
					{
						close(connfd);
						break;	
					} 

					if((n=recv_mssg(connfd, buffer, size))==-1)
						exit(1);

					else if(n==0)
						break;	

					if(query_check(buffer, check, 1)<0)
						strcpy(str2,"No such query\n\n");		

					else
					{
						if(strcmp(check,"share")==0)
						{
							result = share_file(buffer);
							if(result == 1)
							{
								strcpy(str2, "File successfully shared\n\n");
							}

							else
							{
								strcpy(str2, "Unable to share the file. Please try again\n\n");

							}
						}

						else if(strcmp(check,"search")==0)
						{
							char final_list[1024];
							final_list[0] = '\0';
							result = search_file(buffer, final_list);
							if(result == 1)
							{
								str2[0]='\0';	
								strcat(str2, "Search complete: RESULT: \n\n");
								if(final_list[0]=='\0')
									strcat(str2, "No match Found\n\n");
								else
									strcat(str2, final_list);

							}

							else
							{
								strcpy(str2, "Search failed\n\n");

							}
						}	

						else if(strcmp(check,"logout")==0)
						{
							log_out();	
							logged_in = 0;
							str2[0]='\0';
						}

						else
							strcpy(str2, "No such query\n");
					}


				}

			}
			log_out();
			close(connfd);
			exit(0);
		}         
		close(connfd);

	}


	return 0;
}
