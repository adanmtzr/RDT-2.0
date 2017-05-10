#include "rdt.h"

int rdt_socket(int address_family, int type, int protocol) {
    int didUDPSocket = socket(address_family, type, protocol);
    if (didUDPSocket == -1) {
        printf("Error starting RDT socket\n");
    } else {
        printf("Starting RDT socket\n");
    }
    return didUDPSocket;
}

int rdt_bind(int socket_descriptor, const struct sockaddr *local_address, socklen_t address_length) {
    int didBind = bind(socket_descriptor, local_address, address_length);
    if (didBind == -1) {
        printf("Cound not bind RDT socket\n");
    } else {
        printf("RDT socket binded\n");
    }
    return didBind;
}

/*

***************RDT RECEIVED*********************
*/
int rdt_recv(int socket_descriptor, char *buffer, int buffer_length, int flags, struct sockaddr *from_address, int *address_length) {
    // TODO: Recieve multiple packets and store in buffer
	uint32_t sequenceNumber = 0;


	int totalPackets = ceil((double)buffer_length/500);
	//int totalPackets = (buffer_length + 500 - 1) / 500;
	packet* packets = new packet[totalPackets];
	// get the header from the packets


	for(int loopNum = 0; loopNum < totalPackets; loopNum++)
	  {

	    sequenceNumber++;
	    	packets[loopNum] = udp_rcv(socket_descriptor, buffer, buffer_length, flags, 
	   		   from_address, address_length, packets[loopNum], loopNum);
       	extract_pk(buffer, buffer_length, sequenceNumber, packets[loopNum], loopNum);
 // set up select
	  fd_set set;
	  struct timeval timeout;
	  char receiAckBuffer[1];
	  bzero(receiAckBuffer,1);
	  // initialize timeout data structure
	  timeout.tv_sec = 5;
	  timeout.tv_usec = 0;
	  int count=0;
	  //  if times out select return 0, input available select return 1,
	  // if an error select return -1

	  // printf("Sending to the client %s receiAck outside /n", receiAckBuffer);
	  //	  printf("Server ack received %d \n", recAck);
	  while(true)
	    {
	      //    packets[loopNum] = udp_rcv(socket_descriptor, buffer, buffer_length,
	      //			 flags, from_address, address_length, packets[loopNum], loopNum);
	      int recAck = packets[loopNum].ackno;
	      receiAckBuffer[0] = recAck;
	      //initialize file descriptor
	      FD_ZERO(&set);
	      FD_SET(socket_descriptor, &set);
	      int resultSelect = select(socket_descriptor, &set, NULL, NULL, &timeout);
	      
	      if(resultSelect == -1)
		{// error
		  printf(" Server Select() **Error ** \n");
		  exit(1);
		}
	      
	      else  if(resultSelect == 0)
		{
		  count++;
		  //it times out
		  printf("Server Select() TIMEOUTS** \n");
		  if(count == 3)
		    exit(1);
		    }
	      else if(FD_ISSET(socket_descriptor, &set))
		{// there is no timeout
		  printf("Server Select(1) %s \n", receiAckBuffer);
		  printf("Server Select() IS GOOD** \n");
		 extract_pk(buffer, buffer_length, sequenceNumber, packets[loopNum], loopNum);
		 memset(packets[loopNum].data, 0, packets->len);
		 packets[loopNum].seqno = sequenceNumber;
		 read(socket_descriptor, receiAckBuffer, sizeof(receiAckBuffer));
		 printf("Received this sequence number fromclient %d \n", packets[loopNum].seqno);
		 		 	     

		}

	    }// end while
	  }
	/*
		
	for(int loopNum = 0; loopNum < totalPackets; loopNum++)
	{
	  sequenceNumber++; // increment seq num at the receiving host
	  // extract_pk(buffer, buffer_length, sequenceNumber, packets[loopNum], loopNum);

	 


	  }*/
		
	// Null terminate end of message.
	//buffer[buffer_length] = '\0';
	
    return buffer_length;
}

