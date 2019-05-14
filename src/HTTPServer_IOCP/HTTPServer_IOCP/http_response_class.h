#pragma once

#include<string>

class HttpResponse
{
public:	
	HttpResponse();
	~HttpResponse();
	bool SetRequest(std::string request);
	std::string GetHead();
	int Read(char *buf, int bufsize);

	//private:
	int filesize = 0;
	FILE *fp = 0;
};

