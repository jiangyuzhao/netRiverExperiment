// 1500012945 Yuzhao Jiang
#include "sysinclude.h"
#include <cstring>
#include <iostream>
#include <fstream>
#include <queue>

extern void SendFRAMEPacket(unsigned char* pData, unsigned int len);

#define WINDOW_SIZE_STOP_WAIT 1
#define WINDOW_SIZE_BACK_N_FRAME 4

// native struct define
typedef enum {DATA,ACK,NAK} frame_kind; 
typedef struct frame_head 
{ 
	frame_kind kind; //帧类型 
	unsigned int seq; //序列号 
	unsigned int ack; //确认号 
	unsigned char data[100]; //数据 
}; 
typedef struct frame 
{ 
	frame_head head; //帧头 
	unsigned int size; //数据的大小 
}; 

/*
* 停等
*/

int stud_slide_window_stop_and_wait(char *pBuffer, int bufferSize, UINT8 messageType)
{
	static frame* frameWindow[WINDOW_SIZE_STOP_WAIT];
	static int windowFrameLen[WINDOW_SIZE_STOP_WAIT];
	// when you see debug file, you will find the frame's seq is begin from 1.
	// so if you assign upper and lower 0, when you do accept, 
	// you will free something shouldn't be free. You had better make upper and lower
	// correspond to the frame the experimental system sent to you.
	static int upper = 1, lower = 1;
	static queue<frame *> waitingFrame;
	static queue<int> waitingFrameLen;
	static ofstream out("stopWait.txt");

	// temp var table
	frame_head *header = (frame_head *)pBuffer;
 	int seq = ntohl(header->seq);
 	int ack = ntohl(header->ack);
 	frame_kind kind = (frame_kind)(ntohl((u_long)header->kind));

 	if (messageType == MSG_TYPE_TIMEOUT){
 		int timeoutSeq = ntohl(*( (unsigned int *)pBuffer ));

 		out << "超时/n" <<" timeoutseq = "<< timeoutSeq <<endl;
		out << "全部重发"<<endl;
		out.flush();

 		for (int i = lower; i < upper; ++i){
 			SendFRAMEPacket((unsigned char *)frameWindow[i % WINDOW_SIZE_STOP_WAIT],
 				windowFrameLen[i % WINDOW_SIZE_STOP_WAIT]);
 		}
 	}
 	else if(messageType == MSG_TYPE_SEND){
 		//log package
		out << "发送/n" << "kind = " << kind <<" seq = "<<seq<<" ack = "<<ack<<" size = "<<bufferSize<<endl;
 		
 		frame *frameToWait = new frame;
 		*frameToWait = *(frame *)pBuffer; 
 		// window full
 		if (upper - lower >= WINDOW_SIZE_STOP_WAIT){
 			waitingFrame.push(frameToWait);
 			waitingFrameLen.push(bufferSize);

 			out<<"入队等待"<<endl;
   			out.flush();
 		}
 		// window not full
 		else{
 			frameWindow[upper % WINDOW_SIZE_STOP_WAIT] = frameToWait;
 			windowFrameLen[upper % WINDOW_SIZE_STOP_WAIT] = bufferSize;
 			++upper;
 			SendFRAMEPacket((unsigned char *)pBuffer, bufferSize);

 			out<<"直接发送"<<endl;
  			out.flush();
 		}
 	}
 	else if (messageType == MSG_TYPE_RECEIVE){
 		//log package
  		out << "接收/n" << "kind = " << kind <<" seq = "<<seq<<" ack = "<<ack<<endl;
  		if (ack < lower || ack >= upper)
  			return 1;
  		if (kind == NAK){
  			// deny the frame
  			//log package
   			out << "否定确认帧, 重发"<<endl;
   			out.flush();
   			// maybe i can reach ack
   			for (int i = lower; i < ack; ++i){
   				SendFRAMEPacket((unsigned char *)frameWindow[i % WINDOW_SIZE_STOP_WAIT], windowFrameLen[i % WINDOW_SIZE_STOP_WAIT]);
   			}
			// while(ack<upper){
			// 	SendFRAMEPacket((unsigned char*)frameBuffer[ack%WINDOW_SIZE_STOP_WAIT],frameLenBuffer[ack%WINDOW_SIZE_STOP_WAIT]);
			// }
			return 0;
  		}
  		// kind := ACK
  		// lower may not reach ack
  		for (; lower <= ack; ++lower){
  			out << "accept " << lower << " package." << endl;
  			delete frameWindow[lower % WINDOW_SIZE_STOP_WAIT];
  		}

  		while (!waitingFrame.empty() && upper - lower < WINDOW_SIZE_STOP_WAIT){
  			frameWindow[upper % WINDOW_SIZE_STOP_WAIT] = waitingFrame.front();
  			waitingFrame.pop();
  			windowFrameLen[upper % WINDOW_SIZE_STOP_WAIT] = waitingFrameLen.front();
  			waitingFrameLen.pop();
  			SendFRAMEPacket((unsigned char *)frameWindow[upper % WINDOW_SIZE_STOP_WAIT], 
  				windowFrameLen[upper % WINDOW_SIZE_STOP_WAIT]);
  			++upper;
  		}
 	}
	return 0;
}

