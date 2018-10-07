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
  //const char* tc = "test";
  //char* tc2 = "123";
  //tc2 = tc;
  //sprintf(ts2, "%d", 123);
  //ts = ts + "q" + ts2;
  //cout<<tc2<<"\n";
  //const char* res = "qwe";
  //if(res == getFileName("qwe,132")){
  //  cout<<"!!!\n";
  //}
  //else{
  //  cout<<"???\n";
  //}

  char content[10000];
  string ts = "file -\n-       -hxlctuvhvwsbsggjdbqmqxyumglpkmsvfdvaxnze-4251-0,2;file -\n-       -dcredbnpgyhoexaocxphgydeqmzvyrtspcrjuheq-4251-1,3;file -\n-       -ksglgrkbhshyjshachyypmmgxiapyqiemzdkkvle-4251-2,4;"; 
  strcpy(content, ts.c_str());
  string content_str = content;
  char* tc = strtok(content, ";");
  cout << tc<<endl;
  string tc_str = tc;
  string new_content_str = content_str.substr(tc_str.length(),1000);
  if("tccc" == getFileName(tc)){
    printf("?");
  }
  //while(true){
    char new_content[10000];
    strcpy(new_content, new_content_str.c_str());
    tc = strtok(new_content,";");
    if(tc == NULL){
       printf("???");
    }
    cout<<tc<<endl;
  //}
  int inum = getFileInum("qwe,132");
  cout<<inum<<"\n";
  //if(res == cmp){
  //  cout<<"!!!\n";
  //}
  //else{
  //  cout<<"???\n";
  //}
  //cout << res << "\n";
  //if(res.c_str())
  return 1;
}
