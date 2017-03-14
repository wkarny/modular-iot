#include<iostream>
#include<cstdio>
#include<cstdlib>
#include<RF24/RF24.h>
using namespace std;

RF24 radio(22,0);


int main(){
  radio.begin();
  uint64_t gotAddress=0xAABBCCDDEEFF0011LL;
  uint64_t readingList[1]={0xAA00112233445501LL};
  radio.openWritingPipe(gotAddress);
  cout<<"Opened Writing Pipe"<<endl;
  radio.openReadingPipe(1,readingList[0]);
  cout<<"Opened Reading Pipe"<<endl;
  radio.stopListening();
  radio.write(&readingList[0],sizeof(uint64_t));
  cout<<"Sent the address"<<endl;
  radio.startListening();
  while(!radio.available());
  cout<<"Got someting"<<endl;
  char c;
  radio.read(&c,sizeof(c));
  if(c=='c')
    cout<<"Successfully connected"<<endl;
  else
    cout<<"Unexpected data recived"<<endl;
   
}
