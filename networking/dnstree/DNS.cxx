#include "DNS.h"
#include "DNSResponse.h"
#include <string.h>
#include <arpa/inet.h>
#include <stdio.h>
#include "DNSDebug.h"

namespace
{
	const char c_Separator = '.';
    const char c_PointerMask = 0xc0; // 11000000
    const bool c_debugTraceEnabled = false;
}

namespace ApplicationLayer
{
	namespace DNS
	{
        const DNSHeader* DNSHeaderFromBuffer(const char* buffer)
        {
            DNSHeader *dnsHeader = (DNSHeader*)(buffer);
            dnsHeader->QuestionCount = ::ntohs(dnsHeader->QuestionCount);
            dnsHeader->AnswerCount = ::ntohs(dnsHeader->AnswerCount);
            dnsHeader->AuthorityRecordCount = ::ntohs(dnsHeader->AuthorityRecordCount);
            dnsHeader->AdditionalRecordCount = ::ntohs(dnsHeader->AdditionalRecordCount);
            return dnsHeader;
        }

        const Question* QuestionFromBuffer(const char* buffer)
        {
            Question *question = (Question*)(buffer);
            question->QType = ::ntohs(question->QType);
            question->QClass = ::ntohs(question->QClass);
            return question;
        }

        unsigned short RRTypeFromHeader(const char* buffer)
        {
            RRHeader* rrHeader = (RRHeader*)(buffer);
            return ::ntohs(rrHeader->Type);
        }
        
        const RRHeader* RRHeaderFromBuffer(const char* buffer)
        {
            RRHeader* rrHeader = (RRHeader*)(buffer);
            rrHeader->Type = ::ntohs(rrHeader->Type);
            rrHeader->Class = ::ntohs(rrHeader->Class);
            rrHeader->TimeToLive = ::ntohl(rrHeader->TimeToLive);
            rrHeader->DataLength = ::ntohs(rrHeader->DataLength);
            return rrHeader;
        }

        const OPTRRHeader* OPTRRHeaderFromBuffer(const char* buffer)
        {
            OPTRRHeader* header = (OPTRRHeader*)(buffer);
            header->Type = ::ntohs(header->Type);
            header->UDPPayloadSize = ::ntohs(header->UDPPayloadSize);
            return header;
        }

		int AddQuestion(char* data, int cbData, const char* name, unsigned short rrType, unsigned short rrClass)
		{
			int cbDomainName = WriteDomainName(name, ::strlen(name), data, cbData);
			if (cbDomainName == 0 || cbData < cbDomainName + 4)
			{
				return 0;
			}
			
			unsigned short* remainingFields = (unsigned short*)(data + cbDomainName);
			remainingFields[0] = ::htons(rrType);
			remainingFields[1] = ::htons(rrClass);
			return cbDomainName + 4;
		}

        int AddOPTRR(char* data, int cbData, unsigned short udpPayloadSize, unsigned char extendedRCode, unsigned char version, bool dnssecOK)
        {
            int cbDomainName = WriteDomainName(".", 1, data, cbData);
            if (cbData < cbDomainName + sizeof(OPTRRHeader))
            {
                return 0;
            }

            OPTRRHeader* header = (OPTRRHeader*)(data + cbDomainName);
            header->Type = ::htons(RRType::OPT);
            header->UDPPayloadSize = ::htons(udpPayloadSize);
            header->ExtendedRCode = extendedRCode;
            header->Version = version;
            header->DO = dnssecOK;
            header->Zero = 0;
            header->DataLength = 0;

            return cbDomainName + sizeof(OPTRRHeader);
        }

