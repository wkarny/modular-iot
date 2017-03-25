#include <iostream>
#include <cstdio>
#include <ctime>
#include <cstdlib>
#include <RF24/RF24.h>
#include <thread>

#include <sys/socket.h>
#include <stdlib.h>
#include <netinet/in.h>
#include <unistd.h>
#include <sys/types.h>
#include <netdb.h>
#include <string.h>
#include <arpa/inet.h>

#include <string>
#include <mutex>
#include <list>

#include "led-node/msgstruct.h"

using namespace std;

#define MAIN_SERVER_IP_ADDR "192.168.0.106"
#define MAIN_SERVER_PORT_NUM 8080



struct topiclinkedlist 
{
   Topic t;
   uint16_t lastdata;
   struct topiclinkedlist *next;
};

struct topiclinkedlist *TopicList;

RF24 radio(22,0);
mutex mu;
list<string> mylist;

uint16_t last_tid=200;           //should be replaced with non-volatile storage

void enqueueList(string str){
  lock_guard<mutex> gaurd(mu);
  mylist.push_back(str);
}

int isEmptyList(){
  lock_guard<mutex> gaurd(mu);
  if(mylist.empty()){
    return 1;
  }
  else
    return 0;
}

string dequeueList(){
  lock_guard<mutex> gaurd(mu);
  return(mylist.pop_front());
}

void socket_thread(){
    struct sockaddr_in address;
    int sock = 0, valread;
    struct sockaddr_in serv_addr;
    char *hello = "UserID: RPI01 PassWD: 2810";
    char buffer[1024] = {0};
    if ((sock = socket(AF_INET, SOCK_STREAM, 0)) < 0)   //Should be replaced by while
    {
      cout<<"Socket creation error\n";
      return;
    }
  
    memset(&serv_addr, '0', sizeof(serv_addr));
  
    serv_addr.sin_family = AF_INET;
    serv_addr.sin_port = htons(MAIN_SERVER_PORT_NUM);                  //Port Address
      
    if(inet_pton(AF_INET, MAIN_SERVER_IP_ADDR, &serv_addr.sin_addr)<=0)  //Server IP Address
    {
      cout<<"Invalid address\n";
      return;
    }
  
    if (connect(sock, (struct sockaddr *)&serv_addr, sizeof(serv_addr)) < 0)
    {
      cout<<"Connection Failed \n";
      return;
    }
    send(sock , hello , strlen(hello) , 0 );
    cout<<"Auth message sent\n";
    char *ack="ACK";
    memset(buffer,0,sizeof(buffer));
    while(1){
      valread = read( sock , buffer, 1024);          //Waiting for Msg
      cout<<"Recived from Socket: "<<buffer<<endl;
      enqueueList(string(buffer));                   //Enqueing in the list
      memset(buffer,0,sizeof(buffer));
      send(sock,ack,strlen(ack),0);                 //Sending ACK
    }

}

int main(){
  radio.begin();
  TopicList=new topiclinkedlist;
  TopicList->next=NULL;

  thread sock_thr(&socket_thread);
  //cout<<"Size of enum :"<<sizeof(enum message_type)<<endl;
  uint64_t gotAddress=0xAABBCC0011LL;                //nrf24 needs 5 bytes of address
  uint64_t readingList[1]={0xAA11223344LL};
  radio.openWritingPipe(gotAddress);
  cout<<"Opened Writing Pipe"<<endl;
  radio.openReadingPipe(1,readingList[0]);
  cout<<"Opened Reading Pipe"<<endl;
  radio.stopListening();
  message m;
  m.type=ATH_REQ;
  m.data.ath_req.nid=0;
  m.data.ath_req.rpipe=0;
  m.data.ath_req.wpipe=readingList[0];
  radio.write(&m,sizeof(message));
  cout<<"Sent the address"<<endl;
  radio.startListening();
  while(!radio.available());
  cout<<"Got someting"<<endl;
  radio.read(&m,sizeof(message));
  if(m.type==ATH_RES){
    cout<<"Successfully connected"<<endl;
    cout<<"Got respond id : "<<std::hex<<m.data.ath_res.res<<endl;
  }
  else
    cout<<"Unexpected data recived"<<endl;


  while(1){          //Gateway reset condition should be given
    if(radio.available()){
      message m;
      radio.read(&m,sizeof(message));
      if(m.type==CRT_TP_REQ){
        cout<<"Got: CRT_TP_REQ"<<endl;
        int nid=m.data.crt_tp_req.nid;
        topiclinkedlist *p;
        for(p=TopicList;p->next!=NULL;p->next=p->next->next);
        p->next=new topiclinkedlist;
        p->next->next=NULL;
        p->next->t.tid=++last_tid;
        p->next->t.type=m.data.crt_tp_req.t.type;
        cout<<"New Topic Created"<<endl;
        m.type=CRT_TP_RES;
        m.data.crt_tp_res.nid=nid;
        m.data.crt_tp_res.t.tid=last_tid;
        m.data.crt_tp_res.t.type=p->next->t.type;
        m.data.crt_tp_res.res=ACK;
        radio.stopListening();
        radio.write(&m,sizeof(message));       //Shold be directed perticular to that node
        radio.startListening();
        cout<<"CRT_TP_RES : Sent"<<endl;
      }
      else if(m.type==GET_TP_UP_REQ){
        int my_time=(int)time(0);
        if(my_time%2==0){
          m.type=GET_TP_UP_RES;
          m.data.get_tp_up_res.tdata=55;
          radio.stopListening();
          radio.write(&m,sizeof(message));
          radio.startListening();
          cout<<"SUB_TP_RES 55 : Sent"<<endl;
        }
        else{
          m.type=GET_TP_UP_RES;
          m.data.get_tp_up_res.tdata=75;
          radio.stopListening();
          radio.write(&m,sizeof(message));
          radio.startListening();
          cout<<"SUB_TP_RES 75 : Sent"<<endl;
        }
      }
    }
  }
   sock_thr.join();
}
