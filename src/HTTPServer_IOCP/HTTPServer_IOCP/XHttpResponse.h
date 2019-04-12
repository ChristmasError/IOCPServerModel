#pragma once
#include<string>
using namespace std;
class XHttpResponse
{
public:
	bool SetRequest(string request);
	string GetHead();
	int Read(char *buf, int bufsize);
	XHttpResponse();
	~XHttpResponse();
//private:
	int filesize=0;
	FILE *fp = NULL;
};

