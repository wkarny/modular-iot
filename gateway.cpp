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
//#define MAIN_SERVER_PORT_NUM 8080
#define PORT 5050

void enqueueList(int,string);

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
  unordered_map<uint16_t,uint32_t> tData;  // tid, tdata
  unordered_map<uint16_t,uint8_t> tType;   // tid, ttype
  unordered_map<uint16_t,uint16_t> tNid;   // tid, nid
public:
  TopicManager(){};
  ~TopicManager(){};
  bool addTopic(uint16_t tid, uint8_t type,uint16_t nid);
  bool find(uint16_t tid);
  uint32_t getData(uint16_t tid);
  uint8_t getType(uint16_t tid);
  bool putData(uint16_t tid, uint32_t data);
  uint16_t getNid(uint16_t tid);
  void sendTopicListToServer(int req);
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
    cout<<"getData : Failed (TID ="+to_string(tid)+" NOT FOUND)"<<endl;
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

void TopicManager::sendTopicListToServer(int req){
  for(auto it=tType.begin();it!=tType.end();++it){
    uint16_t tid=it->first;
    uint8_t type=it->second;
    uint32_t data=tData[tid];
    string msg="GATR+"+to_string(tid)+"+"+to_string(type)+"+"+to_string(data)+"+"+to_string(req);
    cout<<"Pushed into queue : ";
    cout<<msg<<endl;
    enqueueList(2,msg);
  }
  enqueueList(2,"GATR+END+"+to_string(req));
}

/* Topic Manager class end */


mutex mu1,mu2;
list<string> queue1;  // for socket_server --> nrf_sever
list<string> queue2;  // for nrf_server --> socket_server

uint16_t last_tid=200;           //should be replaced with non-volatile storage

void enqueueList(int q,string str){
  if(q==1){
    lock_guard<mutex> gaurd(mu1);
    queue1.push_back(str);
  }else if(q==2){
    lock_guard<mutex> gaurd(mu2);
    queue2.push_back(str);
  }
}

int isEmptyList(int q){
  if(q==1){
    lock_guard<mutex> gaurd(mu1);
    if(queue1.empty()){
      return 1;
    }
    else{
      return 0;
    }
  }else if(q==2){
    lock_guard<mutex> gaurd(mu2);
    if(queue2.empty()){
      return 1;
    }
    else{
      return 0;
    }
  }
}

string dequeueList(int q){
  if(q==1){
    lock_guard<mutex> gaurd(mu1);
    string str=queue1.front();
    queue1.pop_front();
    return(str);
  }else if(q==2){
    lock_guard<mutex> gaurd(mu2);
    string str=queue2.front();
    queue2.pop_front();
    return(str);
  }
}

void client_handle(int new_socket){
  int valread;
  char buffer[1024] = {0};
  char cp_buffer[1024]= {0};
  //while(1){
      memset(buffer,0,sizeof(buffer));
      valread = read( new_socket , buffer, 1024);          //Waiting for Msg
      cout<<"Recived from Socket: "<<buffer<<endl;
      strcpy(cp_buffer,buffer);
      char *cm=strtok(cp_buffer,"+");
      if(strcmp(cm,"LOGIN")==0){
        string rpl;
        cm=strtok(NULL,"+");
        if(strcmp(cm,"wyes")==0){
          cm=strtok(NULL,"+");
          if(strcmp(cm,"123456")==0){
          cout<<"Login : Success"<<endl;
          rpl="LOGIN+ACK";
          }
          else{
          cout<<"Login : Denied"<<endl;
          rpl="LOGIN+NACK";
         }
        }
        else{
          cout<<"Login : Denied"<<endl;
          rpl="LOGIN+NACK";
        }
        send(new_socket,rpl.c_str(),strlen(rpl.c_str()),0); 
      }
      else{
        
        enqueueList(1,string(buffer));                   //Enqueing in the list
        while(isEmptyList(2));
        while(!isEmptyList(2)){
          string reply;
          reply=dequeueList(2);
          send(new_socket,reply.c_str(),strlen(reply.c_str()),0); 
          usleep(100000);
        }
      }
    //}
      close(new_socket);

}

