#include "sysInclude.h"

extern void ip_DiscardPkt(char* pBuffer,int type);

extern void ip_SendtoLower(char*pBuffer,int length);

extern void ip_SendtoUp(char *pBuffer,int length);

extern unsigned int getIpv4Address();

int stud_ip_recv(char *pBuffer,unsigned short length)
{
	// get the variable
	char *tempIndex = pBuffer;
	unsigned short version = ((unsigned char)tempIndex[0]) >> 4;
	unsigned short IHL = tempIndex[0] & 15;
	// be careful to add 8 to index;
	tempIndex = tempIndex + 8;
	unsigned short ttl = ((unsigned char)tempIndex[0]);
	tempIndex += 2;
	unsigned int headerChecksum = ((unsigned short *)tempIndex)[0];
	tempIndex += 2;
	unsigned int srcip = ntohl(*((unsigned int *)tempIndex));
	tempIndex += 4;
	unsigned int dstip = ntohl(*((unsigned int *)tempIndex));

	if (version != 4){
		ip_DiscardPkt(pBuffer, STUD_IP_TEST_VERSION_ERROR);
		return 1;
	}
	if (IHL < 5){
		ip_DiscardPkt(pBuffer, STUD_IP_TEST_HEADLEN_ERROR);
		return 1;
	}
	if (ttl == 0){
		ip_DiscardPkt(pBuffer, STUD_IP_TEST_TTL_ERROR);
		return 1;
	}
	if (dstip != getIpv4Address() && dstip != 0xffffffff){
		ip_DiscardPkt(pBuffer, STUD_IP_TEST_DESTINATION_ERROR);
		return 1;
	}

	// Fast algo from RFC1071
	unsigned int checksum = 0;
	for (int i = 0; i < 20; i += 2){
		checksum += (pBuffer[i] & 0xff) << 8;
		checksum += (pBuffer[i + 1] & 0xff);
	}
	while (checksum >> 16)
		checksum = (checksum >> 16) + checksum;
	checksum = (~checksum) & 0xffff;
	if (checksum != 0){
		ip_DiscardPkt(pBuffer, STUD_IP_TEST_CHECKSUM_ERROR);
		return 1;
	}

	ip_SendtoUp(pBuffer, (int) length);
	return 0;
}

int stud_ip_Upsend(char *pBuffer,unsigned short len,unsigned int srcAddr,
           unsigned int dstAddr,byte protocol,byte ttl)
{
	char *ipPacket = new char[len + 20];

	memset(ipPacket, 0, len + 20);
	ipPacket[0] = 0x45; // version & headlen
	unsigned short totallen = htons(len + 20);
	// memcpy(buf + 2, &ttllen, sizeof(unsigned short));
	// use a code block
	{
		unsigned short *tmp = (unsigned short *)(ipPacket + 2);
		tmp[0] = totallen;
	}

	ipPacket[8] = ttl;
	ipPacket[9] = protocol;

	unsigned int srcadd = htonl(srcAddr);
	unsigned int dstadd = htonl(dstAddr);
	{
		unsigned int *tmp = (unsigned int *)(ipPacket + 12);
		tmp[0] = srcadd;
		tmp[1] = dstadd;
	}

	unsigned int checksum = 0;
	for (int i = 0; i < 20; i += 2){
		checksum += (ipPacket[i] & 0xff) << 8;
		checksum += (ipPacket[i + 1] & 0xff);
		checksum %= 65535;
		// printf("checksum: %x, num1: %x, num2: %x\n", checksum, (buf[i] & 0xff) << 8, (buf[i + 1] & 0xff));
	}
	unsigned short res = htons(0xffff - (unsigned short)checksum);
	{
		unsigned short *tmp = (unsigned short *)(ipPacket + 10);
		tmp[0] = res;
	}
	
	// copy the content in pBuffer to ipPacket content.
	memcpy(ipPacket + 20, pBuffer, len);
	ip_SendtoLower(ipPacket, (int)(len + 20));
	return 0;
}