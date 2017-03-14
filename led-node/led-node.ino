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


int nodeMode=0;

struct topiclinkedlist *subscribe_list=NULL,*publish_list=NULL;  
// Lists for subscribe & publish topic

int NodeID=0x00AA;
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
    //eeAddress=sizeof(char);
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
      message m;
      radio.read(&m,sizeof(message));  //Accepting attach request
      if(m.type==ATH_REQ){
        wPipe=m.data.ath_req.wpipe;
        Serial.println("Got Pipe Address");
        for(int i=4;i>=0;i--)
          Serial.print((uint8_t)(wPipe>>i*8),HEX);
        Serial.println();
        radio.openWritingPipe(wPipe);
        Serial.println("Opened Writing Pipe");
        radio.stopListening();
        m.type=ATH_RES;
        m.data.ath_res.res=0xABAB;
        radio.write(&m,sizeof(message));
        Serial.println("Sent Responce");
        radio.startListening();
        nodeMode=1;          //Setting to connected mode
        EEPROM.put(C_ADDRESS,'Y');
        EEPROM.put(P_ADDRESS,wPipe);
      }
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







