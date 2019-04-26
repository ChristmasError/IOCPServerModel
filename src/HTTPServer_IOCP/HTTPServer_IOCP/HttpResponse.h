#pragma once
#include<string>
using namespace std;
class HttpResponse
{
public:
	bool SetRequest(string request);
	string GetHead();
	int Read(char *buf, int bufsize);
	HttpResponse();
	~HttpResponse();
//private:
	int filesize=0;
	FILE *fp = NULL;
};

