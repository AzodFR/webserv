#ifndef HEADERS_HPP
#define HEADERS_HPP

#include "Utils.hpp"

class Headers
{
private:
	map<unsigned int, map<string, string> > _headers;

public:
	Headers();
	void operator+=(string);
	~Headers();
	map<string, string> operator[](int n) { return _headers[n]; };
	string return_response_header(int status, Headers header, size_t size_content);
	string return_response_header(int status, Headers header, size_t size_content, string);
	size_t size() { return _headers.size(); };
	size_t count() { return _headers.size() - 1; };
	map<string, string> last() { return _headers[count()]; };
	int check(map<string, string>);
};

#endif