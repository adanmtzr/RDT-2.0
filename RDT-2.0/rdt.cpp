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

int rdt_recv(int socket_descriptor, char *buffer, int buffer_length, int flags, struct sockaddr *from_address, int *address_length) {
    // TODO: Recieve multiple packets and store in buffer
	uint32_t sequenceNumber = 0;
	
	int totalPackets = ceil((double)buffer_length/500);
	//int totalPackets = (buffer_length + 500 - 1) / 500;
	packet* packets = new packet[totalPackets];
	// get the header from the packets

	
	for(int loopNum = 0; loopNum < totalPackets; loopNum++)
	{
		packets[loopNum] = udp_rcv(socket_descriptor, buffer, buffer_length, flags, from_address, address_length, packets[loopNum], loopNum);
	}
	
	for(int loopNum = 0; loopNum < totalPackets; loopNum++)
	{
		extract_pk(buffer, buffer_length, sequenceNumber, packets[loopNum], loopNum);
	}

  // Null terminate end of message.
  //buffer[buffer_length] = '\0';

    return buffer_length;
}

packet udp_rcv(int socket_descriptor, char *buffer, int buffer_length, int flags, struct sockaddr *from_address, int *address_length, packet hPacket, int loopNum)
{
	char tempBuffer[512];
    int didRecieve = recvfrom(socket_descriptor, &tempBuffer, 512, flags, from_address, (socklen_t*)address_length);
	printf("RDT Received Length %d\n", didRecieve);
    printf("Recieved a packet!\n");

	if (didRecieve == -1)
	{
		printf("Error recieving packet.\n");
	}
	
	//packet tempPacket;

    memcpy(&hPacket, tempBuffer, 512);
	//printf("Packet segment %s\n", hPacket.data);
	return hPacket;
}
int rdt_sendto(int socket_descriptor, char *buffer, int buffer_length, int flags, struct sockaddr * destination_address, int address_length) {
    // TODO: Send multiple packets over socket
	uint32_t sequenceNumber = 0;
	//	char pakBuffer[500];  
	// Figure out how many packets
  
	int totalPackets = ceil((double)buffer_length/500);
	//int totalPackets = (buffer_length + 500 - 1) / 500;
	printf("Buffer length %d\n", buffer_length);
	printf("Total packets %d\n", totalPackets);
	packet* packets = new packet[totalPackets];

	// Make every packet
	for(int loopNum = 0; loopNum < totalPackets; loopNum++)
	{
		

	  packets[loopNum] = make_pkt(buffer, buffer_length, sequenceNumber, packets[loopNum], loopNum);

	}  
	
	//printf("random packet %s\n", &packets[3].data);
	
	// Send every packet
	for(int loopNum = 0; loopNum < totalPackets; loopNum++)
	{
	    // pakBuffer[loopNum] = packets->data[loopNum];
	    udt_sendto(socket_descriptor, buffer, buffer_length, flags, destination_address, address_length, &packets[loopNum]);
	}

    return buffer_length;
}

void udt_sendto(int socket_descriptor, char *buffer, int buffer_length, int flags, struct sockaddr * destination_address, int address_length, packet* hPacket)
{
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
	//uint32_t* packet = (uint32_t*)malloc(sizeof(packet_t)+length);
	//(*(packet_t)packet).seq_number= seqNo;
	//memcpy(packet + sizeof(packet_t), buffer, length);
	
	// Attach data to packet
	for(int dataLoop = 0; dataLoop < 500; dataLoop++)
	{
		int dataPoint = (loopNum * 500) + dataLoop;
		(dataPoint < length) ? hPacket.data[dataLoop] = buffer[dataPoint] : hPacket.data[dataLoop] = '\0';
	}
	hPacket.cksum = getCheckSum(hPacket);
	printf("Packet CHECKSUM %04X\n", hPacket.cksum);
return hPacket;
	//return packet;
}
void extract_pk(char *buffer, int length, uint32_t seqNo, packet hPacket, int loopNum)
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
	uint16_t calcCheckSum = getCheckSum(hPacket);	
	if(calcCheckSum == hPacket.cksum)
	  {

	    printf("It worked !!!!! Yeahhhh !!\n");
	  }
	else {printf("IT exploted ;(\n");

	  printf("********** %d\n", calcCheckSum);
}
printf("EXtract packet chceksum  %04X\n",hPacket.cksum );

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
  //  printf("CheckSum value at the function is = *** %04X\n\n", sum);
  return ((unsigned short)~sum );
}
unsigned short getCheckSum(packet cpacket){
  //  unsigned char *buff;
  unsigned short pakCksum;
  cpacket.len = strlen(cpacket.data); 
  //  buff = cpacket.data;
  pakCksum = checkSum((unsigned char *)cpacket.data, cpacket.len);
  printf("pakCksum    --> %04X", pakCksum);
  return pakCksum;

}