void socket_thread(){

    int server_fd, new_socket, valread;
    struct sockaddr_in address;
    int opt = 1;
    int addrlen = sizeof(address);
    char buffer[1024] = {0};
    //char *hello = "Hello from server";
      
    // Creating socket file descriptor
    if ((server_fd = socket(AF_INET, SOCK_STREAM, 0)) == 0)
    {
        perror("socket failed");
        exit(EXIT_FAILURE);
    }
      
    // Forcefully attaching socket to the port 8080
    if (setsockopt(server_fd, SOL_SOCKET, SO_REUSEADDR | SO_REUSEPORT,
                                                  &opt, sizeof(opt)))
    {
        perror("setsockopt");
        exit(EXIT_FAILURE);
    }
    address.sin_family = AF_INET;
    address.sin_addr.s_addr = INADDR_ANY;
    address.sin_port = htons( PORT );
      
    // Forcefully attaching socket to the port 8080
    if (bind(server_fd, (struct sockaddr *)&address, 
                                 sizeof(address))<0)
    {
        cout<<"Socket Bind : Failed"<<endl;
        perror("bind failed");
        exit(EXIT_FAILURE);
    }
    while(1){
      if (listen(server_fd, 3) < 0)
      {
          cout<<"Socket Listen : Failed"<<endl;
          perror("listen");
          exit(EXIT_FAILURE);
      }
      if ((new_socket = accept(server_fd, (struct sockaddr *)&address, 
                         (socklen_t*)&addrlen))<0)
      {
          cout<<"Socket Accept : Failed"<<endl;
          perror("accept");
          exit(EXIT_FAILURE);
      }
      thread client(&client_handle,new_socket);
      client.join();

    }
    // valread = read( new_socket , buffer, 1024);
    // printf("Recived :%s\n",buffer );
    
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
  ~MyRadio(){};
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
  //radio->openReadingPipe(last_pipe_num,last_pipe);
}

int MyRadio::attachNode(uint64_t pipe){
  if(node_id.size()>=5){
    return 0;
  }
  else{
    radio->openWritingPipe(pipe);
    cout<<"Opened Writing Pipe"<<endl;
    radio->openReadingPipe(last_pipe_num+1,last_pipe+1);
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
      while(!radio->available(&pipe_num));            //Should terminate after some time
    cout<<"Got someting"<<endl;
    radio->read(&m,sizeof(message));
    if(m.type==ATH_RES){
      cout<<"Successfully connected"<<endl;
      cout<<"Got respond id : "<<std::hex<<m.data.ath_res.res<<endl;
      increaseNode(pipe);
      return nodeIDAllocated;
    }
    else
      cout<<"Unexpected data recived"<<endl;
  }
}

