# ION Watch Characters

ION Version: 4.1.3s

Bundle Protocol Version:7

---

Watch characters, when activated, provide immediate feedback on ION operations by printing various characters standard output (terminal). By examing the watch characters, and the order in which they appear, operators can quickly confirm proper operation or detect configuration or run-time errors.

This document will list all watch characters currently supported by ION.

## Enhanced Watch Characters (ION 4.1.3 or later)

Enhanced watch characters were added ION 4.1.3 to provide detailed state information at Bundle Protocol (BP) and LTP levels and can be activated at compile time by:

```bash
./configure --enable-ewchar
or 
./configure CFLAGS=-DEWCHAR
```

Enhanced watch characters prepends additional state information to the standard watch characters inside a pair of parenthesis. In this document, we use the following notion regarding enhanced watch characters information.

* `nnn` = source node number
* `sss` = service number
* `ttt` = bundle creation time in milliseconds Epoch(2000)
* `ccc` = bundle sequence number
* `xxx` (LTP session number)

Each field can be longer or shorter than 3 digits/characters.

## Logging and Processing of Watch Characters

Besides real-time monitoring of the watch characters on standard out, ION can redirect the watch characters to customized user applications for network monitoring purposes.Prior to and including ION 4.1.2, watch characters are single character. Starting from ION 4.1.3 release, a _watch character_ is now generalized to a string of type `char*`.

To activate customized processing, use the following steps:

* Create a C code `gdswatcher.c` that defines a functions to process watch characters, and pass that function to ION to handle watch character:

  ```c
  static void processWatchChar(char* token)
  {
      //your code goes here
  } 

  static void ionRedirectWatchCharacters()
  { 
      setWatcher(processWatchChar);
  }
  ```

* Then use the following compiler flag to build ION:

  ```bash
  ./configure --enable-ewchar CFLAGS="-DGDSWATCHER -I/<path to the folder holding the gdswatcher.c file>"
  ```

* You can also include the GDSLOGGER program, then

  ```bash
  ./configure --enable-ewchar CFLAGS="-DGDSWATCHER -DGDSLOGGER -I/<path to the folder holding the gdswatcher.c file>"
  ```

## Bundle Protocol Watch Character

`a` - new bundle is queued for forwarding; `(nnn,sss,tttt,cccc)a`

`b` - bundle is queued for transmission; `(nnn,sss,ccc)b`

`c` - bundle is popped from its transmission queue; `(nnn,sss,ccc)c`

`m` - custody acceptance signal is received

`w` - custody of bundle is accepted

`x` - custody of bundle is refused

`y` - bundle is accepted upon arrival; `(nnn,sss,tttt,ccc)y`

`z` - bundle is queued for delivery to an application; `(nnn,sss,tttt,ccc)z`

`~` - bundle is abandoned (discarded) on attempt to forward it; `(nnn,sss,ccc)`~

`!` - bundle is destroyed due to TTL expiration; `(nnn,sss,ccc)!`

`&` - custody refusal signal is received

`#` - bundle is queued for re-forwarding due to CL protocol failure; `(nnn,sss,ccc)`#

`j` - bundle is placed in \"limbo\" for possible future re-forwarding; `(nnn,sss,ccc)`j

`k` - bundle is removed from \"limbo\" and queued for re-forwarding; `(nnn,sss,ccc)`k

## LTP Watch Characters

`d` - bundle appended to block for next session

`e` - segment of block is queued for transmission

`f` - block has been fully segmented for transmission; `(xxxx)f`

`g` - segment popped from transmission queue;

* `(cpxxx)g` -- checkpoint, this could be a data segment or a standalone
  check point
* `(dsxxx)g` -- non-check point data segment
* `(rcpxxx)g` -- retransmitted checkpoint
* `(prsxxx)g` -- positive report (all segments received)
* `(nrsxxx)g` -- negative report (gaps)
* `(rrsxxx)g` -- retransmitted report (either positive or negative)
* `(rasxxx)g` -- a report ack segment
* `(csxxx)g` -- cancellation by block source
* `(crxxx)g` -- cancellation by block receiver
* `(caxxx)g` -- cancellation ack for either CS or CR

`h` - positive ACK received for block, session ended; `(xxx)h`

`s` - segment received

* `(dxxx)s` -- received data segement with session number xxx
* `(rsxxx)s` -- received report segement with session number xxx
* `(rasxxx)s` -- received report ack segement with session number xxx
* `(csxxx)s` -- received cancel by source segement with session number xxx
* `(cas<xx)s` -- received cancel by source ack segement with session number xxx
* `(crxxx)s` -- received cancel by receiver segement with session number xxx
* `(carxxx)s` -- received cancel by receiver ack segement with session number xxx

`t` - block has been fully received

`@` - negative ACK received for block, segments retransmitted; `(xxx)@`

`=` - unacknowledged checkpoint was retransmitted; `(xxx)=`

`+` - unacknowledged report segment was retransmitted; `(xxx)+`

`{` - export session canceled locally (by sender)

`}` - import session canceled by remote sender

`[` - import session canceled locally (by receiver)

`]` - export session canceled by remote receiver

## BIBECT Watch Characters

`w` - custody request is accepted (by receiving entity)

`m` - custody acceptance signal is received (by requester)

`x` - custody of bundle has been refused

`&` - custody refusal signal is received (by requester)

`$` - bundle retransmitted due to expiration of custody request timer

## BSSP Watch Characters

`D` - bssp send completed

`E` - bssp block constructed for issuance

`F` - bssp block issued

`G` - bssp block popped from best-efforts transmission queue

`H` - positive ACK received for bssp block, session ended

`S` - bssp block received

`T` - bssp block popped from reliable transmission queue

`-` - unacknowledged best-efforts block requeued for reliable transmission

`*` - session canceled locally by sender
