#include <stdio.h>
#include <string.h>
#include <sys/socket.h>
#include <unistd.h>
#include <arpa/inet.h>
#include "DNS.h"

unsigned short g_headerId = 123;

int ConstructDNSQuery(char* buffer, int cb, const char* domainName, unsigned int dest)
{
	::memset(buffer, -1, cb);
	
	ApplicationLayer::DNS::DNSHeader *dnsHeader = (ApplicationLayer::DNS::DNSHeader*)(buffer);
	dnsHeader->Id = g_headerId;

	dnsHeader->RecursionDesired = 1;
	dnsHeader->Truncation = 0;
	dnsHeader->AuthoritativeAnswer = 0;
	dnsHeader->Opcode = 0; // standard query
	dnsHeader->IsResponse = 0;

	dnsHeader->ResponseCode = 0;
	dnsHeader->CheckingDisabled = 0;
	dnsHeader->AuthenticatedData = 0;
	dnsHeader->Reserved = 0;
	dnsHeader->RecursionAvailable = 0;

	dnsHeader->QuestionCount = ::htons(1);
	dnsHeader->AnswerCount = ::htons(0);
	dnsHeader->AuthorityRecordCount = ::htons(0);
	dnsHeader->AdditionalRecordCount = ::htons(0);
	
	char* dnsQuery = (char*)(dnsHeader + 1);
	int cbQuery = ApplicationLayer::DNS::AddQuestion(dnsQuery, cb - (dnsQuery - buffer), domainName, 1, 1); // 1 == A, 1 == IN
	char* end = dnsQuery + cbQuery;
	
	return end - buffer;
}

void PrintResponse(const char* buffer, int cb)
{
	if (cb < sizeof(ApplicationLayer::DNS::DNSHeader))
	{
		printf("Didn't read enough data! %d\n", cb);
		return;
	}
	
	ApplicationLayer::DNS::DNSHeader *dnsHeader = (ApplicationLayer::DNS::DNSHeader*)(buffer);
	printf("Id (should be %d): %d\n", dnsHeader->Id, g_headerId);

	printf("RecursionDesired (should be 1): %d\n", dnsHeader->RecursionDesired);
	printf("Truncation (should be 0): %d\n", dnsHeader->Truncation);
	printf("AuthoritativeAnswer (should be 0): %d\n", dnsHeader->AuthoritativeAnswer);
	printf("Opcode (should be 0): %d\n", dnsHeader->Opcode);
	printf("IsResponse (should be 1): %d\n", dnsHeader->IsResponse);

	printf("ResponseCode (should be 0): %d\n", dnsHeader->ResponseCode);
	printf("CheckingDisabled (should be 0): %d\n", dnsHeader->CheckingDisabled);
	printf("AuthenticatedData (should be 0): %d\n", dnsHeader->AuthenticatedData);
	printf("Reserved (should be 0): %d\n", dnsHeader->Reserved);
	printf("RecursionAvailable (should be 1): %d\n", dnsHeader->RecursionAvailable);

	printf("QuestionCount (should be 1): %d\n", ::ntohs(dnsHeader->QuestionCount));
	printf("AnswerCount (should be 2): %d\n", ::ntohs(dnsHeader->AnswerCount));
	printf("AuthorityRecordCount (should be 0): %d\n", ::ntohs(dnsHeader->AuthorityRecordCount));
	printf("AdditionalRecordCount (should be 0): %d\n", ::ntohs(dnsHeader->AdditionalRecordCount));
}

int main(int argc, char **argv)
{
	int s = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP);
	struct sockaddr_in server;
	socklen_t cbServer = sizeof(server);
	
	if (s == -1)
	{
		printf("Could not create socket.\n");
	}
	
	const char* pszHost = "www.cnn.com";
	char data[1500];
	size_t cbData = sizeof(data);
	
	server.sin_addr.s_addr = inet_addr("8.8.8.8");
	server.sin_family = AF_INET;
	server.sin_port = htons(53);
	
	char* packet = data;
	int cbPacket = ConstructDNSQuery(packet, cbData, pszHost, server.sin_addr.s_addr);
	
	if (sendto(s, packet, cbPacket, 0, (struct sockaddr*)&server, cbServer) < 0)
	{
		printf("Send failed\n");
		return 1;
	}
	
	printf("Data sent!\n");
	
	::memset(data, 0, sizeof(data));

	int cbRead = recvfrom(s, data, sizeof(data), 0, (struct sockaddr*)&server, &cbServer);
	if (cbRead < 0)
	{
		printf("Recv failed\n");
		return 1;
	}
	
	printf("Reply received!\n");
	PrintResponse(data, cbRead);
	
	close(s);
	
	return 0;
}