int MyRadio::sendMessage(uint16_t nid,message m){
  if(writing_list.find(nid)==writing_list.end()){
    cout<<"sendMessage : nid not found"<<endl;
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

    //Start of Servicing the Socket Server

    if(!isEmptyList(1)){   // For servicing Server Requests
      string str=dequeueList(1);
      char *cm;
      char buffer[1024];
      strcpy(buffer,str.c_str());
      cm=strtok(buffer,"+");
      if(strcmp(cm,"ADQ")==0){   //ADQ start
        char* token[3];
        bool errorInToken=false;
        for(int i=0;i<3;i++){
          cm=strtok(NULL,"+");
          if(cm==NULL){
            cout<<"Error is parsing : Not Enough Tokens"<<endl;
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
          if((allocatedNID=sensorNetwork.attachNode(pipe))!=0){
            cout<<"Successfully node attach"<<endl;
          }
          else{
            cout<<"Unable to Attach"<<endl;
          }
          enqueueList(2,"ADR+"+to_string(allocatedNID)+"+"+to_string(reqid));
        }
        else{
          //Send a NACK
        }
    }            //ADQ end
    else if(strcmp(cm,"PTUQ")==0){
        char* token[3];
        bool errorInToken=false;
        for(int i=0;i<3;i++){
          cm=strtok(NULL,"+");
          if(cm==NULL){
            cout<<"Error is parsing : Not Enough Tokens"<<endl;
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
            enqueueList(2,"PTUR+"+to_string(temp_tid)+"+ACK+"+to_string(temp_reqid));
          else
            enqueueList(2,"PTUR+"+to_string(temp_tid)+"+NACK+"+to_string(temp_reqid));
        }
    }
    else if(strcmp(cm,"GTUQ")==0){
        char* token[2];
        bool errorInToken=false;
        for(int i=0;i<2;i++){
          cm=strtok(NULL,"+");
          if(cm==NULL){
            cout<<"Error is parsing : Not Enough Tokens"<<endl;
            errorInToken=true;
            break;
          }
          token[i]=cm;
        }
        if(!errorInToken){
          int temp_tid=atoi(token[0]);
          int temp_reqid=atoi(token[1]);
          if(tp_man.find(temp_tid)){
            enqueueList(2,"GTUR+"+to_string(temp_tid)+"+"+to_string(tp_man.getData(temp_tid))+"+"+to_string(temp_reqid));
            cout<<"GTUR : Sent Successfully"<<endl;
          }
          else{
            enqueueList(2,"GTUR+"+to_string(temp_tid)+"+NACK+"+to_string(temp_reqid));
            cout<<"GTUR : TID mismatch : "+to_string(temp_tid)<<endl;
          }
        }
    }
    else if(strcmp(cm,"GATQ")==0){
          cm=strtok(NULL,"+");
          if(cm==NULL){
            cout<<"Error is parsing : GATQ ReqID not found"<<endl;
          }
          else{
            int req=atoi(cm);
            tp_man.sendTopicListToServer(req);
          }

    }
    else{
      cout<<"Unrecognised Command"<<endl;
    }
  }

  //End of Servicing the Socket Server


  //Start of Servicing the sensor nodes

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
        if(tp_man.addTopic(last_tid+1,m.data.crt_tp_req.t.type,m.nid)){     // Should optimize
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
        while(!sensorNetwork.sendMessage(nid,m)){
          cout<<"CRT_TP_RES : Sending"<<endl;
        }
          cout<<"CRT_TP_RES : Sent"<<endl;
        
      }
      else if(m.type==GET_TP_UP_REQ){
        // int my_time=(int)time(0);
        // if(my_time%2==0){
          int temp_tid=m.data.get_tp_up_req.tid;
          m.type=GET_TP_UP_RES;
          m.data.get_tp_up_res.tdata=tp_man.getData(temp_tid);   //Should handle error
          if(sensorNetwork.sendMessage(nid,m))
            cout<<"GET_TP_UP_RES : Sent"<<endl;
          else
            cout<<"GET_TP_UP_RES : Failed"<<endl;
      }
      else if(m.type==PUB_TP_REQ){
          int temp_tid=m.data.pub_tp_req.tid;
          int temp_data=m.data.pub_tp_req.tdata;
          uint8_t resp=NACK;
          if(tp_man.find(temp_tid)){
            tp_man.putData(temp_tid,temp_data);
            resp=ACK;
          }
          m.type=PUB_TP_RES;
          m.data.pub_tp_res.tid=temp_tid;
          m.data.pub_tp_res.res=resp;
          if(sensorNetwork.sendMessage(nid,m))
            cout<<"PUB_TP_RES : Sent"<<endl;
          else
            cout<<"PUB_TP_RES : Failed"<<endl;
      }
  }

  //End of Servicing the sensor nodes

}
}

int main(int argc,char *argv[]){
  // radio.begin();
  // TopicList=new topiclinkedlist;
  // TopicList->next=NULL;
  // if(argc!=2){
  //   cout<<"Required 1 argument (./a.out <ip address>)"<<endl;
  //   return 0;
  // }
  thread sock_thr(&socket_thread);
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