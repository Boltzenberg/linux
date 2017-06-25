#include <stdio.h>
#include <string.h>
#include <sys/socket.h>
#include <unistd.h>
#include <arpa/inet.h>

int main(int argc, char **argv)
{
    while (1)
    {
    	int s = socket(AF_INET, SOCK_STREAM, 6);
    	struct sockaddr_in server;
    	char* message;
    	char reply[16 * 1024];

        server.sin_addr.s_addr = inet_addr("13.107.245.94");
        server.sin_family = AF_INET;
        server.sin_port = htons(80);

        message = "GET /id HTTP/1.1\r\nHost: failhttp.afdorigin-test.com\r\n\r\n";

        bzero(reply, sizeof(reply));
    	
    	if (s != -1)
    	{
            int connectResult = connect(s, (struct sockaddr *)&server, sizeof(server));
            printf("connectResult: %d\n", connectResult);
            if (connectResult >= 0)
            {
                int sendResult = send(s, message, strlen(message), 0);
                printf("sendResult: %d\n", sendResult);
                if (sendResult >= 0)
                {
                    int recvResult = recv(s, reply, sizeof(reply), 0);
                    printf("recvResult: %d\n", recvResult);
                    if (recvResult >= 0)
                    {
                        printf("received:  %s\n\n", reply);
                    }
                    else
                    {
                        printf("Recv failed\n");
                    }
                }
                else
                {
                    printf("Send failed\n");
                }
            }
            else
            {
                printf("Connection error: %d\n", connectResult);
            }
            
            close(s);
    	}
        else
    	{
    		printf("Could not create socket.\n");
    	}
    	
        sleep(1);
    }
	
	return 0;
}

