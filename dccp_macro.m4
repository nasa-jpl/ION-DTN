AC_DEFUN([CHECK_DCCP],
			[AC_MSG_CHECKING(for DCCP support)
			 AC_TRY_RUN(	[#define MIN_REQUIRED_VERSION_MAJOR 2
					#define MIN_REQUIRED_VERSION_MINOR 6
					#define MIN_REQUIRED_VERSION_REV   36

					#include <sys/utsname.h>
					#include <sys/socket.h>
					#include <string.h>
					#include <stdlib.h>
					#ifndef SOCK_DCCP
					#define SOCK_DCCP 6  
					#endif
					#ifndef IPPROTO_DCCP    
					#define IPPROTO_DCCP 33
					#endif

					int main()
					{
						int major, minor, rev;
						struct utsname buf;
						if(uname(&buf)<0){
							return 1;
						}

						major=atoi((strtok(buf.release,".")));
						minor=atoi((strtok(NULL,".")));
						rev=atoi((strtok(NULL,".-")));

						if( major <  MIN_REQUIRED_VERSION_MAJOR || minor < MIN_REQUIRED_VERSION_MINOR 
							 || rev < MIN_REQUIRED_VERSION_REV){
							return 1;
						}

						if(socket(AF_INET, SOCK_DCCP, IPPROTO_DCCP)<0){
							return 1;
						}
					return 0;
					}],
			[ AC_MSG_RESULT(yes) 
			  AC_DEFINE([build_dccp],[1],[DCCP only supported on Linux >2.6.36]) 
			  AC_MSG_NOTICE([You can build dccplsi/dccplso, please see the README in the ltp/dccp source directory])],
			[AC_MSG_RESULT(no)] )
		]
	)
		
