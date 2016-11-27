namespace ApplicationLayer
{
	namespace DNS
	{
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
		
		int AddQuestion(char* data, int cbData, const char* name, unsigned short queryType, unsigned short queryClass);
		int WriteDomainName(const char* domainName, int cbDomainName, char* data, int cbData);
		int ParseDomainName(char* data, int cbData, int offsetOfDomainName, char* domainName, int& cbDomainName);
	}
}
