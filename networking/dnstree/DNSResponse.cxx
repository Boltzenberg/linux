#include <string.h>
#include <arpa/inet.h>
#include <stdio.h>
#include "DNSDebug.h"
#include "DNSResponse.h"

namespace ApplicationLayer
{
	namespace DNS
	{
        DNSResponse::DNSResponse(const char* buffer, const int cb)
            : m_buffer(buffer),
              m_cb(cb),
              m_header(NULL),
              m_questionsOffset(0),
              m_answersOffset(0),
              m_authoritiesOffset(0),
              m_additionalsOffset(0)
        {
            m_header = DNSHeaderFromBuffer(m_buffer);
            
            const char* read = buffer + sizeof(DNSHeader);
            m_questionsOffset = read - buffer;
            for (int i = 0; i < m_header->QuestionCount; i++)
            {
                // Read all of the questions to advance to the beginning of the answers
                read += CbDomainName(read, cb - (read - buffer));
                const Question* question = QuestionFromBuffer(read);
                read += sizeof(Question);
            }

            m_answersOffset = read - buffer;
            for (int i = 0; i < m_header->AnswerCount; i++)
            {
                // Read all of the answers to advance to the beginning of the authorities
                read += CbDomainName(read, cb - (read - buffer));
                const RRHeader* header = RRHeaderFromBuffer(read);
                read += sizeof(RRHeader) + header->DataLength;
            }

            m_authoritiesOffset = read - buffer;
            for (int i = 0; i < m_header->AuthorityRecordCount; i++)
            {
                // Read all of the authorities to advance to the beginning of the additional headers
                read += CbDomainName(read, cb - (read - buffer));
                const RRHeader* header = RRHeaderFromBuffer(read);
                read += sizeof(RRHeader) + header->DataLength;
            }

            m_additionalsOffset = read - buffer;
            for (int i = 0; i < m_header->AdditionalRecordCount; i++)
            {
                // Read all of the authorities to advance to the beginning of the additional headers
                read += CbDomainName(read, cb - (read - buffer));
                const RRHeader* header = RRHeaderFromBuffer(read);
                read += sizeof(RRHeader) + header->DataLength;
            }
        }

        DNSResponse::~DNSResponse()
        {
        }

        const DNSHeader* DNSResponse::GetHeader() const
        {
            return m_header;
        }

        bool DNSResponse::InitializeDNSQuestions(DNSQuestion& question) const
        {
            if (m_header->QuestionCount > 0)
            {
                question.Initialize(m_buffer, m_cb, m_questionsOffset, m_header->QuestionCount);
                return true;
            }

            return false;
        }

        bool DNSResponse::InitializeDNSAnswers(DNSResponseRecord& record) const
        {
            if (m_header->AnswerCount > 0)
            {
                record.Initialize(m_buffer, m_cb, m_answersOffset, m_header->AnswerCount);
                return true;
            }

            return false;
        }
        
        bool DNSResponse::InitializeDNSAuthorities(DNSResponseRecord& record) const
        {
            if (m_header->AuthorityRecordCount> 0)
            {
                record.Initialize(m_buffer, m_cb, m_authoritiesOffset, m_header->AuthorityRecordCount);
                return true;
            }
            
            return false;
        }
            
        bool DNSResponse::InitializeDNSAdditionals(DNSResponseRecord& record) const
        {
            if (m_header->AdditionalRecordCount > 0)
            {
                record.Initialize(m_buffer, m_cb, m_additionalsOffset, m_header->AdditionalRecordCount);
                return true;
            }
            
            return false;
        }        

        DNSQuestion::DNSQuestion()
            : m_buffer(NULL),
              m_cb(0),
              m_offset(0),
              m_cbQuestion(0),
              m_remainingQuestionCount(0),
              m_question(NULL)
        { }
        
        DNSQuestion::~DNSQuestion()
        { }
        
        void DNSQuestion::Initialize(const char* buffer, const int cb, const int startOffset, const int questionCount)
        {
            m_buffer = buffer;
            m_cb = cb;
            m_offset = startOffset;
            m_remainingQuestionCount = questionCount - 1;
            int cbDomain = CbDomainName(buffer + startOffset, cb - startOffset);
            m_question = (Question*)(buffer + startOffset + cbDomain);
            m_cbQuestion = cbDomain + sizeof(Question);
        }
        
        bool DNSQuestion::NextQuestion()
        {
            if (m_remainingQuestionCount > 0)
            {
                Initialize(m_buffer, m_cb, m_offset + m_cbQuestion, m_remainingQuestionCount);
                return true;
            }

            return false;
        }
        
        const int DNSQuestion::CopyDomainName(char* domain, int cbDomain) const
        {
            int cbDomainName = cbDomain;
            ParseDomainName(m_buffer, m_cb, m_offset, domain, cbDomainName);
            return cbDomainName;
        }
        
        const unsigned short DNSQuestion::GetType() const
        {
            return m_question->QType;
        }
        
        const unsigned short DNSQuestion::GetClass() const
        {
            return m_question->QClass;
        }

        
        DNSResponseRecord::DNSResponseRecord()
            : m_buffer(NULL),
              m_cb(0),
              m_offset(0),
              m_dataOffset(0),
              m_cbRecord(0),
              m_remainingRecordCount(0),
              m_header(NULL)
        { }
        
        DNSResponseRecord::~DNSResponseRecord()
        { }
    
