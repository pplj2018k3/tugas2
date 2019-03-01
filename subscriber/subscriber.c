// Write CPP code here 
#include <arpa/inet.h> 
#include <netinet/in.h> 
#include <unistd.h> 
#include <netdb.h> 
#include <stdio.h> 
#include <stdlib.h> 
#include <string.h> 
#include <sys/socket.h> 
#include <json-c/json.h>

#define MAX_STR 32
#define PORT 1349
#define SA struct sockaddr 

// Procedure & Function
void toJson(char** data, char* out);


int main(int argc, char *argv[]) { 
	if (argc < 2){
        fprintf(stderr, "Usage: %s <ip address> [--raw]\n", argv[0]);
        exit(1);
    }
	int rawdata = 0;
	for (int i=0; i<argc; i++){
		if (strcmp(argv[i], "--raw") == 0){
			rawdata = 1;
			break;
		}
	}
	// printf("rawdata=%d", rawdata);

	int sockfd, connfd; 
	struct sockaddr_in servaddr, cli; 

	// socket create and varification 
	sockfd = socket(AF_INET, SOCK_STREAM, 0); 
	if (sockfd == -1) { 
		printf("socket creation failed...\n"); 
		exit(0); 
	} 
	else
		printf("Socket successfully created..\n"); 
	bzero(&servaddr, sizeof(servaddr)); 

	// assign IP, PORT 
	servaddr.sin_family = AF_INET; 
	servaddr.sin_addr.s_addr = inet_addr(argv[1]); 
	servaddr.sin_port = htons(PORT); 

	// connect the client socket to server socket 
	if (connect(sockfd, (SA*)&servaddr, sizeof(servaddr)) != 0) { 
		printf("connection with the server failed...\n"); 
		exit(0); 
	} 
	else
		printf("connected to the server..\n"); 

	printf("Nama Client: ");
	char nama[MAX_STR];
	scanf("%s", nama);
	printf("Topic: ");
	char topic[MAX_STR];
	scanf("%s", topic);
	char *data[4] = { "sub", nama, topic, "init"};
	char buff[1024];
	// proses inisiasi
	toJson(data, buff);
	send(sockfd, buff, sizeof(buff), 0);
	char buffin[1024] = {0};
	int len = 0;
	while((len = recv(sockfd, buffin, 1024, 0)) > 0){
		if (rawdata)
			printf("%s\n", buffin);
		else {
			struct json_object *parsed_json;
			struct json_object *who_json;
			struct json_object *content_json;

			parsed_json = json_tokener_parse(buffin);
			json_object_object_get_ex(parsed_json, "who", &who_json);
			json_object_object_get_ex(parsed_json, "content", &content_json);
			printf("Data From:%s\n-> %s\n", json_object_get_string(who_json), json_object_get_string(content_json));
		}
	}
	// close the socket 
	close(sockfd); 
} 

void toJson(char** data, char* out){
	const int l = 4;
    char * key[4] = {"command", "who", "topic", "content"};
    char json_str[1024];
    strcpy(json_str, "{");
    int x = 1;
    for (int i=0; i<l; i++){
        char temp[1024];
        sprintf(temp, "\"%s\":\"%s\"", key[i], data[i]);
        strcpy(json_str+x, temp);
        x+=5+strlen(key[i])+strlen(data[i]);
        if (i!=l-1){
            strcpy(json_str+x, ",");
            x++;
        }
    }
    strcpy(json_str+x, "}\n");
    strcpy(out, json_str);
}