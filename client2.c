#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <errno.h>
#include <dirent.h>

#define SERV_IP 	"220.149.128.100"
#define SERV_PORT 	4180

#define USER2_IP 	"220.149.128.103"
#define USER2_PORT  4182
#define _USER2_PORT	"4182"
#define BACKLOG  10

#define BUF_SIZE 512
#define MSG_SIZE 512
#define ID_SIZE  20
#define PW_SIZE  20

#define MAX_FILE 30

int main(int argc, char *argv[])
{
	int sockfd, p2p_fd, p2p_sockfd, new_fd; /* will hold the destination addr */
	struct sockaddr_in dest_addr;
	struct sockaddr_in dest_addr2;
	struct sockaddr_in my_addr;
	struct sockaddr_in their_addr;
	unsigned int sin_size;

	int val = 1;

	/* buffer */
	int rcv_byte;
	char buf[BUF_SIZE];
	char msg[MSG_SIZE];

	char id[ID_SIZE];
	char pw[PW_SIZE];

	char p2p_ip[30];
	char p2p_port[30];

	pid_t pid;
	
	// file
	DIR *dir;
	struct dirent *ent;
	dir = opendir("./");

	FILE *read_fp;
	FILE* fp;

	/* create socket */
	sockfd = socket(AF_INET, SOCK_STREAM, 0);

	if (sockfd == -1)
	{
		perror("Client-socket() error lol!");
		exit(1);
	}
//	else printf("Client-socket() sockfd is OK...\n");


	/* host byte order */
	dest_addr.sin_family = AF_INET;

	/* short, network byte order */
	dest_addr.sin_port = htons(SERV_PORT);
	dest_addr.sin_addr.s_addr = inet_addr(SERV_IP);

	/* zero the rest of the struct */
	memset(&(dest_addr.sin_zero), 0, 8);


	/* connect */
	if (connect(sockfd, (struct sockaddr *)&dest_addr, sizeof(struct sockaddr)) == -1)
	{
		perror("Client-connect() error lol");
		exit(1);
	}
//	else printf("Client-connect() is OK...\n\n");

	/* receive INIT_MSG */
	rcv_byte = recv(sockfd, buf, sizeof(buf)+1, 0);
	printf("%s\n\n", buf);

	/* input ID,PW & send ID,PW */
	printf(" << ID: ");
	scanf("%s", id);
	send(sockfd, id, strlen(id)+1, 0);

	printf(" << PW: ");
	scanf("%s", pw);
	send(sockfd, pw, strlen(pw)+1, 0);

	/* send IP & Port number */
	send(sockfd, USER2_IP, sizeof(USER2_IP)+1, 0);
	send(sockfd, _USER2_PORT, sizeof(_USER2_PORT)+1, 0);

	/* receive log-in result */
	memset(buf, 0, 512);
	rcv_byte = recv(sockfd, buf, sizeof(buf)+1, 0);
	printf("\n%s", buf);


	/* if login success, enter chatting room */
	if (!strcmp(buf, " [user1]: Log-in Success!") 
	 || !strcmp(buf, " [user2]: Log-in Success!")) {
		memset(buf, 0, 512);
		rcv_byte = recv(sockfd, buf, sizeof(buf)+1, 0);
		printf("\n%s\n", buf);

		printf(">---------- Chatting Room ----------<\n");

		pid = fork();
		if (pid == 0) {
			/* send process */
			while (1) {
				fgets(msg, sizeof(msg), stdin);
				for (int i=0; i<MSG_SIZE; i++) {
					if (msg[i] == '\n') {
						msg[i] = '\0';
						break;
					}
				}
				send(sockfd, msg, strlen(msg)+1, 0);
				if (!strcmp(msg, "[QUIT]")) {
					close(sockfd);
					printf("\n || Bye-bye, Please Enter ctrl+c ");
					exit(0);
				}
				else if (!strcmp(msg, "[FILE]")) {
					// user2 server open
					p2p_fd = socket(AF_INET, SOCK_STREAM, 0);

					my_addr.sin_family = AF_INET;
					my_addr.sin_port = htons(USER2_PORT);
					my_addr.sin_addr.s_addr = INADDR_ANY;

					memset(&(my_addr.sin_zero), 0, 8);

					if (setsockopt(p2p_fd, SOL_SOCKET, SO_REUSEADDR, (char*)&val, sizeof(val)) < 0) {
						perror("setsockpt");
						close(p2p_fd);
						return -1;
					}
					if (bind(p2p_fd, (struct sockaddr*)&my_addr, sizeof(struct sockaddr)) == -1) {
						perror("Server-bind() error lol!");
						exit(1);
					} //else printf("bind ok...\n");
					if (listen(p2p_fd, BACKLOG) == -1) {
						perror("listen() error lol!");
						exit(1);
					} //else printf("listen ok...\n");
					sin_size = sizeof(struct sockaddr_in);
					new_fd = accept(p2p_fd, (struct sockaddr*)&their_addr, &sin_size);

					int n;
					char* fileName;
					// recv file List
					memset(buf, 0, 512);
					rcv_byte = recv(new_fd, buf, sizeof(buf)+1, 0);

					int fileCount = 0;
					char choose[2];
					fileName = strtok(buf, "/");
					while (fileName != NULL) {
						printf(" %d. %s\n", fileCount+1, fileName);
						fileName = strtok(NULL, "/");
						fileCount++;
					}

					// choose file
					printf("Please enter number: ");
					scanf("%s", choose);
					send(new_fd, choose, sizeof(choose) + 1, 0);

					// recv file name
					memset(buf, 0, 512);
					rcv_byte = recv(new_fd, buf, sizeof(buf) + 1, 0);
					fp = fopen(buf, "w");

					// recv file txt
					memset(buf, 0, 512);
					rcv_byte = recv(new_fd, buf, sizeof(buf) + 1, 0);
					fputs(buf, fp);

					printf("\n  -+-+-+-+-+-+-+-+-+-+-+-+-+- \n");
					printf("  ||  file recv success!!  || \n");
					printf("  -+-+-+-+-+-+-+-+-+-+-+-+-+- \n\n");

					fclose(fp);
				}
			}
		}
		else if (pid > 0) {
			/* recv process */
			while (1) {
				memset(buf, 0, 512);
				rcv_byte = recv(sockfd, buf, sizeof(buf)+1, 0);
				if (!strcmp(buf, " >> [user1] [FILE]")) {
					printf(" >> [user1's file request]\n");
					
					// recv user1's IP & port
					memset(buf, 0, 512);
					rcv_byte = recv(sockfd, buf, sizeof(buf)+1, 0);
					
					strncpy(p2p_ip, buf, 15);
					p2p_ip[15] = '\0';

					for (int i=0; i<4; i++) {
						p2p_port[i] = buf[i+16];
					}
					p2p_port[4] = '\0';
					
					// user2 -> user1
					p2p_sockfd = socket(AF_INET, SOCK_STREAM, 0);
					if (p2p_sockfd == -1) {
						perror("socket() error lol!");
						exit(1);
					}
					//else printf("socket() is ok...\n");
					
					dest_addr2.sin_family = AF_INET;
					int p2p_port_ = atoi(p2p_port);
					dest_addr2.sin_port = htons(p2p_port_);
					dest_addr2.sin_addr.s_addr = inet_addr(p2p_ip);

					memset(&(dest_addr2.sin_zero), 0, 8);

					if (connect(p2p_sockfd, (struct sockaddr*)&dest_addr2, sizeof(struct sockaddr)) == -1) {
						perror("connect() error lol\n");
						exit(1);
					}

					/////////////////////////
					// send user2's file list
					char fileList[512];
					char fileList2[MAX_FILE][512];
					int choosen;
					memset(fileList, 0, 512);
					if (dir != NULL) {
						int len;
						int fileCount=0;
						while ((ent = readdir(dir)) != NULL) {
							len = strlen(ent->d_name);
							if (ent->d_name[len-4] == '.' &&
								ent->d_name[len-3] == 't' && 
								ent->d_name[len-2] == 'x' && 
								ent->d_name[len-1] == 't') {
								// txt file
								strcpy(fileList2[fileCount], ent->d_name);
								sprintf(buf, "%s/", ent->d_name);
								strcat(fileList, buf);
								fileCount++;
							}
						}
						send(p2p_sockfd, fileList, sizeof(fileList)+1, 0);

				
						memset(buf, 0, 512);
						rcv_byte = recv(p2p_sockfd, buf, sizeof(buf)+1, 0);
						choosen = atoi(buf);
						printf(" >> user1's pick file is [%s]\n", fileList2[choosen-1]);
						
						// send file name
						send(p2p_sockfd, fileList2[choosen-1], sizeof(fileList2[choosen-1])+1, 0);
						
						// send file txt
						char fileSendBuf[512];
						memset(fileSendBuf, 0, 512);
						read_fp = fopen(fileList2[choosen-1], "r");
						if (read_fp == NULL)
							printf("failed to open file..\n");
						memset(buf, 0, 512);
						while (fgets(buf, sizeof(buf)+1, read_fp) != NULL) {
							strcat(fileSendBuf, buf);
							memset(buf, 0, 512);
						}
						send(p2p_sockfd, fileSendBuf, sizeof(fileSendBuf)+1, 0);

						printf("\n  --------------------------- \n");
						printf("  ||  transfer success!!!  || \n");
						printf("  --------------------------- \n\n");
						
						fclose(read_fp);

						closedir(dir);

						close(p2p_sockfd);
					}
					else {
						perror("");
						return EXIT_FAILURE;
					}
				}
				else printf("%s\n", buf);
			}
		}
		else { // fork error
			fprintf(stderr, "can't fork, error %d\n", errno);
			exit(1);
		}
	}


	else { // login fail...
		memset(buf, 0, 512);
		rcv_byte = recv(sockfd, buf, sizeof(buf)+1, 0);
		printf("%s\n", buf);
	}

	close(sockfd);

	return 0;
}
