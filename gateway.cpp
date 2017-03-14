#include<iostream>
#include<cstdio>
#include<cstdlib>
#include<RF24/RF24.h>
using namespace std;

enum message_type { ADD_REQ, ATH_REQ, ATH_RES };
enum sensor_type {LED_ACTUATOR, TEMP_SENSOR, POWER_SWITCH_ACTUATOR};

struct add_node
{
  int nid;
  uint8_t lpipe;
};

struct attach_request
{
  uint64_t rpipe;
  uint64_t wpipe;
  int nid;
};

struct attach_respond
{
  int res;
  int nid;
};

typedef struct 
{
  int tid;
  enum sensor_type type ;
  char name[20];
} Topic;

struct create_topic
{
  Topic t;
};

struct subscribe_topic_req
{
  Topic t;
  int nid;
};

struct subscribe_topic_res
{
  Topic t;
  int nid;
};

struct publish_topic
{
  int tid;
  int nid;
  int tdata;
};

struct topic_update_req{
  int tid;
  int nid;
};

struct topic_update_res
{
  int tid;
  int tdata;
};


typedef struct message_t
{
  enum message_type type;
  union {
    struct add_node add_req;
    struct attach_request ath_req;
    struct attach_respond ath_res;

  } data;
} message;

struct topiclinkedlist 
{
   Topic t;
   struct topiclinkedlist *next;
};

RF24 radio(22,0);


int main(){
  radio.begin();
  uint64_t gotAddress=0xAABBCC0011LL;
  uint64_t readingList[1]={0xAA00112233LL};
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
