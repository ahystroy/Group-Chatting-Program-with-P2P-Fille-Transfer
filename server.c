#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <errno.h>

#define SERV_IP "220.149.128.100"
#define SERV_PORT 4180
#define BACKLOG 10

#define USER1_IP "220.149.128.101"
#define USER1_PORT "4181"
#define USER2_IP "220.149.128.103"
#define USER2_PORT "4182"

#define BUF_SIZE 512
#define MSG_SIZE 512
#define ID_SIZE  20
#define PW_SIZE  20
#define IP_SIZE  16
#define PORT_SIZE 8

#define INIT_MSG2 " =================\n  AHY's Messenger\n  Please Login!!\n ================="
#define INIT_TITLE "\n>---------- Chatting Room ----------<\n"

#define USER1_ID "user1"
#define USER1_PW "passwd1"
#define USER2_ID "user2"
#define USER2_PW "passwd2"

char user1_ip[IP_SIZE];
char user1_port[PORT_SIZE];
char user2_ip[IP_SIZE];
char user2_port[PORT_SIZE];

int main(void)
{
	/* listen on sock_fd, new connection on new_fd */
	int sockfd, new_fd;

	/* my address information, address where I run this program */
	struct sockaddr_in my_addr;

	/* remote address information */
	struct sockaddr_in their_addr;
	unsigned int sin_size;

	/* buffer */
	int rcv_byte;
	char buf[BUF_SIZE]; // received msg buffer

	char id[ID_SIZE];
	char pw[PW_SIZE];
	char ip[IP_SIZE];
	char port[PORT_SIZE];

	char msg[MSG_SIZE];       // send msg
	char login_msg[MSG_SIZE]; // login result msg

	char user1_msg[MSG_SIZE]; // user1 message formatting
	char user2_msg[MSG_SIZE]; // user2 message formatting

	/* retval of fork() */
	pid_t pid;

	int val = 1;

	/* pipe : for communication between fork process */
	int pipe1[2], pipe2[2];
	pipe(pipe1); // send user1's msg to user2
	pipe(pipe2); // send user2's msg to user1

	/* log-in flag */
	int user1_flag = 0;
	int user2_flag = 0;
	int id_error = 0;

	/* socket */
	sockfd = socket(AF_INET, SOCK_STREAM, 0);
	if (sockfd == -1) {
		perror("Server-socket() error lol!");
		exit(1);
	}
//	else printf("Server-socket() sockfd is OK...\n");

	/* host byte order */
	my_addr.sin_family = AF_INET;

	/* short, network byte order */
	my_addr.sin_port = htons(SERV_PORT);
	// my_addr.sin_addr.s_addr = inet_addr(SERV_IP);
	my_addr.sin_addr.s_addr = INADDR_ANY;

	/* zero the rest of the struct */
	memset(&(my_addr.sin_zero), 0, 8);

	/* to prevent 'Address already in use...' */
	if (setsockopt(sockfd, SOL_SOCKET, SO_REUSEADDR, (char*)&val, sizeof(val)) < 0) {
		perror("setsockpt");
		close(sockfd);
		return -1;
	}

	/* bind */
	if (bind(sockfd, (struct sockaddr *)&my_addr, sizeof(struct sockaddr)) == -1)
	{
		perror("Server-bind() error lol!");
		exit(1);
	}
//	else printf("Server-bind() is OK...\n");

	/* listen */
	if (listen(sockfd, BACKLOG) == -1)
	{
		perror("listen() error lol!");
		exit(1);
	}
//	else printf("listen() is OK...\n\n");

	printf("%s", INIT_TITLE);

	/**** !Server Concurrency! ****/
	while (1)
	{	/* ...other codes to read the received data... */
		sin_size = sizeof(struct sockaddr_in);
		new_fd = accept(sockfd, (struct sockaddr *)&their_addr, &sin_size);
		//printf("accept() is OK...\n\n");

		/* send INIT_MSG */
		send(new_fd, INIT_MSG2, sizeof(INIT_MSG2)+1, 0);

		pid = fork();
		if (pid == 0) { /* child process */
			// communicate with new socket
			memset(buf, 0, 512);
			rcv_byte = recv(new_fd, buf, sizeof(buf)+1, 0);
			strcpy(id,  buf);
			memset(buf, 0, 512);
			rcv_byte = recv(new_fd, buf, sizeof(buf)+1, 0);
			strcpy(pw, buf);

			/* check id & pw */
			if (strcmp(id, USER1_ID) == 0) {
				if (strcmp(pw, USER1_PW) == 0) user1_flag = 1;
				else if (strcmp(pw, USER1_PW) != 0) user1_flag = -1;
			}
			else if (strcmp(id, USER2_ID) == 0) {
				if (strcmp(pw, USER2_PW) == 0) user2_flag = 1;
				else if (strcmp(pw, USER2_PW) != 0) user2_flag = -1;
			}
			else id_error = 1;

			/* recv IP & Port number */
			memset(ip, 0, IP_SIZE);
			rcv_byte = recv(new_fd, ip, sizeof(ip)+1, 0);
			memset(port, 0, PORT_SIZE);
			rcv_byte = recv(new_fd, port, sizeof(port)+1, 0);


			/* log-in error */
			if (user1_flag == -1) {
				sprintf(login_msg, " Log-in fail: Incorrect passwd..\n");
				send(new_fd, login_msg, sizeof(login_msg)+1, 0);
				printf(" !! user1 tried log-in, but passwd was wrong.. !!\n");
			}
			else if (user2_flag == -1) {
				sprintf(login_msg, " Log-in fail: Incorrect passwd..\n");
				send(new_fd, login_msg, sizeof(login_msg)+1, 0);
				printf(" !! user2 tried log-in, but passwd was wrong.. !!\n");
			}
			else if (id_error) {
				sprintf(login_msg, " Log-in fail: Unregistered ID..\n");
				send(new_fd, login_msg, sizeof(login_msg)+1, 0);
				printf(" !! Unknown user tried log-in,, !!\n");
			}
			

			/* log-in success */
			/* user1 process */
			else if (user1_flag) {
				sprintf(login_msg, " [user1]: Log-in Success!"); 
				send(new_fd, login_msg, sizeof(login_msg)+1, 0);
				sprintf(login_msg, "\n//-+- Welcome to AHY's Messenger -+-//\n");
				send(new_fd, login_msg, sizeof(login_msg)+1, 0);
				
				printf("     --------------------------- \n");
				printf("     ||    [user1] enter!!    || \n");

				memset(user1_ip, 0, IP_SIZE);
				strcpy(user1_ip, ip);
				memset(user1_port, 0, PORT_SIZE);
				strcpy(user1_port, port);

				printf("     ||  %s:%s || \n", user1_ip, user1_port);
				printf("     --------------------------- \n");

				/* user1 send & recv process */
				pid = fork();
				if (pid == 0) {
					/* send user2's msg to user1 */
					while (1) {
						if (read(pipe2[0], buf, BUF_SIZE) > 0) {
							if (!strcmp(buf, " >> [user2] [QUIT]")) {
								sprintf(buf, "     ||    [user2] exit...    ||");
								send(new_fd, buf, sizeof(buf)+1, 0);
								while(1);
							}
							else if (!strcmp(buf, " >> [user2] [FILE]")) {
								send(new_fd, buf, sizeof(buf) + 1, 0);

								// send user2 IP & port
								memset(buf, 0, 512);
								strcpy(buf, USER2_IP);
								strcat(buf, ":");
								strcat(buf, USER2_PORT);
								send(new_fd, buf, sizeof(buf) + 1, 0);
							}
							else send(new_fd, buf, sizeof(buf)+1, 0);
						}
					}
				}
				else if (pid > 0) {
					/* recv user1's msg */
					while (1) {
						memset(buf, 0, 512);
						rcv_byte = recv(new_fd, buf, sizeof(buf)+1, 0);
						
						memset(user1_msg, 0, 512);
						sprintf(user1_msg, " >> [user1] ");
						strcat(user1_msg, buf);
						if (!strcmp(user1_msg, " >> [user1] ")) continue;
						write(pipe1[1], user1_msg, BUF_SIZE);
						
						if (!strcmp(user1_msg, " >> [user1] [QUIT]")) {
							printf("     --------------------------- \n");
							printf("     ||    [user1] exit...    || \n");
							printf("     --------------------------- \n");
							close(new_fd);
							exit(0);
						}
						else if (!strcmp(user1_msg, " >> [user1] [FILE]")) {
							printf("  -+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+ \n");
							printf("  !! user1 requested user2's file !! \n");
							printf("  -+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+ \n");
						}
						else printf("%s\n", user1_msg);
					}
				}
				else { // fork error
					fprintf(stderr, "can't fork, error %d\n", errno);
					exit(1);
				}
			}

			/* user2 process */
			else if (user2_flag) {
				sprintf(login_msg, " [user2]: Log-in Success!");
				send(new_fd, login_msg, sizeof(login_msg)+1, 0);
				sprintf(login_msg, "\n//-+- Welcome to AHY's Messenger -+-//\n");
				send(new_fd, login_msg, sizeof(login_msg)+1, 0);

				printf("     --------------------------- \n");
				printf("     ||    [user2] enter!!    || \n");

				memset(user2_ip, 0, IP_SIZE);
				strcpy(user2_ip, ip);
				memset(user2_port, 0, PORT_SIZE);
				strcpy(user2_port, port);

				printf("     ||  %s:%s || \n", user2_ip, user2_port);
				printf("     --------------------------- \n");

				/* user2 send & recv process */
				pid = fork();
				if (pid == 0) {
					/* send user1's msg to user2 */
					while (1) {
						if (read(pipe1[0], buf, BUF_SIZE) > 0) {
							if (!strcmp(buf, " >> [user1] [QUIT]")) {
								sprintf(buf, "     ||    [user1] exit...    ||");
								send(new_fd, buf, sizeof(buf)+1, 0);
								while(1);
							}
							else if (!strcmp(buf, " >> [user1] [FILE]")) {
								// P2P file transfer process

								// 1. user1 send [FILE] to server
								// 2. Server send user1's IP,port to user2
								// 3. user2 -> user1
								// 4. user2 send 'txt file list' to user1
								// 5. user1 pick one and send to user2
								// 6. user2 transfer txt file that user1's pick
								
								send(new_fd, buf, sizeof(buf)+1, 0);
								
								// send user1 IP & port
								memset(buf, 0, 512);
								strcpy(buf, USER1_IP);
								strcat(buf, ":");
								strcat(buf, USER1_PORT);
								send(new_fd, buf, sizeof(buf)+1, 0);
							}
							else send(new_fd, buf, sizeof(buf)+1, 0);
						}
					}
				}
				else if (pid > 0) {
					/* recv user2's msg */
					while(1) {
						memset(buf, 0, 512);
						rcv_byte = recv(new_fd, buf, sizeof(buf)+1, 0);

						memset(user2_msg, 0, 512);
						sprintf(user2_msg, " >> [user2] ");
						strcat(user2_msg, buf);
						if (!strcmp(user2_msg, " >> [user2] ")) continue;
						write(pipe2[1], user2_msg, BUF_SIZE);

						if (!strcmp(user2_msg, " >> [user2] [QUIT]")) {
							printf("     --------------------------- \n");
							printf("     ||    [user2] exit...    || \n");
							printf("     --------------------------- \n");
							close(new_fd);
							exit(0);
						}
						else if (!strcmp(user2_msg, " >> [user2] [FILE]")) {
							printf("  -+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+ \n");
							printf("  !! user2 requested user1's file !! \n");
							printf("  -+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+ \n");
						}
						else printf("%s\n", user2_msg);
					}
				}
				else { // fork error
					fprintf(stderr, "can't fork, error %d\n", errno);
					exit(1);
				}
			}

			close(new_fd);
			exit(0);
		}
		else if (pid > 0) { /* parent process */
			// accept client with sockfd
			close(new_fd); // first of all, close new_fd
			// second, go to while(1)'s first line
			// and then, accept new client with sockfd
		}
		else { // fork error
			fprintf(stderr, "can't fork, error %d\n", errno);
			exit(1);
		}

	}//while(1)

	close(new_fd);
	close(sockfd);

	return 0;
}
