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


using namespace std;

#define MAIN_SERVER_IP_ADDR "192.168.0.106"
#define MAIN_SERVER_PORT_NUM 8080

//message type
#define ADD_REQ 0
#define ATH_REQ 1
#define ATH_RES 2
#define SUB_TP_REQ 3
#define SUB_TP_RES 4
#define GET_TP_UP_REQ 5
#define GET_TP_UP_RES 6


//topic type
#define TP_LED 0
#define TP_TEMP 1
#define TP_POWER_SWITCH 2

/**
    This code is for a sensor module connected to a gateway.
    It uses EEPROM memory to save & retrive allocated writing pipe addresses,
    whereas the reading pipe address are fixed.

    Note : Here I used this node as LED on/off actuator.

**/

// message structures start

struct __attribute__((packed)) add_node
{
  uint16_t nid;
  uint64_t wpipe;
};

struct __attribute__((packed)) attach_request
{
  uint16_t nid;
  uint64_t rpipe;
  uint64_t wpipe;
};

struct __attribute__((packed)) attach_respond
{
  uint16_t res;
  uint16_t nid;
};

typedef struct __attribute__((packed)) topic_t
{
  uint16_t tid;
  uint8_t type ;
} Topic;

struct __attribute__((packed)) create_topic
{
  Topic t;
};

struct __attribute__((packed)) subscribe_topic_req
{
  uint16_t nid;
  Topic t;
};

struct __attribute__((packed)) subscribe_topic_res
{
  uint16_t nid;
  Topic t;

};

struct __attribute__((packed)) publish_topic
{
  uint16_t tid;
  uint16_t nid;
  uint16_t tdata;
};

struct __attribute__((packed)) get_topic_update_req{
  uint16_t tid;
  uint16_t nid;
};

struct __attribute__((packed)) get_topic_update_res
{
  uint16_t tid;
  uint16_t tdata;
};


typedef struct __attribute__((packed)) message_t
{
  uint8_t type;
  union {
    struct add_node add_req;
    struct attach_request ath_req;
    struct attach_respond ath_res;
    struct subscribe_topic_req sub_tp_req;
    struct subscribe_topic_res sub_tp_res;
    struct get_topic_update_req get_tp_up_req;
    struct get_topic_update_res get_tp_up_res;

  } data;
} message;

//message structure end
struct topiclinkedlist 
{
   Topic t;
   uint16_t lastdata;
   struct topiclinkedlist *next;
};

struct topiclinkedlist *TopicList;

RF24 radio(22,0);

uint16_t last_tid=200;           //should be replaced with non-volatile storage

void socket_thread(){
    struct sockaddr_in address;
    int sock = 0, valread;
    struct sockaddr_in serv_addr;
    char *hello = "UserID: RPI01 PassWD: 2810".c_str();
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
    char *ack="ACK".c_str();
    memset(buffer,0,sizeof(buffer));
    while(1){
      valread = read( sock , buffer, 1024);          //Waiting for Msg
      cout<<"Recived from Socket: "<<buffer<<endl;
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

  sock_thr.join();

  while(1){          //Gateway reset condition should be given
    if(radio.available()){
      message m;
      radio.read(&m,sizeof(message));
      if(m.type==SUB_TP_REQ){
        cout<<"Got: SUB_TP_REQ"<<endl;
        int nid=m.data.sub_tp_req.nid;
        topiclinkedlist *p;
        for(p=TopicList;p->next!=NULL;p->next=p->next->next);
        p->next=new topiclinkedlist;
        p->next->next=NULL;
        p->next->t.tid=++last_tid;
        p->next->t.type=m.data.sub_tp_req.t.type;
        cout<<"New Topic Created"<<endl;
        m.type=SUB_TP_RES;
        m.data.sub_tp_res.nid=nid;
        m.data.sub_tp_res.t.tid=last_tid;
        m.data.sub_tp_res.t.type=p->next->t.type;
        radio.stopListening();
        radio.write(&m,sizeof(message));       //Shold be directed perticular to that node
        radio.startListening();
        cout<<"SUB_TP_RES : Sent"<<endl;
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
   
}
