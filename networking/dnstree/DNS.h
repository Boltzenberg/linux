#ifndef _DNS_H_
#define _DNS_H_

namespace ApplicationLayer
{
	namespace DNS
	{
        // https://tools.ietf.org/html/rfc1035
		struct DNSHeader
		{
			unsigned short Id;
			
			unsigned short RecursionDesired : 1;
			unsigned short Truncation : 1;
			unsigned short AuthoritativeAnswer : 1;
			unsigned short Opcode : 4;
			unsigned short IsResponse : 1;
			
			unsigned short ResponseCode : 4;
			unsigned short CheckingDisabled : 1;
			unsigned short AuthenticatedData : 1;
			unsigned short Reserved : 1;
			unsigned short RecursionAvailable : 1;

			unsigned short QuestionCount;
			unsigned short AnswerCount;
			unsigned short AuthorityRecordCount;
			unsigned short AdditionalRecordCount;
		};
        
        struct Question
        {
            unsigned short QType;
            unsigned short QClass;
        };

        #pragma pack(push, 1)
        struct RRHeader
        {
            unsigned short Type;
            unsigned short Class;
            unsigned int TimeToLive;
            unsigned short DataLength;
        };
        #pragma pack(pop)

        class RRType
        {
        public:
            static const unsigned short A = 1; //a host address
            static const unsigned short NS = 2; // an authoritative name server
            static const unsigned short CNAME = 5; // the canonical name for an alias
            static const unsigned short SOA = 6; // marks the start of a zone of authority
            static const unsigned short WKS = 11; // a well known service description
            static const unsigned short PTR = 12; // a domain name pointer
            static const unsigned short HINFO = 13; // host information
            static const unsigned short MINFO = 14; // mailbox or mail list information
            static const unsigned short MX = 15; // mail exchange
            static const unsigned short TXT = 16; // text strings
            static const unsigned short AAAA = 28; // aaaa host address

            static const char* ToString(const unsigned short type)
            {
                switch (type)
                {
                    case A: return "A";
                    case NS: return "NS";
                    case CNAME: return "CNAME";
                    case SOA: return "SOA";
                    case WKS: return "WKS";
                    case PTR: return "PTR";
                    case HINFO: return "HINFO";
                    case MINFO: return "MINFO";
                    case MX: return "MX";
                    case TXT: return "TXT";
                    case AAAA: return "AAAA";
                    default: return "Unknown";
                }
            }
        };

        class RRClass
        {
        public:
            static const unsigned short IN = 1; // the Internet
            static const unsigned short CH = 3; // the CHAOS class
            static const unsigned short HS = 4; // Hesiod [Dyer 87]

            static const char* ToString(const unsigned short cls)
            {
                switch (cls)
                {
                    case IN: return "IN";
                    case CH: return "CH";
                    case HS: return "HS";
                    default: return "Unknown";
                }
            }
        };

        const DNSHeader* DNSHeaderFromBuffer(const char* buffer);
        const Question* QuestionFromBuffer(const char* buffer);
        const RRHeader* RRHeaderFromBuffer(const char* buffer);
		int AddQuestion(char* data, int cbData, const char* name, unsigned short rrType, unsigned short rrClass);
		int WriteDomainName(const char* domainName, int cbDomainName, char* data, int cbData);
		int ParseDomainName(const char* data, int cbData, int offsetOfDomainName, char* domainName, int& cbDomainName);
        int CbDomainName(const char* data, const int cbData);
	}
}

#endif // _DNS_H_
