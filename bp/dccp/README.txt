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

Unfortunately, our tests using the Linux DCCP implementation showed that the code
in the standard Linux kernel is practically unusable. In order to get stable
operation it was necessary to pull and build the DCCP testing tree. We recommend
that anyone attempting to use DCCP with ION at this time do the same.

The DCCP testing tree is maintained as a git repository at 
http://eden-feed.erg.abdn.ac.uk/. Once you pull the tree, you need to select the
"dccp" branch. For detailed instructions on pulling the testing tree, please see:
http://www.linuxfoundation.org/collaborate/workgroups/networking/dccp_testing#Cloning_the_entire_tree
Once you have the testing tree, simply configure, build, and install the kernel as
appropriate for your system. If you are not familiar with the steps required to
build a Linux kernel, here is a good guide: 
http://www.cyberciti.biz/tips/compiling-linux-kernel-26.html

Using the DCCP testing tree gets usable performance from DCCP. However, the
throughput is still very variable and the performance compared to TCP or UDP
is lousy. After some analysis, it appears that DCCP tends to 1)fill the 
receiver's socket buffer too quickly, which results in dropped packets and
ultimately 200ms gaps in transmission and 2)drop the sender's congestion window
down to 1 packet per RTT and never increase it.

As a temporary work-around for extremely poor throughput, you might try increasing
the priority of dccpcli. Our tests indicate that this reduces the 200ms gaps in
transmission mentioned above, which results in much better throughput.

The remaining DCCP issues are under active research; we will update this README
as we learn more about their causes.

Samuel Jero
Internetworking Research Group, Ohio University
sj323707@ohio.edu

Last Updated: December 13, 2010
