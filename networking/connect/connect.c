#include <stdio.h>
#include <string.h>
#include <sys/socket.h>
#include <unistd.h>
#include <arpa/inet.h>

int main(int argc, char **argv)
{
	int s = socket(AF_INET, SOCK_STREAM, 6);
	if (s != -1)
	{
        unsigned char ipString[100];
        
        //Get the IP address to connect to from the terminal
        printf("Enter the IP address to connect to: ");
        scanf("%s" , ipString);

        struct sockaddr_in server;
        
        server.sin_addr.s_addr = inet_addr(ipString);
        server.sin_family = AF_INET;
        server.sin_port = htons(80);

        printf("Connecting to %s\n", ipString);
        int connectResult = connect(s, (struct sockaddr *)&server, sizeof(server));
        printf("connectResult: %d\n", connectResult);
        close(s);
	}
    else
	{
		printf("Could not create socket.\n");
	}
	
	return 0;
}

