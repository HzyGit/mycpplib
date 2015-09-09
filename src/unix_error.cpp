#include "mycpp/unix_error.h"
#include <errno.h>
#include <sstream>
#include <string.h>

UnixError::UnixError(const std::string &note)throw()
	:_errno(errno)
{
	std::ostringstream sout;
	sout<<program_invocation_short_name<<":";
	sout<<__FILE__<<","<<__LINE__;
	if(!note.empty())
		sout<<":"<<note;
	if(_errno>0)
		sout<<":"<<strerror(_errno);
	_err_str=sout.str();
}

UnixError::UnixError(int err,const std::string &note)throw()
	:_errno(err)
{
	std::ostringstream sout;
	sout<<program_invocation_short_name<<":";
	sout<<__FILE__<<","<<__LINE__;
	if(!note.empty())
		sout<<":"<<note;
	if(_errno>0)
		sout<<":"<<strerror(_errno);
	_err_str=sout.str();
}

UnixError::~UnixError()throw()
{
}

int UnixError::get_errno()const throw()
{
	return _errno;
}

const char *UnixError::what()const noexcept
{
	return _err_str.c_str();
}
