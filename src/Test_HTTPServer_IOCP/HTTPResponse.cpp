#pragma once

#include "HTTPResponse.h"

#include <regex>
#include <iostream>
using namespace std;
//分析http头
bool HttpResponse::SetRequest(std::string request, std::string & responseType)
{
	string src = request;
	string pattern = "^([A-Z]+) /([a-zA-Z0-9]*([.][a-zA-Z]*)?)[?]?(.*) HTTP/1";
	regex r(pattern);
	smatch mas;
	regex_search(src, mas, r);
	if (mas.size() == 0)
	{
		cerr << pattern << " failed!" << endl;
		return false;
	}
	string type = mas[1];
	responseType = type;
	string path = "/";
	path += mas[2];
	string filetype = mas[3];
	string query = mas[4];

	if (filetype.size()>0)
		filetype = filetype.substr(1, filetype.size() - 1);//去掉“.”
	
	if (type != "GET") {
		cerr << "Not Get!!!!!" << endl;
		return false;
	}
	string filename = path;
	//访问根目录
	if (path == "/")
	{
		filename = "/index.html";
	}
	string filepath = "www";
	filepath += filename;

	//打开php的情况
	if (filetype == "php")
	{
		//pht-cgi www/index.php?id=1 name=2 > www/index.php.html
		string cmd = "php-cgi ";
		cmd += filepath;   //filepath = index.php
		cmd += " ";
		for (int i = 0; i<query.size(); i++)
		{
			if (query[i] == '&')
				query[i] = ' ';
		}
		cmd += query;

		cmd += " > ";
		filepath += ".html";
		cmd += filepath;

		system(cmd.c_str());
	}
	//根据路径找到文件
	if (fopen_s(&fp, filepath.c_str(), "rb") != 0)
	{
		printf("Open file %s failed!\n", filepath.c_str());
		return false;
	}
	//获取文件大小
	fseek(fp, 0, SEEK_END);
	filesize = ftell(fp);
	fseek(fp, 0, 0);

	if (filetype == "php")
	{
		char c = 0;
		//\r\n\r\n   去掉php头部
		int headsize = 0;
		while (fread(&c, 1, 1, fp) > 0)
		{
			headsize++;
			if (c == '\r')
			{
				fseek(fp, 3, SEEK_CUR);
				headsize += 3;
				break;
			}
		}
		filesize -= headsize;
	}
	return true;
}
string  HttpResponse::GetHead()
{
	//返回头信息
	//回应HTTPGET请求
	//消息头
	string rmsg = " ";
	rmsg = "HTTP/1.1 200 OK\r\n";
	rmsg += "Server: XHTTP\r\n";
	rmsg += "Content-Type: text/html\r\n";
	rmsg += "Content-Length:";
	char bsize[128] = { 0 };
	sprintf_s(bsize, "%d", filesize);
	rmsg += bsize;
	rmsg += "\r\n\r\n";
	return rmsg;
}
int  HttpResponse::Read(char *buf, int bufsize)
{
	//将请求的文件存入buf
	return fread(buf, 1, bufsize, fp);
}

HttpResponse::HttpResponse()
{
}


HttpResponse::~HttpResponse()
{
}