packet udp_rcv(int socket_descriptor, char *buffer, int buffer_length, int flags, struct sockaddr *from_address, int *address_length, packet hPacket, int loopNum)
{
	char tempBuffer[512];
	bzero((void *) tempBuffer, 512);
    int didRecieve = recvfrom(socket_descriptor, &tempBuffer, 512, flags, from_address, (socklen_t*)address_length);
	printf("RDT Received Length %d\n", didRecieve);
    printf("Recieved a packet!\n");

	if (didRecieve == -1)
	{
		printf("Error recieving packet.\n");
	}
	
	//packet tempPacket;
	//unsigned short paklen = strlen(hPacket.data);

	memcpy(&hPacket, tempBuffer, 512);
	//printf("Packet segment %s\n", hPacket.data);
	return hPacket;
}
int rdt_sendto(int socket_descriptor, char *buffer, int buffer_length, int flags, struct sockaddr * destination_address, int address_length) {
    // TODO: Send multiple packets over socket
	uint32_t sequenceNumber = 0;
	// Figure out how many packets

	int totalPackets = ceil((double)buffer_length/500);
	//int totalPackets = (buffer_length + 500 - 1) / 500;
	printf("Buffer length %d\n", buffer_length);
	printf("Total packets %d\n", totalPackets);
	packet* packets = new packet[totalPackets];

	// Make every packet
	for(int loopNum = 0; loopNum < totalPackets; loopNum++)
	{
	  sequenceNumber++; // increment sequence 
	  packets[loopNum] = make_pkt(buffer, buffer_length, sequenceNumber, packets[loopNum], loopNum);

	 
	}  
	//	printf("Packet #1  %s\n", packets[0].data);
	//	printf("\npacket seq# at pak 2 is  %d\n", packets[1].seqno);
	//	printf("\npacket # 3  %s\n", packets[2].data);
	//	printf("\npacket # 4  %s\n", packets[3].data);

	// Send every packet
	for(int loopNum = 0; loopNum < totalPackets; loopNum++)
	{
	
	  udt_sendto(socket_descriptor, buffer, buffer_length, flags, destination_address, address_length, &packets[loopNum]); 

	    	 // set up select
	fd_set set;
	struct timeval timeout;
	int rec;
	int counter=0;
	char receiAckBuffer[1];
	bzero(receiAckBuffer,1);
	// initialize timeout data structure
       	timeout.tv_sec = 5;
	timeout.tv_usec = 0;

	int current_ack_send = packets[loopNum].ackno;
	/*if times out select return 0, input available select return 1,
	  if an error select return -1*/
	while(true)
	  {
	  //initialize file descriptor
	  FD_ZERO(&set);
	  FD_SET(socket_descriptor, &set);
	    int resultSelect = select(socket_descriptor, &set, NULL, NULL, &timeout);
	
	  if(resultSelect == -1)
	    {// error
	      printf("Select() **Error ** \n");
	      exit(1);
	    }
	  else  if(resultSelect == 0)
	    {
	      counter++;
	      //it times out
	      printf("Select() TIMEOUTS** \n");
	      //resend packets since the last dropped
	      for(int i = current_ack_send; i < totalPackets; i++)
		{

		  udt_sendto(socket_descriptor, buffer, buffer_length, flags, destination_address, address_length, &packets[i]);
		  printf("retransmiting packect \n"); 
		}
	       timeout.tv_sec = 3;
	       timeout.tv_usec = 0;
	       if(counter == 3)
		 exit(1);
	    }
       	
	  else if(FD_ISSET(socket_descriptor, &set))
	    {// there is no timeout
	      printf("Select() IS GOOD** \n");
	      int rec = recv(socket_descriptor, receiAckBuffer, sizeof(receiAckBuffer), 0);

	      if(rec == current_ack_send)
		{
		  printf("Received from Server %s\n", receiAckBuffer);

		}
	      for(int i = current_ack_send; i < totalPackets; i++)
		{

		  udt_sendto(socket_descriptor, buffer, buffer_length, flags, destination_address, address_length, &packets[i]);
		  printf("retransmiting packect \n"); 
		}
	    }

	  }// end while
	}


    return buffer_length;
}

