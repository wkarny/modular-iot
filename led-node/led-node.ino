#include <RF24.h>
#include <EEPROM.h>
#define C_ADDRESS 0
#define P_ADDRESS 1
#define LED_PIN 5

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

struct __attribute__((packed)) attach_request
{
  uint16_t nid;
  uint64_t rpipe;
  uint64_t wpipe;
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
  //Led Setup
    pinMode(LED_PIN,OUTPUT);
  //Led Setup

  radio.begin();
  Serial.begin(115200);
  radio.setPALevel(RF24_PA_LOW);
  char flag='N';
  //
  EEPROM.put(C_ADDRESS,'N');
  //
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
  Serial.println("Size of enum :");
  //Serial.println(sizeof(enum message_type));

  subscribe_list=new topiclinkedlist;
  publish_list=new topiclinkedlist;
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
    if(subscribe_list->next==NULL){
      //Subscribe to a topic
      message m;
      m.type=SUB_TP_REQ;
      m.data.sub_tp_req.t.type=TP_LED;
      m.data.sub_tp_req.nid=NodeID;
      radio.stopListening();
      radio.write(&m,sizeof(message));
      radio.startListening();
      while(!radio.available());            //waiting for responce
      radio.read(&m,sizeof(message));       //reading responce
      if(m.type==SUB_TP_RES){
        topiclinkedlist *p=subscribe_list;
        for(;p->next!=NULL;p->next=p->next->next);
        p->next=new topiclinkedlist;
        p->next->t.tid=m.data.sub_tp_res.t.tid;
      }
      else{
        Serial.println("SUB_TP_RES : Failed");
      }

    }
    else{
      for(topiclinkedlist *p=subscribe_list->next;p!=NULL;p=p->next){
        message m;
        m.type=GET_TP_UP_REQ;
        m.data.get_tp_up_req.tid=p->t.tid;
        m.data.get_tp_up_req.nid=NodeID;
        radio.stopListening();
        radio.write(&m,sizeof(message));
        radio.startListening();
        while(!radio.available());           //waiting for responce
        radio.read(&m,sizeof(message));      //reading responce
        if(m.type==GET_TP_UP_RES){
          int lastdata=m.data.get_tp_up_res.tdata;
          Serial.print("GOT Data: ");
          Serial.println(lastdata);
          if(lastdata==55)                  //the value 55 is used for on
            digitalWrite(LED_PIN,HIGH);
          else if(lastdata==75)             //the value 75 is used for off
            digitalWrite(LED_PIN,LOW);
        }
        else{
        Serial.println("GET_TP_UP_RES : Failed");
      }
      }
    }
    if(publish_list==NULL){
      //Create a publishing topic
    }

  }
  delay(500);
}







