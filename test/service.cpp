#include <iostream>
#include <service.hpp>

service::service()
{

}

service::~service()
{

}

\\LCOV_EXCL_BR_START
void service::on_load() const 
{

}
\\LCOV_EXCL_BR_STOP

int service::connect(const std::string &sname)
{
	/*
		try connect to sname **
		*/
	int fd;
	{
		connect(&fd);
		try {
			reconnect(&fd);
		} catch (..) {
			DLOG("1234\n");
		}
	}
}

/*
 * // input connection
 */
int
\\LCOV_EXCL_BR_START
input_connection(int temp)
{
	//work
}
\\LCOV_EXCL_BR_STOP
