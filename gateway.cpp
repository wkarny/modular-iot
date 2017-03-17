#include<iostream>
#include<cstdio>
#include<cstdlib>
#include<RF24/RF24.h>
using namespace std;

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

struct add_node
{
  uint16_t nid;
  uint64_t wpipe;
};

struct attach_request
{
  uint64_t rpipe;
  uint64_t wpipe;
  uint16_t nid;
};

struct attach_respond
{
  uint16_t res;
  uint16_t nid;
};

typedef struct 
{
  uint16_t tid;
  uint8_t type ;
} Topic;

struct create_topic
{
  Topic t;
};

struct subscribe_topic_req
{
  uint16_t nid;
  Topic t;
};

struct subscribe_topic_res
{
  uint16_t nid;
  Topic t;

};

struct publish_topic
{
  uint16_t tid;
  uint16_t nid;
  uint16_t tdata;
};

struct get_topic_update_req{
  uint16_t tid;
  uint16_t nid;
};

struct get_topic_update_res
{
  uint16_t tid;
  uint16_t tdata;
};


typedef struct message_t
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
   struct topiclinkedlist *next;
};

RF24 radio(22,0);


int main(){
  radio.begin();
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
  m.data.ath_req.wpipe=readingList[0];
  radio.write(&m,sizeof(message));
  cout<<"Sent the address"<<endl;
  radio.startListening();
  while(!radio.available());
  cout<<"Got someting"<<endl;
  radio.read(&m,sizeof(message));
  if(m.type==ATH_RES){
    cout<<"Successfully connected"<<endl;
    cout<<"Got respond id : "<<m.data.ath_res.res<<endl;
  }
  else
    cout<<"Unexpected data recived"<<endl;
   
}
