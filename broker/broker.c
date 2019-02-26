#include <stdio.h> 
#include <stdlib.h> 
#include <string.h> //strlen 
#include <errno.h> 
#include <unistd.h> //close 
#include <arpa/inet.h> //close 
#include <netinet/in.h> 
#include <sys/types.h> 
#include <sys/socket.h> 
#include <sys/time.h> //FD_SET, FD_ISSET, FD_ZERO macros 

#include <json-c/json.h> // json library

#define TRUE 1 
#define FALSE 0 
#define SA struct sockaddr	
#define PORT 1349 

#define SERVER_OK 		"200" 
#define SENSOR_OK 		"400" 
#define SENSOR_INIT		"401"
#define SENSOR_PUBLISH	"402"
#define CLIENT_OK 		"600" 
#define CLIENT_INIT 	"601"
#define CLIENT_GET 		"602"

#define MAX_STR 32

struct client
{
	char id[MAX_STR];
	int sockfd;
	char topic[MAX_STR];
	int condition; 
	// 0 = kosong
	// 1 = idle
	// 
};

// Procedure & Function
void addtoList(struct client *in,struct client *out);
int isTopicEquals(char *topic, struct client *id_in);
void deleteList(int port);
int getIndexList();

int main(int argc , char *argv[]) 
{ 
	int opt = TRUE; 
	int master_socket , addrlen , new_socket , client_socket[10] , 
		max_clients = 10 , activity, i , valread , sd; 
	int max_sd; 
	struct sockaddr_in address; 
		
	char buffer[1024]; //data buffer of 1K 
		
	//set of socket descriptors 
	fd_set readfds; 
			
	//initialise all client_socket[] to 0 so not checked 
	for(i = 0; i < max_clients; i++){ 
		client_socket[i] = 0; 
	} 
		
	//create a master socket 
	if( (master_socket = socket(AF_INET , SOCK_STREAM , 0)) == 0){ 
		perror("socket failed"); 
		exit(EXIT_FAILURE); 
	} 
	
	//set master socket to allow multiple connections , 
	//this is just a good habit, it will work without this 
	if( setsockopt(master_socket, SOL_SOCKET, SO_REUSEADDR, (char *)&opt, sizeof(opt)) < 0 ){ 
		perror("setsockopt"); 
		exit(EXIT_FAILURE); 
	} 
	
	//type of socket created 
	address.sin_family = AF_INET; 
	address.sin_addr.s_addr = INADDR_ANY; 
	address.sin_port = htons( PORT ); 
		
	//bind the socket to localhost port 8888 
	if (bind(master_socket, (SA *)&address, sizeof(address))<0){ 
		perror("bind failed"); 
		exit(EXIT_FAILURE); 
	} 
	printf("Listener on port %d \n", PORT); 
		
	//try to specify maximum of 5 pending connections for the master socket 
	if (listen(master_socket, 5) < 0){ 
		perror("listen"); 
		exit(EXIT_FAILURE); 
	} 
		
	//accept the incoming connection 
	addrlen = sizeof(address); 
	puts("Waiting for connections ...\n"); 

	struct client list_sensor[5];
	struct client list_client[5];
	
	for (int x=0; x<5; x++){
		list_sensor[x].condition = 0;
		list_client[x].condition = 0;
	}

	while(TRUE){ 
		//clear the socket set 
		FD_ZERO(&readfds); 
	
		//add master socket to set 
		FD_SET(master_socket, &readfds); 
		max_sd = master_socket; 
			
		//add child sockets to set 
		for ( i = 0 ; i < max_clients ; i++){ 
			//socket descriptor 
			sd = client_socket[i]; 
				
			//if valid socket descriptor then add to read list 
			if(sd > 0) 
				FD_SET( sd , &readfds); 
				
			//highest file descriptor number, need it for the select function 
			if(sd > max_sd) 
				max_sd = sd; 
		} 
	
		//wait for an activity on one of the sockets , timeout is NULL , 
		//so wait indefinitely 
		activity = select( max_sd + 1 , &readfds , NULL , NULL , NULL); 
	
		if ((activity < 0) && (errno!=EINTR)){ 
			printf("select error"); 
		} 
			
		//If something happened on the master socket , 
		//then its an incoming connection 
		if (FD_ISSET(master_socket, &readfds)){ 
			if ((new_socket = accept(master_socket, 
					(SA *)&address, (socklen_t*)&addrlen))<0){ 
				perror("accept"); 
				exit(EXIT_FAILURE); 
			} 
			
			//inform user of socket number - used in send and receive commands 
			printf("New connection , socket fd is %d , ip is : %s , port : %d\n", 
            new_socket, inet_ntoa(address.sin_addr), ntohs(address.sin_port)); 
				
			//add new socket to array of sockets 
			for (i = 0; i < max_clients; i++){ 
				//if position is empty 
				if( client_socket[i] == 0 ){ 
					client_socket[i] = new_socket; 
					printf("Adding to list of sockets as %d\n" , i); 
						
					break; 
				} 
			} 
		} 
			
		//else its some IO operation on some other socket 
		for (i = 0; i < max_clients; i++){ 
			sd = client_socket[i]; 
				
			if (FD_ISSET( sd , &readfds)){ 
				//Check if it was for closing , and also read the 
				//incoming message 
				if ((valread = read( sd , buffer, 1024)) == 0){ 
					//Somebody disconnected , get his details and print 
					getpeername(sd , (SA*)&address , (socklen_t*)&addrlen); 
					printf("Host disconnected , ip %s , port %d \n" , 
						inet_ntoa(address.sin_addr) , ntohs(address.sin_port)); 
						
					// Close the socket and mark as 0 in list for reuse
					close( sd ); 
					client_socket[i] = 0; 
				} 
				// Process data yang masuk
				else { 
					// sd, inet_ntoa(address.sin_addr), ntohs(address.sin_port); 
					struct json_object *parsed_json;
					struct json_object *cmd;
					struct json_object *id_client;
					struct json_object *topic_json;
					struct json_object *content_json;

					parsed_json = json_tokener_parse(buffer);

					json_object_object_get_ex(parsed_json, "command", &cmd);
					json_object_object_get_ex(parsed_json, "who", &id_client);
					json_object_object_get_ex(parsed_json, "topic", &topic_json);
					json_object_object_get_ex(parsed_json, "content", &content_json);
					
					char command[MAX_STR]; 
					char client_id[MAX_STR];
					char topic[MAX_STR];
					char content[MAX_STR];
					sprintf(command, "%s", json_object_get_string(cmd));
					sprintf(client_id, "%s", json_object_get_string(id_client));
					sprintf(topic, "%s", json_object_get_string(topic_json));
					sprintf(content, "%s", json_object_get_string(content_json));

					struct client tmp;
					sprintf(tmp.id, "%s", client_id);
					sprintf(tmp.topic, "%s", topic);
					tmp.sockfd = sd;
					if (strcmp(command, "pub") == 0){
						addtoList(&tmp, list_sensor);
					} else if (strcmp(command, "sub") == 0){
						addtoList(&tmp, list_client);
						char *temp2 = "Test From Broker";
						send(sd , temp2 , strlen(temp2) , 0 );						
					}

					printf("From: %s\nCommand: %s\n", client_id, command);
					printf("-> \"%s\"\t\tin: %s\n", content, topic);
					//send(sd , temp2 , strlen(temp2) , 0 );
				} 
			} 
		} 
	} 
		
	return 0; 
} 

/*/
struct client
{
	char id[MAX_STR];
	int sockfd;
	char topic[MAX_STR];
	int condition; 
	// 0 = kosong
	// 1 = idle
	// 
};//*/

void addtoList(struct client *in, struct client *out){
	int i;
	int dapat=0;
	for(i=0; i<5; i++){
		if (out->condition == 0){
			sprintf(out->id, "%s", in->id);
			out->sockfd = in->sockfd;
			sprintf(out->topic, "%s", in->topic);
			out->condition = 1;
			dapat=1;
			break;
		}
	}
	if (!dapat){
		printf("List penuh %d\n", i);
	} else {
		printf("Data masuk dalam List %d\n", i);
	}
}

void deleteList(int port){

}

int isTopicEquals(char *topic, struct client *id_in){
	if (strcmp(id_in->topic, topic) != 0){
		return 0;
	} else {
		return 1;
	}
	
}