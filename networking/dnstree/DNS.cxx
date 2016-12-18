#include "DNS.h"
#include <string.h>
#include <arpa/inet.h>
#include <stdio.h>
#include "DNSDebug.h"

namespace
{
	const char c_Separator = '.';
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

        const RRHeader* RRHeaderFromBuffer(const char* buffer)
        {
            RRHeader* rrHeader = (RRHeader*)(buffer);
            rrHeader->Class = ::ntohs(rrHeader->Class);
            rrHeader->Type = ::ntohs(rrHeader->Type);
            rrHeader->TimeToLive = ::ntohl(rrHeader->TimeToLive);
            return rrHeader;
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
				*write = c_Separator;
				write++;
				bytesWritten++;
			}

			::memcpy(write, read + 1, length);
			bytesRead += length;
			bytesWritten += length;
			return true;
		}

        int ParseDomainNameHelper(const char* data, int cbData, int offsetOfDomainName, char* domainName, int& cbDomainName, bool firstCall)
		{
			const char c_PointerMask = 0xc0; // 11000000
			const char* read = data + offsetOfDomainName;
			char* write = domainName;
			while (true)
			{
				if (read - data > cbData)
				{
					return 0;
				}

				bool includeSeparator = (write != domainName);
				char length = *read;
				if ((length & c_PointerMask) == c_PointerMask)
				{
					read++;
					if (read - data > cbData)
					{
						return 0;
					}

					short offset = (length & ~c_PointerMask);
					offset = offset << 8;
					offset = offset + *read;
					read++;

                    if (firstCall && includeSeparator)
                    {
                        *write = c_Separator;
                        write++;
                        cbDomainName++;
                    }

                    if (ParseDomainNameHelper(data, cbData, offset, write, cbDomainName, false) == 0)
                    {
                        return 0;
                    }

                    break;
				}
				else
				{
					int bytesRead, bytesWritten;
					if (!CopyDomainNameSegment(read, cbData - (read - data), write, cbDomainName, includeSeparator, bytesRead, bytesWritten))
					{
						return 0;
					}

					read += bytesRead;
					write += bytesWritten;
					cbDomainName -= bytesWritten;

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
            return ParseDomainNameHelper(data, cbData, offsetOfDomainName, domainName, cbDomainName, true);
        }
	}
}
