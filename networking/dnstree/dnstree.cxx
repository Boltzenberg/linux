#include <stdio.h>
#include <string.h>
#include <sys/socket.h>
#include <unistd.h>
#include <arpa/inet.h>
#include "DNS.h"
#include "DNSResponse.h"
#include "DNSDebug.h"

namespace
{
    const unsigned short g_headerId = 123;
    const char* c_defaultHost = "www.google.com";
    const unsigned short c_defaultQueryType = ApplicationLayer::DNS::RRType::A;
}

int ConstructDNSQuery(char* buffer, int cb, const char* domainName, const unsigned short type, unsigned int dest)
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
	int cbQuery = ApplicationLayer::DNS::AddQuestion(dnsQuery, cb - (dnsQuery - buffer), domainName, type, ApplicationLayer::DNS::RRClass::IN);
	char* end = dnsQuery + cbQuery;
	
	return end - buffer;
}

int PrintQuestion(const char* buffer, int cb, int questionOffset)
{
    char host[256];
    int cbHost = sizeof(host);
    int cbRead = ApplicationLayer::DNS::ParseDomainName(buffer, cb, questionOffset, host, cbHost);

    const ApplicationLayer::DNS::Question *question = ApplicationLayer::DNS::QuestionFromBuffer(buffer + questionOffset + cbRead);

    printf("Question:\n");
    printf(" Host: %.*s\n", cbHost, host);
    printf(" Type: %s\n", ApplicationLayer::DNS::RRType::ToString(question->QType));
    printf("Class: %s\n", ApplicationLayer::DNS::RRClass::ToString(question->QClass));
    printf("\n");
    
    return cbRead + 4;    
}

void PrintAData(const char* buffer, int cb, int dataOffset, int dataLength)
{
    sockaddr_in addr;
    addr.sin_addr.s_addr = *((long*)(buffer + dataOffset));
    printf("%s", ::inet_ntoa(addr.sin_addr));
}

void PrintNSData(const char* buffer, int cb, int dataOffset, int dataLength)
{
    char host[256];
    int cbHost = sizeof(host);
    int cbRead = ApplicationLayer::DNS::ParseDomainName(buffer, cb, dataOffset, host, cbHost);
    printf("%.*s", cbHost, host);
}

void PrintCNAMEData(const char* buffer, int cb, int dataOffset, int dataLength)
{
    char host[256];
    int cbHost = sizeof(host);
    int cbRead = ApplicationLayer::DNS::ParseDomainName(buffer, cb, dataOffset, host, cbHost);
    printf("%.*s", cbHost, host);
}

void PrintSOAData(const char* buffer, int cb, int dataOffset, int dataLength)
{
    char mname[256];
    int cbMName = sizeof(mname);
    int cbRead = ApplicationLayer::DNS::ParseDomainName(buffer, cb, dataOffset, mname, cbMName);

    char rname[256];
    int cbRName = sizeof(rname);
    cbRead += ApplicationLayer::DNS::ParseDomainName(buffer, cb, dataOffset + cbRead, rname, cbRName);

    const uint32_t* remaining = (const uint32_t*)(buffer + dataOffset + cbRead);
    uint32_t serial = ::ntohl(remaining[0]);
    uint32_t refresh = ::ntohl(remaining[1]);
    uint32_t retry = ::ntohl(remaining[2]);
    uint32_t expire = ::ntohl(remaining[3]);
    uint32_t minimumTTL = ::ntohl(remaining[4]);
    printf("MName: %.*s, RName: %.*s, Serial: %d, Refresh: %d, Retry: %d, Expire: %d, Minimum TTL: %d", cbMName, mname, cbRName, rname, serial, refresh, retry, expire, minimumTTL);
}

void PrintWKSData(const char* buffer, int cb, int dataOffset, int dataLength)
{
    DumpBufferAsHex(buffer + dataOffset, dataLength);
}

