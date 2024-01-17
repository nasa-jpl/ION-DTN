README for DCCPLSO/DCCPLSI

DCCPLSO/DCCPLSI are DCCP (Datagram Congestion Control Protocol) based link service
daemons for LTP. DCCP provides congestion control without guaranteeing reliable, 
in-order delivery. As such, it is a good choice for use with LTP which can
selectively provide reliable, in-order delivery via its red/green parts.

DCCP is a connection-oriented protocol that provides congestion control without
reliability. It can be thought of as TCP without retransmissions or UDP with
handshakes and congestion control. Despite being defined in an RFC in 2006, DCCP
has yet to become popular on the general Internet. In fact, the only maintained
implementation is in the Linux kernel. 

While DCCP was first added to the Linux Kernel in version 2.6.14, our experiments
with the protocol in late 2010 revealed numerous bugs in the Linux implementation.
I reported this to the Linux developers, and we worked to fix these issues. The
fixes have just been merged into Linux Kernel 3.2.0. Because of the seriousness
of these bugs, we have chosen to disable DCCP on systems with Kernels older than
3.2.0.

Ubuntu 12.04 and Fedora 17 both come with kernel version 3.2.0 or above by default.
Hence, my first recommendation to those desiring to use DCCP is to update to the
most recent version of your favorite distro.

If for some reason you can't do that, you will need to build a recent kernel from
source. The following article does a very good job at describing the process: 
http://www.cyberciti.biz/tips/compiling-linux-kernel-26.html


Samuel Jero
Internetworking Research Group
Ohio University
sj323707@ohio.edu

Last Updated: March 20, 2013
