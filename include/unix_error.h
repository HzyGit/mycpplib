#ifndef _UNIX_ERROR_H
#define _UNIX_ERROR_H
#include <exception>
#include <string>

class UnixError : public std::exception
{
	public:
		UnixError(const std::string &note=std::string())throw();
		UnixError(int err,const std::string &note=std::string())throw();
		virtual ~UnixError()throw();

		int get_errno()const throw();
	private:
		int _errno;
		std::string _err_str;

};

#endif