void PrintPTRData(const char* buffer, int cb, int dataOffset, int dataLength)
{
    char host[256];
    int cbHost = sizeof(host);
    int cbRead = ApplicationLayer::DNS::ParseDomainName(buffer, cb, dataOffset + sizeof(short), host, cbHost);
    printf("%.*s", cbHost, host);
}

void PrintHINFOData(const char* buffer, int cb, int dataOffset, int dataLength)
{
    DumpBufferAsHex(buffer + dataOffset, dataLength);
}

void PrintMINFOData(const char* buffer, int cb, int dataOffset, int dataLength)
{
    DumpBufferAsHex(buffer + dataOffset, dataLength);
}

void PrintMXData(const char* buffer, int cb, int dataOffset, int dataLength)
{
    unsigned short preference = *(unsigned short*)(buffer + dataOffset);

    char host[256];
    int cbHost = sizeof(host);
    int cbRead = ApplicationLayer::DNS::ParseDomainName(buffer, cb, dataOffset + sizeof(short), host, cbHost);
    printf("%.*s at preference %d", cbHost, host, preference);
}

void PrintTXTData(const char* buffer, int cb, int dataOffset, int dataLength)
{
    char txt[16 * 1024];
    int cbTxt = sizeof(txt);
    int cbRead = ApplicationLayer::DNS::ParseDomainName(buffer, cb, dataOffset, txt, cbTxt);
    printf("%.*s", cbTxt, txt);
}

void PrintDNSResourceRecord(const ApplicationLayer::DNS::DNSResponseRecord& record)
{
    char host[256];
    char data[256];
    int cbHost = record.CopyDomainName(host, sizeof(host));

    printf("       Host: %.*s\n", cbHost, host);
    printf("       Type: %s\n", ApplicationLayer::DNS::RRType::ToString(record.GetType()));
    printf("      Class: %s\n", ApplicationLayer::DNS::RRClass::ToString(record.GetClass()));
    printf("        TTL: %d\n", record.GetTimeToLive());
    printf("Data Length: %d\n", record.GetDataLength());
    if (record.GetType() == ApplicationLayer::DNS::RRType::A)
    {
        int address;
        record.CopyDataAsAddress(address);
        printf("       Data: %s\n", ::inet_ntop(AF_INET, &address, data, INET_ADDRSTRLEN));
    }
    else if (record.GetType() == ApplicationLayer::DNS::RRType::AAAA)
    {
        in6_addr address;
        record.CopyDataAsAddress(address);
        printf("       Data: %s\n", ::inet_ntop(AF_INET6, &address, data, INET6_ADDRSTRLEN));
    }
    else
    {
        int cbData = record.CopyDataAsString(data, sizeof(data));
        printf("       Data: %.*s\n", cbData, data);
    }
    
    printf("\n");
}