/*
* 回退N
*/

int stud_slide_window_back_n_frame(char *pBuffer, int bufferSize, UINT8 messageType)
{
	static frame* frameWindow[WINDOW_SIZE_BACK_N_FRAME];
	static int windowFrameLen[WINDOW_SIZE_BACK_N_FRAME];
	static int upper = 1, lower = 1;
	static queue<frame *> waitingFrame;
	static queue<int> waitingFrameLen;
	static ofstream out("backN.txt");

	// temp var table
	frame_head *header = (frame_head *)pBuffer;
 	int seq = ntohl(header->seq);
 	int ack = ntohl(header->ack);
 	frame_kind kind = (frame_kind)(ntohl((u_long)header->kind));

 	if (messageType == MSG_TYPE_TIMEOUT){
 		int timeoutSeq = ntohl(*( (unsigned int *)pBuffer ));

 		out << "超时/n" <<" timeoutseq = "<< timeoutSeq <<endl;
		out << "全部重发"<<endl;
		out.flush();

 		for (int i = lower; i < upper; ++i){
 			SendFRAMEPacket((unsigned char *)frameWindow[i % WINDOW_SIZE_BACK_N_FRAME],
 				windowFrameLen[i % WINDOW_SIZE_BACK_N_FRAME]);
 		}
 	}
 	else if(messageType == MSG_TYPE_SEND){
 		//log package
		out << "发送/n" << "kind = " << kind <<" seq = "<<seq<<" ack = "<<ack<<" size = "<<bufferSize<<endl;
 		
 		frame *frameToWait = new frame;
 		*frameToWait = *(frame *)pBuffer; 
 		// window full
 		if (upper - lower >= WINDOW_SIZE_BACK_N_FRAME){
 			waitingFrame.push(frameToWait);
 			waitingFrameLen.push(bufferSize);

 			out<<"入队等待"<<endl;
   			out.flush();
 		}
 		// window not full
 		else{
 			frameWindow[upper % WINDOW_SIZE_BACK_N_FRAME] = frameToWait;
 			windowFrameLen[upper % WINDOW_SIZE_BACK_N_FRAME] = bufferSize;
 			++upper;
 			SendFRAMEPacket((unsigned char *)pBuffer, bufferSize);

 			out<<"直接发送"<<endl;
  			out.flush();
 		}
 	}
 	else if (messageType == MSG_TYPE_RECEIVE){
 		//log package
  		out << "接收/n" << "kind = " << kind <<" seq = "<<seq<<" ack = "<<ack<<endl;
  		if (ack < lower || ack >= upper)
  			return 1;
  		if (kind == NAK){
  			// deny the frame
  			//log package
   			out << "否定确认帧, 重发"<<endl;
   			out.flush();
   			// maybe i can reach ack
   			for (int i = lower; i < ack; ++i){
   				SendFRAMEPacket((unsigned char *)frameWindow[i % WINDOW_SIZE_BACK_N_FRAME], windowFrameLen[i % WINDOW_SIZE_BACK_N_FRAME]);
   			}
			// while(ack<upper){
			// 	SendFRAMEPacket((unsigned char*)frameBuffer[ack%WINDOW_SIZE_BACK_N_FRAME],frameLenBuffer[ack%WINDOW_SIZE_BACK_N_FRAME]);
			// }
			return 0;
  		}
  		// kind := ACK
  		// lower may not reach ack
  		for (; lower <= ack; ++lower){
  			out << "accept " << lower << " package." << endl;
  			delete frameWindow[lower % WINDOW_SIZE_BACK_N_FRAME];
  		}

  		while (!waitingFrame.empty() && upper - lower < WINDOW_SIZE_BACK_N_FRAME){
  			frameWindow[upper % WINDOW_SIZE_BACK_N_FRAME] = waitingFrame.front();
  			waitingFrame.pop();
  			windowFrameLen[upper % WINDOW_SIZE_BACK_N_FRAME] = waitingFrameLen.front();
  			waitingFrameLen.pop();
  			SendFRAMEPacket((unsigned char *)frameWindow[upper % WINDOW_SIZE_BACK_N_FRAME], 
  				windowFrameLen[upper % WINDOW_SIZE_BACK_N_FRAME]);
  			++upper;
  		}
 	}
	return 0;
}

