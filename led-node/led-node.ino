#include <RF24.h>
#include <EEPROM.h>
#define C_ADDRESS 0
#define P_ADDRESS 1

/**
    This code is for a sensor module connected to a gateway.
    It uses EEPROM memory to save & retrive allocated writing pipe addresses,
    whereas the reading pipe address are fixed.

    Note : Here I used this node as LED on/off actuator.

**/


struct add_node
{
  int nid;
  uint8_t lpipe;
};

struct attach_request
{
  uint64_t rPipe;
  uint64_t wPipe;
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
  enum type {LED_ACTUATOR, TEMP_SENSOR, POWER_SWITCH_ACTUATOR};
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
  enum { ADD_REQ, ATH_REQ, ATH_RES,  } type;
  union {
    struct add_node r;
    struct attach_request r;
    struct attach_respond r;

  } data;
} message;

struct topiclinkedlist 
{
   Topic t;
   struct topiclinkedlist *next;
};


int nodeMode=0;

topiclinkedlist *subscribe_list,*publish_list;  // Lists for subscribe & publish topic

subscribe_list=NULL;
publish_list=NULL;

uint64_t rPipe=0xAABBCC0011LL; //Address for the reading pipe
uint64_t wPipe; //Address of writing pipe

RF24 radio(9,10);  //Set as per your config

void setup(){
  radio.begin();
  Serial.begin(115200);
  radio.setPALevel(RF24_PA_LOW);
  char flag='N';
  EEPROM.get(C_ADDRESS,flag);
  if(flag=='Y'){
    Serial.println("Flag = Y");
    nodeMode=1;
    eeAddress=sizeof(char);
    EEPROM.get(P_ADDRESS,wPipe);
    Serial.println("Got wPipe");
    radio.openReadingPipe(1,rPipe);
    radio.openWritingPipe(wPipe);
  }
  else{
    nodeMode=0;
    Serial.println("Flag != Y");
    radio.openReadingPipe(1,rPipe);
  }
  radio.startListening();
}

void loop(){
  if(nodeMode==0){
    Serial.println("Waiting for Connection");
    if(radio.available()){
      char c='c';
      radio.read(&wPipe,sizeof(wPipe));
      Serial.println("Got Pipe Address");
      int i=sizeof(uint64_t);
      for(i=i-2;i>=0;i-=sizeof(int))
        Serial.print((int)(wPipe>>i*8),HEX);
      Serial.println();
      radio.openWritingPipe(wPipe);
      Serial.println("Opened Writing Pipe");
      radio.stopListening();
      radio.write(&c,sizeof(c));
      Serial.println("Sent Responce");
      radio.startListening();
      nodeMode=1;          //Setting to connected mode
      EEPROM.put(C_ADDRESS,'Y');
      EEPROM.put(P_ADDRESS,wPipe);
    }
  }
  else{
    if(subscribe_list==NULL){

    }
    if(publish_list==NULL){

    }
  }
  delay(500);
}