int PrintResourceRecord(const char* buffer, int cb, int answerOffset)
{
    char host[256];
    int cbHost = sizeof(host);
    int cbRead = ApplicationLayer::DNS::ParseDomainName(buffer, cb, answerOffset, host, cbHost);

    const char* read = buffer + answerOffset + cbRead;
    if (cb < cbRead + sizeof(ApplicationLayer::DNS::RRHeader) + sizeof(short))
    {
        return 0;
    }

    const ApplicationLayer::DNS::RRHeader* header = ApplicationLayer::DNS::RRHeaderFromBuffer(read);
    read += sizeof(ApplicationLayer::DNS::RRHeader);
    cbRead += sizeof(ApplicationLayer::DNS::RRHeader);
    
    if (cb < cbRead)
    {
        return 0;
    }

    const char* rData = read;

    read += header->DataLength;
    cbRead += header->DataLength;

    printf("       Host: %.*s\n", cbHost, host);
    printf("       Type: %s\n", ApplicationLayer::DNS::RRType::ToString(header->Type));
    printf("      Class: %s\n", ApplicationLayer::DNS::RRClass::ToString(header->Class));
    printf("        TTL: %d\n", header->TimeToLive);
    printf("Data Length: %d\n", header->DataLength);
    printf("       Data: ");

    switch (header->Type)
    {
        case ApplicationLayer::DNS::RRType::A: PrintAData(buffer, cb, (int)(rData - buffer), header->DataLength); break;
        case ApplicationLayer::DNS::RRType::NS: PrintNSData(buffer, cb, (int)(rData - buffer), header->DataLength); break;
        case ApplicationLayer::DNS::RRType::CNAME: PrintCNAMEData(buffer, cb, (int)(rData - buffer), header->DataLength); break;
        case ApplicationLayer::DNS::RRType::SOA: PrintSOAData(buffer, cb, (int)(rData - buffer), header->DataLength); break;
        case ApplicationLayer::DNS::RRType::WKS: PrintWKSData(buffer, cb, (int)(rData - buffer), header->DataLength); break;
        case ApplicationLayer::DNS::RRType::PTR: PrintPTRData(buffer, cb, (int)(rData - buffer), header->DataLength); break;
        case ApplicationLayer::DNS::RRType::HINFO: PrintHINFOData(buffer, cb, (int)(rData - buffer), header->DataLength); break;
        case ApplicationLayer::DNS::RRType::MINFO: PrintMINFOData(buffer, cb, (int)(rData - buffer), header->DataLength); break;
        case ApplicationLayer::DNS::RRType::MX: PrintMXData(buffer, cb, (int)(rData - buffer), header->DataLength); break;
        case ApplicationLayer::DNS::RRType::TXT: PrintTXTData(buffer, cb, (int)(rData - buffer), header->DataLength); break;
        default: printf("Unknown"); break;
    }

    printf("\n");
    printf("\n");
    
    return cbRead;
}

void PrintHeader(const ApplicationLayer::DNS::DNSHeader *dnsHeader)
{
    printf("Header:\n");
	printf("                   Id: %d\n", dnsHeader->Id);

	printf("     RecursionDesired: %d\n", dnsHeader->RecursionDesired);
	printf("           Truncation: %d\n", dnsHeader->Truncation);
	printf("  AuthoritativeAnswer: %d\n", dnsHeader->AuthoritativeAnswer);
	printf("               Opcode: %d\n", dnsHeader->Opcode);
	printf("           IsResponse: %d\n", dnsHeader->IsResponse);

	printf("         ResponseCode: %d\n", dnsHeader->ResponseCode);
	printf("     CheckingDisabled: %d\n", dnsHeader->CheckingDisabled);
	printf("    AuthenticatedData: %d\n", dnsHeader->AuthenticatedData);
	printf("             Reserved: %d\n", dnsHeader->Reserved);
	printf("   RecursionAvailable: %d\n", dnsHeader->RecursionAvailable);

	printf("        QuestionCount: %d\n", dnsHeader->QuestionCount);
	printf("          AnswerCount: %d\n", dnsHeader->AnswerCount);
	printf(" AuthorityRecordCount: %d\n", dnsHeader->AuthorityRecordCount);
	printf("AdditionalRecordCount: %d\n", dnsHeader->AdditionalRecordCount);
    printf("\n");
}

void PrintResponse2(const char* buffer, int cb)
{
    char host[256];
    ApplicationLayer::DNS::DNSQuestion question;
    ApplicationLayer::DNS::DNSResponseRecord record;
    ApplicationLayer::DNS::DNSResponse response(buffer, cb);
    const ApplicationLayer::DNS::DNSHeader *pHeader = response.GetHeader();
    PrintHeader(pHeader);

    if (response.InitializeDNSQuestions(question))
    {
        printf("Question:\n");
        do
        {
            int cbHost = question.CopyDomainName(host, sizeof(host));
            printf(" Host: %.*s\n", cbHost, host);
            printf(" Type: %s\n", ApplicationLayer::DNS::RRType::ToString(question.GetType()));
            printf("Class: %s\n", ApplicationLayer::DNS::RRClass::ToString(question.GetClass()));
            printf("\n");
        } while (question.NextQuestion());
    }
    
    if (response.InitializeDNSAnswers(record))
    {
        printf("Answers:\n");
        do
        {
            PrintDNSResourceRecord(record);
        } while(record.NextRecord());
    }

    if (response.InitializeDNSAuthorities(record))
    {
        printf("Authorities:\n");
        do
        {
            PrintDNSResourceRecord(record);
        } while(record.NextRecord());
    }
    
    if (response.InitializeDNSAdditionals(record))
    {
        printf("Additional Records:\n");
        do
        {
            PrintDNSResourceRecord(record);
        } while(record.NextRecord());
    }
}

