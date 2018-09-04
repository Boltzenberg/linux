#include <stdio.h>
#include <string.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <errno.h>

int main(int argc, char **argv)
{
	int result = 0;
    struct sockaddr_in server;
    
    if (argc != 4)
    {
        printf("usage:  client <host> <port> <message>\n");
        return -1;
    }

    int port = atoi(argv[2]);
    
    server.sin_addr.s_addr = inet_addr(argv[1]);
    server.sin_family = AF_INET;
    server.sin_port = htons(port);

    int s = socket(AF_INET, SOCK_STREAM, 0);
    if (s != -1)
    {
		result = connect(s, (struct sockaddr *)&server, sizeof(server));
		if (result == 0)
		{
            ssize_t cb = send(s, argv[3], strlen(argv[3]), 0);
            if (cb == -1)
            {
                printf("Send failed: %d\n", errno);
            }

            close(s);
		}
		else
		{
			printf("Connection failed: %d\n", errno);
		}
	}

    return 0;
}
