# tcp-using-udp


##Data Sequencing: 
The sender (client or server - both should be able to send as well as receive data) must divide the data (assume some text) into smaller chunks (using chunks of fixed size or using a fixed number of chunks). Each chunk is assigned a number which is sent along with the transmission (use structs). The sender should also communicate the total number of chunks being sent. After the receiver has data from all the chunks, it should aggregate them according to their sequence number and display the text.
##Retransmissions: 
The receiver must send an ACK packet for every data chunk received (The packet must reference the sequence number of the received chunk). If the sender doesn’t receive the acknowledgement for a chunk within a reasonable amount of time (say 0.1 seconds), it must resend the data. However, the sender shouldn’t wait for receiving acknowledgement for a previously sent chunk before transmitting the next chunk.