/*
* 选择重传
*/
int stud_slide_window_choice_frame_resend(char *pBuffer, int bufferSize, UINT8 messageType)
{
	static frame* frameWindow[WINDOW_SIZE_STOP_WAIT];
	static int windowFrameLen[WINDOW_SIZE_STOP_WAIT];
	static int upper = 1, lower = 1;
	static queue<frame *> waitingFrame;
	static queue<int> waitingFrameLen;
	static ofstream out("select.txt");

	// temp var table
	frame_head *header = (frame_head *)pBuffer;
 	int seq = ntohl(header->seq);
 	int ack = ntohl(header->ack);
 	frame_kind kind = (frame_kind)(ntohl((u_long)header->kind));

 	if (messageType == MSG_TYPE_TIMEOUT){
 		int timeoutSeq = ntohl(*( (unsigned int *)pBuffer ));

 		out << "超时/n" <<" timeoutseq = "<< timeoutSeq <<endl;
 		out << "重发超时帧"<<endl;
		out.flush();
		// resend directly
		SendFRAMEPacket((unsigned char *)frameWindow[timeoutSeq % WINDOW_SIZE_BACK_N_FRAME],
			windowFrameLen[timeoutSeq % WINDOW_SIZE_BACK_N_FRAME]);
		return 0;
 	}
 	else if(messageType == MSG_TYPE_SEND){
 		//log package
		out << "发送/n" << "kind = " << kind <<" seq = "<<seq<<" ack = "<<ack<<" size = "<<bufferSize<<endl;
 		
 		frame *frameToWait = new frame;
 		*frameToWait = *(frame *)pBuffer; 
 		// window full
 		if (upper - lower >= WINDOW_SIZE_BACK_N_FRAME){
 			waitingFrame.push(frameToWait);
 			waitingFrameLen.push(bufferSize);

 			out<<"入队等待"<<endl;
   			out.flush();
 		}
 		// window not full
 		else{
 			frameWindow[upper % WINDOW_SIZE_BACK_N_FRAME] = frameToWait;
 			windowFrameLen[upper % WINDOW_SIZE_BACK_N_FRAME] = bufferSize;
 			++upper;
 			SendFRAMEPacket((unsigned char *)pBuffer, bufferSize);

 			out<<"直接发送"<<endl;
  			out.flush();
 		}
 	}
 	else if (messageType == MSG_TYPE_RECEIVE){

		if( ack<lower || ack >=upper )
			return 1;

		if (kind == NAK){
			// resend a frame
			out << "重发一帧"<<endl;
			out.flush();

			SendFRAMEPacket((unsigned char *)frameWindow[ack % WINDOW_SIZE_BACK_N_FRAME],
				 windowFrameLen[ack % WINDOW_SIZE_BACK_N_FRAME]);
			return 0;
		}
		// kind := ACK
  		// lower may not reach ack(ack may not need to free)
  		for (; lower <= ack; ++lower){
  			out << "accept " << lower << " package." << endl;
  			delete frameWindow[lower % WINDOW_SIZE_BACK_N_FRAME];
  		}

  		while (!waitingFrame.empty() && upper - lower < WINDOW_SIZE_BACK_N_FRAME){
  			frameWindow[upper % WINDOW_SIZE_BACK_N_FRAME] = waitingFrame.front();
  			waitingFrame.pop();
  			windowFrameLen[upper % WINDOW_SIZE_BACK_N_FRAME] = waitingFrameLen.front();
  			waitingFrameLen.pop();
  			SendFRAMEPacket((unsigned char *)frameWindow[upper % WINDOW_SIZE_BACK_N_FRAME], 
  				windowFrameLen[upper % WINDOW_SIZE_BACK_N_FRAME]);
  			++upper;
  		}

 	}
	return 0;
}

