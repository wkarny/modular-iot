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
#include <unordered_map>

#include "led-node/msgstruct.h"

using namespace std;

//#define MAIN_SERVER_IP_ADDR "192.168.0.106"
#define MAIN_SERVER_PORT_NUM 8080



// struct topiclinkedlist 
// {
//    Topic t;
//    uint16_t lastdata;
//    struct topiclinkedlist *next;
// };

// struct topiclinkedlist *TopicList;

//RF24 radio(22,0);


/* Topic manager class start */
class TopicManager
{
  unordered_map<uint16_t,uint32_t> tData;
  unordered_map<uint16_t,uint8_t> tType;
  unordered_map<uint16_t,uint16_t> tNid;
public:
  TopicManager();
  ~TopicManager();
  bool addTopic(uint16_t tid, uint8_t type,uint16_t nid);
  bool find(uint16_t tid);
  uint32_t getData(uint16_t tid);
  uint8_t getType(uint16_t tid);
  bool putData(uint16_t tid, uint32_t data);
  uint16_t getNid(uint16_t tid);
};

bool TopicManager::addTopic(uint16_t tid,uint8_t type,uint16_t nid){
  if(find(tid)==true){
    cout<<"TopicManager : There is already a topic with this tid"<<endl;
    return false;
  }
  else{
    tData[tid]=0;
    tType[tid]=type;
    tNid[tid]=nid;
    return true;
  }
}

bool TopicManager::find(uint16_t tid){
  if(tType.find(tid)==tType.end()){
    return false;  // Not found
  }
  else{
    return true;
  }
}

uint32_t TopicManager::getData(uint16_t tid){
  if(find(tid)){
    return tData[tid];
  }
  else{
    cout<<"getData : Failed"<<endl;
    return 0;
  }
}

uint8_t TopicManager::getType(uint16_t tid){
  if(find(tid)){
    return tType[tid];
  }
  else{
    cout<<"getType : Failed"<<endl;
    return 0;
  }
}

uint16_t TopicManager::getNid(uint16_t tid){
  if(find(tid)){
    return tNid[tid];
  }
  else{
    cout<<"getNid : Failed"<<endl;
    return 0;
  }
}

bool TopicManager::putData(uint16_t tid, uint32_t data){
  if(find(tid)){
    tData[tid]=data;
  }
  else{
    cout<<"putData : Failed, not found tid"<<endl;
  }
}

/* Topic Manager class end */


mutex mu1,mu2;
list<string> queue1;
list<string> queue2;

uint16_t last_tid=200;           //should be replaced with non-volatile storage

void enqueueList(list<string> mylist,mutex mu,string str){
  lock_guard<mutex> gaurd(mu);
  mylist.push_back(str);
}

int isEmptyList(list<string> mylist,mutex mu){
  lock_guard<mutex> gaurd(mu);
  if(mylist.empty()){
    return 1;
  }
  else
    return 0;
}

string dequeueList(list<string> mylist,mutex mu){
  lock_guard<mutex> gaurd(mu);
  string str=mylist.front();
  mylist.pop_front();
  return(str);
}

void socket_thread(string ip_address){
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
      
    if(inet_pton(AF_INET, ip_address.c_str(), &serv_addr.sin_addr)<=0)  //Server IP Address
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
      enqueueList(queue1,mu1,string(buffer));                   //Enqueing in the list
      char *cm;
      cm=strtok(buffer,"+");
      if(strcmp(cm,"PRINT")==0){
        cout<<"Printing list"<<endl;
        for(list<string>::iterator it=queue1.begin();it!=queue1.end();++it)  //Should not access here
          cout<<*it<<endl;
      }
      memset(buffer,0,sizeof(buffer));
      send(sock,ack,strlen(ack),0);                 //Sending ACK
    }

}



class MyRadio
{
public:
  RF24 *radio;
  unordered_map<uint16_t,uint64_t> writing_list;  // <nid,pipe>
  unordered_map<uint16_t,uint64_t> reading_list;  // <nid,pipe>
  unordered_map<uint8_t,uint16_t> node_id;     // <pipe_id,nid>
  uint64_t last_pipe;
  uint16_t last_nid;
  uint8_t last_pipe_num;

  //Member functions
  MyRadio(int ce_pin,int cs_pin);
  ~MyRadio();
  void begin();
  int attachNode(uint64_t pipe);
  void increaseNode(uint64_t pipe);
  int sendMessage(uint16_t nid, message m);
  bool available();
  uint16_t read(message *m);
  /* data */
};

MyRadio::MyRadio(int ce_pin,int cs_pin){
  radio=new RF24(ce_pin,cs_pin);
  last_pipe=0xAA11223344LL;                   //Should be read from File
  last_nid=100;
  last_pipe_num=0;
}
void MyRadio::begin(){
  radio->begin();
}

