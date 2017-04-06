#include <RF24.h>
#include <EEPROM.h>

#include "msgstruct.h"

#define C_ADDRESS 0
#define P_ADDRESS 1
#define LED_PIN 5



struct topiclinkedlist 
{
   Topic t;
   struct topiclinkedlist *next;
};


int nodeMode=0;

struct topiclinkedlist *subscribe_list=NULL,*publish_list=NULL;  
// Lists for subscribe & publish topic

int NodeID=0x00AA;
uint64_t rPipe=86; //Address for the reading pipe
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
  subscribe_list->next=NULL;
  publish_list=new topiclinkedlist;
  publish_list->next=NULL;
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
  else{                                   //nodeMode==1
    if(subscribe_list->next==NULL){
      //Subscribe to a topic, Here the LED node requires only one topic
      message m;
      m.type=CRT_TP_REQ;
      m.data.crt_tp_req.t.type=TP_LED;
      m.data.crt_tp_req.nid=NodeID;
      radio.stopListening();
      radio.write(&m,sizeof(message));
      radio.startListening();
      Serial.println("CRT_TP_REQ : Sent");
      while(!radio.available()) Serial.println("Waiting for : CRT_TP_RES"); //waiting for responce
      radio.read(&m,sizeof(message));       //reading responce
      if(m.type==CRT_TP_RES){
        Serial.println("CRT_TP_RES : Success");
        if(m.data.crt_tp_res.res==ACK){
          topiclinkedlist *p=subscribe_list;
          p->next=new topiclinkedlist;
          p->next->t.tid=m.data.crt_tp_res.t.tid;
          p->next->next=NULL;
          Serial.println("Topic created");
        }
        else{
          Serial.println("CRT_TP_RES: Failed (NACK)");
        }
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
        Serial.println("GET_TP_UP_REQ : Sent");
        while(!radio.available());           //waiting for responce
        radio.read(&m,sizeof(message));      //reading responce
        if(m.type==GET_TP_UP_RES){
          Serial.println("GET_TP_UP_RES : Success");
          int lastdata=m.data.get_tp_up_res.tdata;
          Serial.print("GOT Data: ");
          Serial.println(lastdata);
          if(lastdata==55)                  //the value 55 is used for on
            digitalWrite(LED_PIN,HIGH);
          else if(lastdata==75)             //the value 75 is used for off
            digitalWrite(LED_PIN,LOW);
          Serial.println("Dicition Taken");
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