        // Turns www.google.com into 3www6google3com
		int WriteDomainName(const char* domainName, int cbDomainName, char* data, int cbData)
		{
			int offset = 0;
			char* write = data;
			int cbWrite = cbData;
            int lastLength = 0;

			while (offset < cbDomainName)
			{
				// find the next separator
				int nextSeparator = offset;
				while (nextSeparator < cbDomainName && domainName[nextSeparator] != c_Separator)
				{
					nextSeparator++;
				}
				
				char length = nextSeparator - offset;
				if (cbWrite < length + 1)
				{
					return 0;
				}
				
                lastLength = length;
				*write = length;
				::memcpy(write + 1, &domainName[offset], length);
				write += length + 1;
				cbWrite -= (length + 1);
				offset = nextSeparator + 1;
			}
			
			if (cbWrite < 1)
			{
				return 0;
			}

            if (lastLength != 0)
            {
    			*write = 0;
    			cbWrite--;
            }
            
			return cbData - cbWrite;
		}
		
		bool CopyDomainNameSegment(const char* read, int cbRead, char* write, int cbWrite, bool includeSeparator, int& bytesRead, int& bytesWritten)
		{
			bytesRead = 1;
			bytesWritten = 0;
			char length = *read;
			if (length == 0)
			{
                if (c_debugTraceEnabled)
                {
                    printf("1. terminating\n");
                }
                *write = '\0';
                bytesWritten++;
				return true;
			}

			if ((length > cbRead) || (length > cbWrite))
			{
				return false;
			}

			if (includeSeparator)
			{
                if (c_debugTraceEnabled)
                {
                    printf("2. separator\n");
                }
				*write = c_Separator;
				write++;
				bytesWritten++;
			}

            if (c_debugTraceEnabled)
            {
                printf("3. %.*s\n", length, read + 1);
            }
			::memcpy(write, read + 1, length);
			bytesRead += length;
			bytesWritten += length;
			return true;
		}

        int ParseDomainNameHelper(const char* data, int cbData, int offsetOfDomainName, char* domainName, int& cbDomainName, bool firstCall, bool wroteAnything)
		{
			const char* read = data + offsetOfDomainName;
			char* write = domainName;

			while (true)
			{
				if (read - data > cbData)
				{
					return 0;
				}

				char length = *read;
				if ((length & c_PointerMask) == c_PointerMask)
				{
                    if (c_debugTraceEnabled)
                    {
                        DumpBufferAsHex(read, 2);
                        printf("\n");
                    }
                    
					read++;
					if (read - data > cbData)
					{
						return 0;
					}

					unsigned short offset = (length & ~c_PointerMask);
					offset = offset << 8;
					offset = offset + (unsigned char)*read;
					read++;

                    if (c_debugTraceEnabled)
                    {
                        printf("Jumping to offset %d.\n", offset);
                    }
                    
                    if (ParseDomainNameHelper(data, cbData, offset, write, cbDomainName, false, wroteAnything) == 0)
                    {
                        return 0;
                    }

                    break;
				}
				else
				{
					int bytesRead, bytesWritten;
					if (!CopyDomainNameSegment(read, cbData - (read - data), write, cbDomainName, wroteAnything, bytesRead, bytesWritten))
					{
						return 0;
					}

					read += bytesRead;
					write += bytesWritten;
					cbDomainName -= bytesWritten;
                    wroteAnything = true;

                    if (length == 0)
                    {
                        break;
                    }                    
				}
			}

			return (read - data) - offsetOfDomainName;
		}
        
        int ParseDomainName(const char* data, int cbData, int offsetOfDomainName, char* domainName, int& cbDomainName)
        {
            return ParseDomainNameHelper(data, cbData, offsetOfDomainName, domainName, cbDomainName, true, false);
        }

        int CbDomainName(const char* data, const int cbData)
        {
            const char* read = data;

            while (true)
            {
                if (read - data > cbData)
                {
                    return 0;
                }

                char length = *read;
                if (length == 0)
                {
                    // Read the 0 length
                    read += 1;
                    break;
                }                
                else if ((length & c_PointerMask) == c_PointerMask)
                {
                    // remove the pointer mask and combine with the next byte
                    // to get the jump target, but that's it for this host in the buffer
                    read += 2;
                    break;
                }
                else
                {
                    // The length character plus the value as the count of letters in the next section
                    read += length + 1;
                }
            }
        
            return (read - data);
        }
	}
}