void MyRadio::increaseNode(uint64_t pipe){
  writing_list[last_nid+1]=pipe;
  reading_list[last_nid+1]=last_pipe+1;
  node_id[last_pipe_num+1]=last_nid+1;
  last_pipe++;
  last_nid++;
  last_pipe_num++;
}

int MyRadio::attachNode(uint64_t pipe){
  if(node_id.size()>=5){
    return 0;
  }
  else{
    radio->openWritingPipe(pipe);
    cout<<"Opened Writing Pipe"<<endl;
    radio.openReadingPipe(last_pipe_num+1,last_pipe+1);
    cout<<"Opened Reading Pipe"<<endl;
    radio->stopListening();
    message m;
    int nodeIDAllocated=last_nid+1;
    m.type=ATH_REQ;
    m.data.ath_req.nid=last_nid+1;
    m.data.ath_req.rpipe=0;
    m.data.ath_req.wpipe=last_pipe+1;
    radio->write(&m,sizeof(message));
    cout<<"Sent the address"<<endl;
    radio->startListening();
    uint8_t pipe_num=0;
    while(pipe_num!=last_pipe_num+1)
      while(!radio->available(pipe_num));            //Should terminate after some time
    cout<<"Got someting"<<endl;
    radio->read(&m,sizeof(message));
    if(m.type==ATH_RES){
      cout<<"Successfully connected"<<endl;
      cout<<"Got respond id : "<<std::hex<<m.data.ath_res.res<<endl;
      increaseNode();
      return nodeIDAllocated;
    }
    else
      cout<<"Unexpected data recived"<<endl;
  }
}

int MyRadio::sendMessage(uint16_t nid,message m){
  if(writing_list.find(nid)==writing_list.end()){
    return 0;
  }
  else{
    radio->stopListening();
    radio->openWritingPipe(writing_list[nid]);
    radio->write(&m,sizeof(m));             // Should handle if unable to send
    radio->startListening();
    return 1;
  }
}

bool MyRadio::available(){
  return(radio->available());
}

uint16_t MyRadio::read(message* m){
  radio->read(m,sizeof(message));
  return(m->nid);
}