void PrintResponse(const char* buffer, int cb)
{
	if (cb < sizeof(ApplicationLayer::DNS::DNSHeader))
	{
		printf("Didn't read enough data! %d\n", cb);
		return;
	}

	const ApplicationLayer::DNS::DNSHeader *dnsHeader = ApplicationLayer::DNS::DNSHeaderFromBuffer(buffer);
    PrintHeader(dnsHeader);
    
    const char* read = buffer + sizeof(ApplicationLayer::DNS::DNSHeader);
    int cbRemaining = cb - sizeof(ApplicationLayer::DNS::DNSHeader);
    
    for (int i = 0; i < dnsHeader->QuestionCount; i++)
    {
        int cbConsumed = PrintQuestion(buffer, cb, read - buffer);
        if (cbConsumed == 0)
        {
            return;
        }

        read += cbConsumed;
        cbRemaining -= cbConsumed;
    }
    printf("\n");

    printf("Answers:\n");
    for (int i = 0; i < dnsHeader->AnswerCount; i++)
    {
        int cbConsumed = PrintResourceRecord(buffer, cb, read - buffer);
        if (cbConsumed == 0)
        {
            return;
        }

        read += cbConsumed;
        cbRemaining -= cbConsumed;
    }
    printf("\n");
    
    printf("Authority Records:\n");
    for (int i = 0; i < dnsHeader->AuthorityRecordCount; i++)
    {
        int cbConsumed = PrintResourceRecord(buffer, cb, read - buffer);
        if (cbConsumed == 0)
        {
            return;
        }

        read += cbConsumed;
        cbRemaining -= cbConsumed;
    }
    printf("\n");
    
    printf("Additional Records:\n");
    for (int i = 0; i < dnsHeader->AdditionalRecordCount; i++)
    {
        int cbConsumed = PrintResourceRecord(buffer, cb, read - buffer);
        if (cbConsumed == 0)
        {
            return;
        }

        read += cbConsumed;
        cbRemaining -= cbConsumed;
    }
    printf("\n");
    
    DumpBufferAsHex(read, cbRemaining);
    printf("\n");
}

const char* GetHostToResolve(int argc, char **argv)
{
    if (argc < 2)
    {
        return c_defaultHost;
    }

    return argv[1];
}

const unsigned short GetRecordTypeToQuery(int argc, char **argv)
{
    unsigned short type = c_defaultQueryType;
    if ((argc < 3) || !ApplicationLayer::DNS::RRType::TryParse(argv[2], type))
    {
        type = c_defaultQueryType;
    }

    return type;
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
	
	const char* pszHost = GetHostToResolve(argc, argv);
    const unsigned short type = GetRecordTypeToQuery(argc, argv);
    printf("Querying '%s' for record of type '%s'\n", pszHost, ApplicationLayer::DNS::RRType::ToString(type));
    
	char data[1500];
	size_t cbData = sizeof(data);
	
	server.sin_addr.s_addr = inet_addr("8.8.8.8");
	server.sin_family = AF_INET;
	server.sin_port = htons(53);
	
	char* packet = data;
	int cbPacket = ConstructDNSQuery(packet, cbData, pszHost, type, server.sin_addr.s_addr);
	
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
	PrintResponse2(data, cbRead);
	
	close(s);
	
	return 0;
}

