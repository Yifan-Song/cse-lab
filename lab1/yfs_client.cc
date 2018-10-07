// yfs client.  implements FS operations using extent and lock server
#include "yfs_client.h"
#include "extent_client.h"
#include <sstream>
#include <iostream>
#include <stdio.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <inode_manager.h>
#include <stdlib.h>

const int step = FILENAME_MAX + 4;

yfs_client::yfs_client()
{
    ec = new extent_client();

}

yfs_client::yfs_client(std::string extent_dst, std::string lock_dst)
{
    ec = new extent_client();
    if (ec->put(1, "") != extent_protocol::OK)
        printf("error init root dir\n"); // XYB: init root dir
}

yfs_client::inum
yfs_client::n2i(std::string n)
{
    std::istringstream ist(n);
    unsigned long long finum;
    ist >> finum;
    return finum;
}

std::string
yfs_client::filename(inum inum)
{
    std::ostringstream ost;
    ost << inum;
    return ost.str();
}

bool
yfs_client::isfile(inum inum)
{
    extent_protocol::attr a;

    if (ec->getattr(inum, a) != extent_protocol::OK) {
        printf("error getting attr\n");
        return false;
    }

    if (a.type == extent_protocol::T_FILE) {
        printf("isfile: %lld is a file\n", inum);
        return true;
    } 
    printf("isfile: %lld is a dir\n", inum);
    return false;
}
/** Your code here for Lab...
 * You may need to add routines such as
 * readlink, issymlink here to implement symbolic link.
 * 
 * */

bool
yfs_client::isdir(inum inum)
{
    // Oops! is this still correct when you implement symlink?
    extent_protocol::attr a;
    if (ec->getattr(inum, a) != extent_protocol::OK) {
        printf("error getting attr\n");
        return false;
    }

    if (a.type == extent_protocol::T_DIR) {
        printf("isdir: %lld is a dir\n", inum);
        return true;
    } 
    printf("isdir: %lld is not a dir\n", inum);
    return false;
}

int
yfs_client::getfile(inum inum, fileinfo &fin)
{
    int r = OK;

    printf("getfile %016llx\n", inum);
    extent_protocol::attr a;
    if (ec->getattr(inum, a) != extent_protocol::OK) {
        r = IOERR;
        goto release;
    }

    fin.atime = a.atime;
    fin.mtime = a.mtime;
    fin.ctime = a.ctime;
    fin.size = a.size;
    printf("getfile %016llx -> sz %llu\n", inum, fin.size);

release:
    return r;
}

int
yfs_client::getdir(inum inum, dirinfo &din)
{
    int r = OK;

    printf("getdir %016llx\n", inum);
    extent_protocol::attr a;
    if (ec->getattr(inum, a) != extent_protocol::OK) {
        r = IOERR;
        goto release;
    }
    din.atime = a.atime;
    din.mtime = a.mtime;
    din.ctime = a.ctime;

release:
    return r;
}

int
yfs_client::getsymlink(inum inum, syminfo &sin)
{
    int r = OK;

    printf("getsymlink %016llx\n", inum);
    extent_protocol::attr a;
    if (ec->getattr(inum, a) != extent_protocol::OK)
    {
        r = IOERR;
        goto release;
    }

    sin.atime = a.atime;
    sin.mtime = a.mtime;
    sin.ctime = a.ctime;
    sin.size = a.size;
    printf("getsymlink %016llx -> sz %llu\n", inum, sin.size);

release:
    return r;
}

#define EXT_RPC(xx) do { \
    if ((xx) != extent_protocol::OK) { \
        printf("EXT_RPC Error: %s:%d \n", __FILE__, __LINE__); \
        r = IOERR; \
        goto release; \
    } \
} while (0)

// Only support set size of attr
int
yfs_client::setattr(inum ino, size_t size)
{
    printf("setattr: %lld \n", ino);
    int r = OK;
    std::string content;
    r = ec->get(ino, content);
    if (r != OK){
      return r;
    }
    content.resize(size);
    r = ec->put(ino, content);
    /*
     * your code goes here.
     * note: get the content of inode ino, and modify its content
     * according to the size (<, =, or >) content length.
     */
    return r;
}

int
yfs_client::create(inum parent, const char *name, mode_t mode, inum &ino_out)
{
    printf("creating file: parent: %lld; name:%s; inum:%lld \n", parent, name, ino_out);
    int r = OK;
    /*
     * your code goes here.
     * note: lookup is what you need to check if file exist;
     * after create file or dir, you must remember to modify the parent infomation.
     */
    bool found = false;
    lookup(parent, name, found, ino_out);
    if (found){
      return EXIST;
    }
    if ((r = ec->create(extent_protocol::T_FILE, ino_out)) != OK){
      std::cout << "create ERROR\n"; 
      return r;
    }
    std::string parent_content;
    std::string file_name(name);
    std::ostringstream new_parent_content;
    
    ec->get(parent, parent_content);
    new_parent_content << parent_content << file_name << "," << ino_out << ";";
    //printf("new_parent_content: %s \n", new_parent_content.str().c_Str());
    if(ec->put(parent, new_parent_content.str()) != OK){
      std::cout << "put ERROR\n";
      return r;
    }
    return r;
}

