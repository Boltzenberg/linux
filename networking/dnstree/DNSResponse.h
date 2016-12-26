#include "DNS.h"

namespace ApplicationLayer
{
	namespace DNS
	{
        class DNSQuestion
        {
            public:
                DNSQuestion();
                ~DNSQuestion();

                void Initialize(const char* buffer, const int cb, const int startOffset, const int questionCount);
                bool NextQuestion();

                const int CopyDomainName(char* domain, int cbDomain) const;
                const unsigned short GetType() const;
                const unsigned short GetClass() const;

            private:
                const char* m_buffer;
                int m_cb;
                int m_offset;
                int m_cbQuestion;
                int m_remainingQuestionCount;
                const Question* m_question;
        };

        class DNSResponseRecord
        {
            public:
                DNSResponseRecord();
                ~DNSResponseRecord();

                void Initialize(const char* buffer, const int cb, const int startOffset, const int recordCount);
                bool NextRecord();

                const int CopyDomainName(char* domain, int cbDomain) const;
                const unsigned short GetType() const;
                const unsigned short GetClass() const;
                const unsigned int GetTimeToLive() const;
                const unsigned short GetDataLength() const;
                void CopyRawData(char* data, int cbData) const;
                int CopyDataAsString(char* data, int cbData) const;

            private:
                int CopyAData(char* data, int cbData) const;
                int CopyDataAsDomainName(char* data, int cbData) const;
                int CopySOAData(char* data, int cbData) const;
                int CopyDataNYI(char* data, int cbData) const;
                int CopyMXData(char* data, int cbData) const;
                
                const char* m_buffer;
                int m_cb;
                int m_offset;
                int m_dataOffset;
                int m_cbRecord;
                int m_remainingRecordCount;
                const RRHeader* m_header;
        };
        
        class DNSResponse
        {
            public:
                DNSResponse(const char* buffer, const int cb);
                ~DNSResponse();

                const DNSHeader* GetHeader() const;
                bool InitializeDNSQuestions(DNSQuestion& question) const;
                bool InitializeDNSAnswers(DNSResponseRecord& record) const;
                bool InitializeDNSAuthorities(DNSResponseRecord& record) const;
                bool InitializeDNSAdditionals(DNSResponseRecord& record) const;
                
            private:
                const char* m_buffer;
                const int m_cb;

                const DNSHeader* m_header;
                int m_questionsOffset;
                int m_answersOffset;
                int m_authoritiesOffset;
                int m_additionalsOffset;
        };
	}
}
