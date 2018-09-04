#include <stdio.h>
#include <string.h>
#include <sys/socket.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <errno.h>

int main(int argc, char **argv)
{
    int result = 0;
    struct sockaddr_in serverAddr;

    if (argc != 2)
    {
        printf("usage:  server <port>\n");
        return -1;
    }

    int server = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
    if (server != -1)
    {
		int enable = 1;
		result = setsockopt(server, SOL_SOCKET, SO_REUSEPORT, &enable, sizeof(int));
		if (result == 0)
		{
			int port = atoi(argv[1]);
			printf("Listening on port %d\n", port);

			memset(&serverAddr, 0, sizeof(serverAddr));
			serverAddr.sin_family = AF_INET;
			serverAddr.sin_addr.s_addr = htonl(INADDR_ANY);
			serverAddr.sin_port = htons(port);

			result = bind(server, (struct sockaddr *)&serverAddr, sizeof(serverAddr));
			if (result == 0)
			{
				result = listen(server, 8);
				if (result == 0)
				{
                    while (1)
                    {
    					struct sockaddr_in clientAddr;
    					socklen_t len = sizeof(clientAddr);
    					int client = accept(server, (struct sockaddr *)&clientAddr, &len);
    					if (client != -1)
    					{
                            char buf[4096];
                            ssize_t cb = recv(client, buf, sizeof(buf), 0);
                            if (cb < 1)
                            {
                                if (cb == -1)
                                {
                                    printf("recv failed: %d\n", errno);
                                }
                                break;
                            }

                            buf[cb] = 0;
                            printf("%s\n", buf);
                            close(client);
                        }
                        else
                        {
                            printf("accept failed: %d\n", errno);
                            break;
                        }
					}
				}
				else
				{
					printf("listen failed: %d\n", errno);
				}
			}
			else
			{
				printf("bind failed: %d\n", errno);
			}
		}
		else
		{
			printf("setsocopt failed: %d\n", errno);
		}
        
        close(server);
    }

    return 0;
}
