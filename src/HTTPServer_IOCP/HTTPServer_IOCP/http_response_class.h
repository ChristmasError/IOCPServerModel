#pragma once

#include<string>

class HttpResponse
{
public:
	bool SetRequest(std::string request);
	std::string GetHead();
	int Read(char *buf, int bufsize);
	HttpResponse();
	~HttpResponse();
	//private:
	int filesize = 0;
	FILE *fp = 0;
};