        void DNSResponseRecord::Initialize(const char* buffer, const int cb, const int startOffset, const int recordCount)
        {
            m_buffer = buffer;
            m_cb = cb;
            m_offset = startOffset;
            m_remainingRecordCount = recordCount - 1;
            int cbDomain = CbDomainName(buffer + startOffset, cb - startOffset);
            m_header = (RRHeader*)(buffer + startOffset + cbDomain);
            m_dataOffset = m_offset + cbDomain + sizeof(RRHeader);
            m_cbRecord = cbDomain + sizeof(RRHeader) + m_header->DataLength;
        }
        
        bool DNSResponseRecord::NextRecord()
        {
            if (m_remainingRecordCount > 0)
            {
                Initialize(m_buffer, m_cb, m_offset + m_cbRecord, m_remainingRecordCount);
                return true;
            }

            return false;
        }
    
        const int DNSResponseRecord::CopyDomainName(char* domain, int cbDomain) const
        {
            int cbDomainName = cbDomain;
            ParseDomainName(m_buffer, m_cb, m_offset, domain, cbDomainName);
            return cbDomainName;
        }

        const unsigned short DNSResponseRecord::GetType() const
        {
            return m_header->Type;
        }
        
        const unsigned short DNSResponseRecord::GetClass() const
        {
            return m_header->Class;
        }
            
        const unsigned int DNSResponseRecord::GetTimeToLive() const
        {
            return m_header->TimeToLive;
        }
            
        const unsigned short DNSResponseRecord::GetDataLength() const
        {
            return m_header->DataLength;
        }
            
        void DNSResponseRecord::CopyDataAsAddress(int& address) const
        {
            address = *(int *)(m_buffer + m_dataOffset);
        }

        void DNSResponseRecord::CopyDataAsAddress(in6_addr& address) const
        {
            address = *(in6_addr *)(m_buffer + m_dataOffset);
        }

        int DNSResponseRecord::CopyDataAsString(char* data, int cbData) const
        {
            switch (GetType())
            {
                case ApplicationLayer::DNS::RRType::A: return CopyAData(data, cbData); break;
                case ApplicationLayer::DNS::RRType::NS: return CopyDataAsDomainName(data, cbData); break;
                case ApplicationLayer::DNS::RRType::CNAME: return CopyDataAsDomainName(data, cbData); break;
                case ApplicationLayer::DNS::RRType::SOA: return CopySOAData(data, cbData); break;
                case ApplicationLayer::DNS::RRType::WKS: return CopyDataNYI(data, cbData); break;
                case ApplicationLayer::DNS::RRType::PTR: return CopyDataAsDomainName(data, cbData); break;
                case ApplicationLayer::DNS::RRType::HINFO: return CopyDataNYI(data, cbData); break;
                case ApplicationLayer::DNS::RRType::MINFO: return CopyDataNYI(data, cbData); break;
                case ApplicationLayer::DNS::RRType::MX: return CopyMXData(data, cbData); break;
                case ApplicationLayer::DNS::RRType::TXT: return CopyDataAsDomainName(data, cbData); break;
                default: return snprintf(data, cbData, "Unknown"); break;
            }
        }
        
        int DNSResponseRecord::CopyAData(char* data, int cbData) const
        {
            sockaddr_in addr;
            addr.sin_addr.s_addr = *((long*)(m_buffer + m_dataOffset));
            return snprintf(data, cbData, "%s", ::inet_ntoa(addr.sin_addr));
        }

        int DNSResponseRecord::CopyDataAsDomainName(char* data, int cbData) const
        {
            int cbDomainName = cbData;
            ParseDomainName(m_buffer, m_cb, m_dataOffset, data, cbDomainName);
            return cbDomainName;
        }
        
        int DNSResponseRecord::CopySOAData(char* data, int cbData) const
        {
            char mname[256];
            int cbMName = sizeof(mname);
            int cbRead = ApplicationLayer::DNS::ParseDomainName(m_buffer, m_cb, m_dataOffset, mname, cbMName);
        
            char rname[256];
            int cbRName = sizeof(rname);
            cbRead += ApplicationLayer::DNS::ParseDomainName(m_buffer, m_cb, m_dataOffset + cbRead, rname, cbRName);
        
            const uint32_t* remaining = (const uint32_t*)(m_buffer + m_dataOffset + cbRead);
            uint32_t serial = ::ntohl(remaining[0]);
            uint32_t refresh = ::ntohl(remaining[1]);
            uint32_t retry = ::ntohl(remaining[2]);
            uint32_t expire = ::ntohl(remaining[3]);
            uint32_t minimumTTL = ::ntohl(remaining[4]);
            return snprintf(data, cbData, "MName: %.*s, RName: %.*s, Serial: %d, Refresh: %d, Retry: %d, Expire: %d, Minimum TTL: %d", cbMName, mname, cbRName, rname, serial, refresh, retry, expire, minimumTTL);
        }
        
        int DNSResponseRecord::CopyDataNYI(char* data, int cbData) const
        {
            return snprintf(data, cbData, "NYI");
        }
        
        int DNSResponseRecord::CopyMXData(char* data, int cbData) const
        {
            unsigned short preference = *(unsigned short*)(m_buffer + m_dataOffset);
        
            char host[256];
            int cbHost = sizeof(host);
            int cbRead = ApplicationLayer::DNS::ParseDomainName(m_buffer, m_cb, m_dataOffset + sizeof(short), host, cbHost);
            return snprintf(data, cbData, "%.*s at preference %d", cbHost, host, preference);
        }
    }
}
