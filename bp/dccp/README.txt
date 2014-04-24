README for DCCPCLO/DCCPCLI

DCCPCLO/DCCPCLI are DCCP (Datagram Congestion Control Protocol) based convergence
layer daemons for BP. The unique feature of DCCP is that it provides congestion
control without guaranteeing reliable, in-order delivery. This makes it a good
convergence-layer protocol for use with BP's Custody Transfer.

Please note that there is no fragmentation support in this convergence layer and
we do not intend to add any. This means that this convergence layer is limited
to transmitting bundles no larger than the MTU of the link it is running on.
The DCCP RFCs make it clear that IP fragmentation is strongly discouraged 
(and Linux doesn't implement DCCP fragmentation). Further, because DCCP is not a
reliable protocol, fragmentation will increase the apparent loss rate on a link
and make it nearly impossible to receive an entire bundle at even medium error
rates.

DCCP is a connection oriented protocol that provides congestion control without
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