int
yfs_client::mkdir(inum parent, const char *name, mode_t mode, inum &ino_out)
{
    printf("mkdir: parent:%lld; name:%s; inum:%lld;\n", parent, name, ino_out);
    int r = OK;
    bool found = false;
    if(lookup(parent, name, found, ino_out) != OK){
      printf("mkdir-lookup ERROR \n");
    }
    if (found){
      printf("dir already exist \n");
      return EXIST;
    }
    if (ec->create(extent_protocol::T_DIR, ino_out) != OK){
      printf("mkdir-create ERROR \n");
      return r;
    }
    std::string parent_content;
    std::string file_name(name);
    std::ostringstream os;

    ec->get(parent, parent_content);
    os<<parent_content<<file_name<<","<<ino_out<<";";
    ec->put(parent, os.str());
    /*
     * your code goes here.
     * note: lookup is what you need to check if directory exist;
     * after create file or dir, you must remember to modify the parent infomation.
     */

    return r;
}

std::string getFileName(char* file){
    char content[BLOCK_SIZE];
    std::string fileStr = std::string(file);
    strcpy(content, fileStr.c_str());
    const char a[2] = ",";
    char* file_name = strtok(content, a);
    std::string res = std::string(file_name);
    return res;
}

int getFileInum(char* file){
    char content[BLOCK_SIZE];
    std::string fileStr = std::string(file);
    strcpy(content, fileStr.c_str());
    const char a[2] = ",";
    char* file_inum_str = strtok(content, a);
    file_inum_str = strtok(NULL, a);
    int file_inum = atoi(file_inum_str);
    return file_inum;
}

int
yfs_client::lookup(inum parent, const char *name, bool &found, inum &ino_out)
{
    printf("lookup file: parent: %lld; name:%s; inum:%lld \n", parent, name, ino_out);
    int r = OK;
    /*
     * your code goes here.
     * note: lookup file from parent dir according to name;
     * you should design the format of directory content.
     */
    std::list<dirent> list;
    std::list<dirent>::iterator iter;
    r = readdir(parent, list);
    for(iter = list.begin();iter != list.end();iter++)
    {
        if(name == iter->name){
            found = true;
            ino_out = iter->inum;
            return r;
        }
    }
    found = false;
    return r;

    /*printf("lookup file: parent: %lld; name:%s; inum:%lld \n", parent, name, ino_out);
    int r = OK;
    int index = 0;
    std::string parent_content;
    std::string c_content;
    r = ec->get(parent, parent_content);
    r = ec->get(parent, c_content);
    char content[BLOCK_SIZE * 100];
    strcpy(content, parent_content.c_str());
    char* head_file;
    head_file = strtok(content, ";");
    if(head_file == NULL){
      found = false;
      r = NOENT;
      printf("head is empty\n");
      return r;
    }
    //printf("head_file:%s \n", head_file);
    std::string head_file_str = std::string(head_file);
    //std::cout<<"headfilestr:"<<head_file_str<<std::endl;
    std::string content_str = std::string(content);
    std::cout<<"content_str:"<<content_str<<std::endl;
    std::cout<<head_file_str.length()<<"\n";
    index += head_file_str.length();
    c_content = c_content.substr(index,BLOCK_SIZE * 100);
    //std::string new_content_str = parent_content.substr(index,BLOCK_SIZE * 100);
    std::cout<<"new_content_str:"<<parent_content<<std::endl;
    
    printf("lookup name: %s \n", name);
    printf("filehead: %s \n", head_file);
    
    if(name == getFileName(head_file)){
      ino_out = getFileInum(head_file);
      found = true;
      r = OK;
      printf("head matched\n");
      return r;
    }
    
    //char * test_file;
    //test_file = strtok(content_str.substr(sizeof(head_file_str)).c_str(), ";");
    //printf("testfiles: %s \n", test_file);

    while(true){
      std::cout<<"1\n";
      char* current_file;
      std::cout<<"2\n";
      char new_content[BLOCK_SIZE * 100];
      std::cout<<"3\n";
      strcpy(new_content, c_content.c_str());
      std::cout<<"4\n";
      current_file = strtok(new_content, ";");
      std::cout<<"5\n";
      std::string current_file_str = current_file;
      if(current_file == NULL){
        found = false;
        r = NOENT;
        printf("meet the end\n");
        break;
      }
      printf("files: %s \n", current_file);
      printf("filename: %s \n", getFileName(current_file).c_str());
      if(name == getFileName(current_file)){
        ino_out = getFileInum(current_file);
        found = true;
        return r;
      }
      index += current_file_str.length();
      c_content = c_content.substr(index,BLOCK_SIZE * 100);
      
      //std::string file_str = current_file;
      //content_str = new_content;
      //std::string new_content_str = content_str.substr(file_str.length());
    }*/
    //found = false;
    //return r;
}