void nrf_thread(){
  // TopicList=new topiclinkedlist;
  // TopicList->next=NULL;
  TopicManager tp_man;
  MyRadio sensorNetwork(22,0);
  sensorNetwork.begin();
  while(1){
    if(!isEmptyList(queue1,mu1)){   // For servicing Server Requests
      string str=dequeueList(queue1,mu1);
      char *cm;
      cm=strtok(str.c_str(),"+");
      if(strcmp(cm,"ADQ")==0){   //ADQ start
        char* token[3];
        bool errorInToken=false;
        for(int i=0;i<3;i++){
          cm=strtok(NULL,"+");
          if(cm==NULL){
            cout<<"Error is parsing : Not Enough Tokens"<endl;
            errorInToken=true;
            break;
          }
          token[i]=cm;
        }
        if(!errorInToken){
          int nid=atoi(token[0]);  //Should add extra level of auth using nid
          uint64_t pipe=atol(token[1]);
          int reqid=atoi(token[2]);
          int allocatedNID=0;
          if((allocatedNID=sensorNetwork.attachNode(pipe)!=0){
            cout<<"Successfully node attach"<<endl;
          }
          else{
            cout<<"Unable to Attach"<<endl;
          }
          enqueueList(queue2,mu2,"ADR+"+allocatedNID+"+"+reqid);
        }
        else{
          //Send a NACK
        }
    }            //ADQ end
    else if(strcmp(cm,"PTUR")==0){
        char* token[3];
        bool errorInToken=false;
        for(int i=0;i<3;i++){
          cm=strtok(NULL,"+");
          if(cm==NULL){
            cout<<"Error is parsing : Not Enough Tokens"<endl;
            errorInToken=true;
            break;
          }
          token[i]=cm;
        }
        if(!errorInToken){
          int temp_tid=atoi(token[0]);
          int temp_tdata=atoi(token[1]);
          int temp_reqid=atoi(token[2]);
          if(tp_man.putData(temp_tid,temp_tdata))
            enqueueList(queue2,mu2,"PTUR+"+temp_tid+"+ACK+"+temp_reqid);
          else
            enqueueList(queue2,mu2,"PTUR+"+temp_tid+"+NACK+"+temp_reqid);
        }
    }
    else{
      cout<<"Unrecognised Command"<endl;
    }
  }


  if(sensorNetwork.available()){  // Servicing node requests
    message m;
    int nid=sensorNetwork.read(&m);
    if(m.type==CRT_TP_REQ){
        cout<<"Got: CRT_TP_REQ"<<endl;
        //int nid=m.data.crt_tp_req.nid;
        // topiclinkedlist *p;
        // for(p=TopicList;p->next!=NULL;p->next=p->next->next);
        // p->next=new topiclinkedlist;
        // p->next->next=NULL;
        // p->next->t.tid=++last_tid;
        // p->next->t.type=m.data.crt_tp_req.t.type;
        int temp_tid=0;
        if(tp_man.addTopic(last_tid,m.data.crt_tp_req.t.type,m.nid)){     // Should optimize
          temp_tid=last_tid+1;
          last_tid++;
          cout<<"New Topic Created"<<endl;
        }
        else{
          cout<<"New Topic Create : Failed"<<endl;
        }
    
        m.type=CRT_TP_RES;
        m.data.crt_tp_res.t.tid=temp_tid;
        m.data.crt_tp_res.res=ACK;
        // radio.stopListening();
        // radio.write(&m,sizeof(message));       //Shold be directed perticular to that node
        // radio.startListening();
        if(sensorNetwork.sendMessage(nid,m)){
          cout<<"CRT_TP_RES : Sent"<<endl;
        }
        else{
          cout<<"CRT_TP_RES : Failed to send"<<endl;
        }
        
      }
      else if(m.type==GET_TP_UP_REQ){
        // int my_time=(int)time(0);
        // if(my_time%2==0){
          int temp_tid=m.data.get_tp_up_req.tid;
          m.type=GET_TP_UP_RES;
          m.data.get_tp_up_res.tdata=tp_man.getData(tid);
          if(sensorNetwork.sendMessage(nid,m))
            cout<<"SUB_TP_RES : Sent"<<endl;
          else
            cout<<"SUB_TP_RES : Failed"<<endl;
      }
  }

}

int main(int argc,char *argv[]){
  // radio.begin();
  // TopicList=new topiclinkedlist;
  // TopicList->next=NULL;
  if(argc!=2){
    cout<<"Required 1 argument (./a.out <ip address>)"<<endl;
    return 0;
  }
  thread sock_thr(&socket_thread,string(argv[1]));
  thread nrf_thr(&nrf_thread);
  //cout<<"Size of enum :"<<sizeof(enum message_type)<<endl;
  


  
   sock_thr.join();
   nrf_thr.join();
}



  // uint64_t gotAddress=0xAABBCC0011LL;                //nrf24 needs 5 bytes of address
  // uint64_t readingList[1]={0xAA11223344LL};
  // radio.openWritingPipe(gotAddress);
  // cout<<"Opened Writing Pipe"<<endl;
  // radio.openReadingPipe(1,readingList[0]);
  // cout<<"Opened Reading Pipe"<<endl;
  // radio.stopListening();
  // message m;
  // m.type=ATH_REQ;
  // m.data.ath_req.nid=0;
  // m.data.ath_req.rpipe=0;
  // m.data.ath_req.wpipe=readingList[0];
  // radio.write(&m,sizeof(message));
  // cout<<"Sent the address"<<endl;
  // radio.startListening();
  // while(!radio.available());
  // cout<<"Got someting"<<endl;
  // radio.read(&m,sizeof(message));
  // if(m.type==ATH_RES){
  //   cout<<"Successfully connected"<<endl;
  //   cout<<"Got respond id : "<<std::hex<<m.data.ath_res.res<<endl;
  // }
  // else
  //   cout<<"Unexpected data recived"<<endl;


// while(1){          //Gateway reset condition should be given
//     if(radio.available()){
//       message m;
//       radio.read(&m,sizeof(message));
//       if(m.type==CRT_TP_REQ){
//         cout<<"Got: CRT_TP_REQ"<<endl;
//         int nid=m.data.crt_tp_req.nid;
//         topiclinkedlist *p;
//         for(p=TopicList;p->next!=NULL;p->next=p->next->next);
//         p->next=new topiclinkedlist;
//         p->next->next=NULL;
//         p->next->t.tid=++last_tid;
//         p->next->t.type=m.data.crt_tp_req.t.type;
//         cout<<"New Topic Created"<<endl;
//         m.type=CRT_TP_RES;
//         m.data.crt_tp_res.nid=nid;
//         m.data.crt_tp_res.t.tid=last_tid;
//         m.data.crt_tp_res.t.type=p->next->t.type;
//         m.data.crt_tp_res.res=ACK;
//         radio.stopListening();
//         radio.write(&m,sizeof(message));       //Shold be directed perticular to that node
//         radio.startListening();
//         cout<<"CRT_TP_RES : Sent"<<endl;
//       }
//       else if(m.type==GET_TP_UP_REQ){
//         int my_time=(int)time(0);
//         if(my_time%2==0){
//           m.type=GET_TP_UP_RES;
//           m.data.get_tp_up_res.tdata=55;
//           radio.stopListening();
//           radio.write(&m,sizeof(message));
//           radio.startListening();
//           cout<<"SUB_TP_RES 55 : Sent"<<endl;
//         }
//         else{
//           m.type=GET_TP_UP_RES;
//           m.data.get_tp_up_res.tdata=75;
//           radio.stopListening();
//           radio.write(&m,sizeof(message));
//           radio.startListening();
//           cout<<"SUB_TP_RES 75 : Sent"<<endl;
//         }
//       }
//     }
//   }