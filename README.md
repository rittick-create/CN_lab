
Sender side + extra things

Sender file will be recieved form the command line

Covnert that to dataword 

We will be using Ethernet IEEE 802.3 standard frame

| Preamble | SFD | Destination MAC | Source MAC | Length/Type | Data + Pad | FCS |
|  7 bytes | 1 B |      6 B        |    6 B     |     2 B     | 46–1500 B  | 4 B |

<.                      Header                                >< payload>< Trailer >


Then we will add error injection techniques(random) to add errors to codeword

Then we will add redundant bits to the codeword 

Then this dataword(codeword+redundat bits) will be sent to the reciever.

------------------------------------------------------------------------------------------------------------

Reciever side (mainly error detection)
Receiver will check if there is any error detected. Based on the detection it will accept
or reject the dataword.

------------------------------------------------------------------------------------------------------------


Error Injections used:- 
single-bit error, two isolated single-bit errors, odd number of errors, and burst
errors.

------------------------------------------------------------------------------------------------------------

Error detection techniques used (also used for adding redundant bits) are:-
Checksum (16-bit)
(CRC-8, CRC-10, CRC-16 and CRC-32)

------------------------------------------------------------------------------------------------------------



=>In our case the header file will contain the source mac the destination mac(We will include this somehow) and payload length and will also contain the error detection type.

=>