void udt_sendto(int socket_descriptor, char *buffer, int buffer_length, int flags, struct sockaddr * destination_address, int address_length, packet* hPacket)
{
  /*  //simulate packet Corruption
  time_t t;
  int randNum;
  // initize the random number
  srand((unsigned) time(&t));

  // random number between 0 and 1999
  randNum = rand()%2000;
  printf("Random number generated %d\n", randNum);
  */

	//printf("Packet segment %s", hPacket.data);
	//printf("Packet segment %s\n", hPacket->data);
  
	int didSend = sendto(socket_descriptor, hPacket, 512, flags, destination_address, address_length);
	if (didSend == -1)
	{
		printf("Error sending packet");
	}

}

int rdt_close(int fildes) {
    int didClose = close(fildes);
    if (didClose == -1) {
        printf("Could not close RDT socket\n");
    } else {
        printf("Closing RDT socket\n");
    }
    return didClose;
}

packet make_pkt(char *buffer, int length, uint32_t seqNo, packet hPacket, int loopNum)
{

	// Attach data to packet
	for(int dataLoop = 0; dataLoop < 500; dataLoop++)
	{
	  hPacket.data[dataLoop] = '\0';
		int dataPoint = (loopNum * 500) + dataLoop;
		(dataPoint < length) ? hPacket.data[dataLoop] = buffer[dataPoint] : hPacket.data[dataLoop] = '\0';
	
	}
		hPacket.seqno = seqNo;
	//	printf("Making sequence number %d\n", hPacket.seqno);
	hPacket.cksum = getCheckSum(hPacket);
	hPacket.ackno = seqNo;
	printf("make_pkt func  hPacket.cksum >>>>>   %04X  at packet %d\n", hPacket.cksum, seqNo);
	printf("make_pkt func  hPacket.ackno >>>>>   %d  at packet %d\n", hPacket.ackno, seqNo);  

return hPacket;
	//return packet;
}
packet extract_pk(char *buffer, int length, uint32_t seqNo, packet hPacket, int loopNum)
{
	//extract packets

	for(int dataLoop = 0; dataLoop < 500; dataLoop++)
	{
		int dataPoint = loopNum * 500 + dataLoop;
		if(dataPoint < length)
		{
			buffer[dataPoint] = hPacket.data[dataLoop];
		}
	}

	
	unsigned short calcCheckSum = getCheckSum(hPacket);	
	if(calcCheckSum == hPacket.cksum)
	  {
	    
	    printf("****Succeessful: Packet arrived !!****\n");
	  }
	else {printf("\t--> :(  Error: Loss packet. Please resend the missing packet. ): \n");
	  //  printf("Packet data thats it is bad  :::: %s", hPacket.data);
	  printf("****calcCheckSum ****** %04X\n", calcCheckSum);
	  // printf("****hPacket.cksum at the extract funct **** %04X\n", hPacket.cksum);
	 
	}
	    printf("Packet hPacket Sequence Number = %d ", hPacket.seqno);
	//	printf("EXtract funct hPacket.cksum ==>  %04X\n",hPacket.cksum );
		if(hPacket.seqno == seqNo)
	  {
	     printf("\t **Sequence Number is Valid** %d\n", seqNo );

	  }
	  	else
	  {
	    printf("\t\t !!!! INValid Sequence Number !!!!!!  %d\n", seqNo );
	  }
		return hPacket;	
}

unsigned short checkSum(unsigned char *addr, int nBytes)
{
  unsigned int sum = 0;
  //Summing loop
  while(nBytes>1)
    {
      sum = sum + *(unsigned short *)addr;
      addr++;
      nBytes -=2;
    }
  //Add the left-Over byte, if any 
  if(nBytes>0)
    {
      sum = sum + *((unsigned char *)addr);
    }
  //Fold 32-bit to 16-bits
  while(sum >> 16) 
    sum = (sum & 0XFFFF)+(sum >> 16);
  //  printf("\tYour CheckSum  ++++> %04X \n",((unsigned short)~sum));
  return ((unsigned short)~sum );
}


//calculates the Checksum of the given packet
unsigned short getCheckSum(packet cpacket){

  unsigned short pakCksum;
  cpacket.len = strlen(cpacket.data); 

  cpacket.len = 500;
  pakCksum = checkSum((unsigned char *)cpacket.data, cpacket.len);
  printf("getCheckSum funct pakCksum    --> %04X\n", pakCksum);
  return pakCksum;

}