int
yfs_client::readdir(inum dir, std::list<dirent> &list)
{
   printf("readdir %lld \n", dir);
   int r = OK;
    /*
     * your code goes here.
     * note: you should parse the dirctory content using your defined format,
     * and push the dirents to the list.
     */
    std::string dir_content;
    if ((r = ec->get(dir, dir_content)) != OK){
      std::cout << "get ERROR\n";
      return r;
    }

    int old_position = 0;
    int new_position = 0;

    while(true){
      new_position = dir_content.find(";", old_position);
      if (new_position == std::string::npos){
        break;
      }
      struct dirent new_entry;
      std::string file = dir_content.substr(old_position, new_position-old_position);
      //printf("currentFile: %s \n", file.c_str());
      int offset = file.find(",");
      std::string file_name = file.substr(0, offset);
      int file_inum = n2i(file.substr(offset + 1));
      //printf("currentFileName: %s \n", file_name);;
      //printf("currentFileInum: %d \n", file_inum);
      new_entry.name = file_name;
      new_entry.inum = file_inum;
      list.push_back(new_entry);
      old_position = new_position + 1;
    }
    return r;

}

int
yfs_client::read(inum ino, size_t size, off_t off, std::string &data)
{
    printf("read %lld \n", ino);
    int r = OK;
    std::string content;
    if(ec->get(ino, content) != OK){
      printf("read ERROR\n");
      return IOERR;
    }
    data = content.substr(off, size);

    //if(off + size > content.size()){
    //  printf("read size ERROR");
    //  return r;
    //}

    /*
     * your code goes here.
     * note: read using ec->get().
     */
    return r;
}

int
yfs_client::write(inum ino, size_t size, off_t off, const char *data,
        size_t &bytes_written)
{

    printf("write %lld, %s \n", ino, data);
    int r = OK;
     
    std::string content;
    //std::string new_content = data;
    //new_content.resize(size);
    if (ec->get(ino, content) != OK){
        printf("write ERROR\n");
        return IOERR;
    }
    if (off > content.size()){
        std::string new_content = data;
        new_content.resize(size);
        //content.append(off-content.size(), '\0');
        content.resize(off, '\0');
        //content.replace(content.size(), off-content.size(), '\0');
        content.replace(off, size, new_content);
    }
    else{
        //content = content.substr(0, off) + new_content;
        content.replace(off, size, data, size);
    }
    if (ec->put(ino, content) != OK){
        printf("write ERROR\n");
        return IOERR;
    }
    /*
     * your code goes here.
     * note: write using ec->put().
     * when off > length of original file, fill the holes with '\0'.
     */

    return r;
}

std::string cutString(std::string s1, std::string s2){
    int index = s1.find(s2);
    std::string front = s1.substr(0, index);
    std::string after = s1.substr(index);
    int next_index = after.find(";");
    after = after.substr(next_index + 1);
    s1 = front + after;
    return s1;
}

int yfs_client::unlink(inum parent,const char *name)
{
    int r = OK;
    bool found = false;
    inum file_inum;
    if(lookup(parent, name, found, file_inum) != OK){
      printf("unlink-lookup ERROR \n");
      return IOERR;
    }
    if (!found){ 
      printf("unlink ERROR: file doesn't exist \n");
      return IOERR;
    }
    if(ec->remove(file_inum) != OK){
      printf("unlink-remove ERROR \n");
      return IOERR;
    }
    std::string parent_content;
    std::string file_name = name;
    if(ec->get(parent, parent_content) != OK){
      printf("unlink-get ERROR \n");
      return IOERR;
    }
    parent_content = cutString(parent_content, file_name);
    if(ec->put(parent, parent_content) != OK){
      printf("unlink-put ERROR \n");
      return IOERR;
    }
    /*
     * your code goes here.
     * note: you should remove the file using ec->remove,
     * and update the parent directory content.
     */

    return r;
}

int yfs_client::symlink(inum parent, const char * dir, const char * name, inum & ino){
    int r = OK;
    if (ec->create(extent_protocol::T_SYMLINK, ino) != extent_protocol::OK){
        printf("symlink-create ERROR \n");
        return IOERR;
    }
    if (ec->put(ino, dir) != OK){
        printf("symlink-put ERROR \n");
        return IOERR;
    }
    std::string parent_content;
    ec->get(parent, parent_content);
    std::string file_name = name;
    std::ostringstream os;
    os << parent_content << file_name << "," << ino << ";";
    ec->put(parent, os.str());
    return r;
}

int yfs_client::readlink(inum inumber, std::string &data){
    int r = OK;
    if(r = ec->get(inumber, data) != OK){
      printf("readlink ERROR \n");
      return IOERR;
    }
    return r;
}

