#include "DNS.h"
#include <string.h>
#include <arpa/inet.h>

namespace
{
	const char c_Separator = '.';
}

namespace ApplicationLayer
{
	namespace DNS
	{
		int AddQuestion(char* data, int cbData, const char* name, unsigned short queryType, unsigned short queryClass)
		{
			int cbDomainName = WriteDomainName(name, ::strlen(name), data, cbData);
			if (cbDomainName == 0 || cbData < cbDomainName + 4)
			{
				return 0;
			}
			
			unsigned short* remainingFields = (unsigned short*)(data + cbDomainName);
			remainingFields[0] = ::htons(queryType);
			remainingFields[1] = ::htons(queryClass);
			return cbDomainName + 4;
		}
		
		int WriteDomainName(const char* domainName, int cbDomainName, char* data, int cbData)
		{
			int offset = 0;
			char* write = data;
			int cbWrite = cbData;
			
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
			
			*write = 0;
			cbWrite--;
			
			return cbData - cbWrite;
		}
		
		int ParseDomainName(char* data, int cbData, int offsetOfDomainName, char* domainName, int& cbDomainName)
		{
			return 0;
		}
	}
}
