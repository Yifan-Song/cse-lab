#include <string.h>
#include <stdio.h>
#include <iostream>
#include <stdlib.h>
using namespace std;

string getFileName(char* file){
    char content[100];
    std::string fileStr = std::string(file);
    strcpy(content, fileStr.c_str());
    const char a[2] = ",";
    char* file_name = strtok(content, a);
    std::string res = std::string(file_name);
    return res;
}

int getFileInum(char* file){
    char content[100];
    string fileStr = string(file);
    strcpy(content, fileStr.c_str());
    const char a[2] = ",";
    char* file_inum_str = strtok(content, a);
    file_inum_str = strtok(NULL, a);
    int file_inum = atoi(file_inum_str);
    return file_inum;
}

int main(){
  /*string str  = "qwe;asd;aq;";
  char s[100];
  strcpy(s,str.c_str());
  const char a[2] = ";";
  char* res;
  res = strtok(s, a);
  cout << res << "\n";
  res = strtok(NULL, a);
  cout << res << "\n";
  res = strtok(NULL, a);
  cout << res << "\n";
  res = strtok(NULL, a);
  if(res == NULL){
    cout << "!!" << "\n";
  }*/
  const char* tc = "test";
  char* tc2 = "123";
  tc2 = tc;
  //sprintf(ts2, "%d", 123);
  //ts = ts + "q" + ts2;
  cout<<tc2<<"\n";
  const char* res = "qwe";
  string cmp = getFileName("qwe,132");
  int inum = getFileInum("qwe,132");
  cout<<inum<<"\n";
  if(res == cmp){
    cout<<"!!!\n";
  }
  else{
    cout<<"???\n";
  }
  //cout << res << "\n";
  //if(res.c_str())
  return 1;
}
