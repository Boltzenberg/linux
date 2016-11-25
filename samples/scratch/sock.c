#include <stdio.h>
#include <string.h>
#include <sys/socket.h>
#include <unistd.h>
#include <arpa/inet.h>

int main(int argc, char **argv)
{
	int s = socket(AF_INET, SOCK_STREAM, IPPROTO_IP);
	struct sockaddr_in server;
	char* message;
	char reply[16 * 1024];
	
	if (s == -1)
	{
		printf("Could not create socket.\n");
	}
	
	server.sin_addr.s_addr = inet_addr("13.107.21.200");
	server.sin_family = AF_INET;
	server.sin_port = htons(80);
	
	if (connect(s, (struct sockaddr *)&server, sizeof(server)) < 0)
	{
		printf("Connection error\n");
		return 1;
	}
	
	printf("Connected!\n");
	
	message = "GET /fdv2/diagnostics.aspx HTTP/1.1\r\nHost: www.bing.com\r\n\r\n";
	if (send(s, message, strlen(message), 0) < 0)
	{
		printf("Send failed\n");
		return 1;
	}
	
	printf("Data sent!\n");
	
	if (recv(s, reply, sizeof(reply), 0) < 0)
	{
		printf("Recv failed\n");
		return 1;
	}
	
	printf("Reply received!\n");
	printf("%s\n", reply);
	
	close(s);
	
	return 0;
}

