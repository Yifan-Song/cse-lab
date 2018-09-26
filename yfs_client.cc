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
    return ! isfile(inum);
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
    int r = OK;

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
    int r = OK;
    printf("creating file: parent: %lld; name:%s; inum:%lld \n", parent, name, ino_out);
    bool found = false;
    lookup(parent, name, found, ino_out);
    if(found == true){
      printf("file already exist");
      r = EXIST;
      return r;
    }
    ec->create(extent_protocol::T_FILE, ino_out);
    std::string parent_content;
    r = ec->get(parent, parent_content);
    printf("old parent_content: %s \n", parent_content.c_str());
    std::string new_content = std::string(name) + ",";
    char new_content2[BLOCK_SIZE];
    sprintf(new_content2, "%lld", ino_out);
    new_content += new_content2;
    new_content += ";";
    printf("new_content: %s \n", new_content.c_str());
    parent_content += new_content;
    r = ec->put(parent, parent_content);
    printf("new parent_content: %s \n", parent_content.c_str());
    return r;
    /*
     * your code goes here.
     * note: lookup is what you need to check if file exist;
     * after create file or dir, you must remember to modify the parent infomation.
     */
}

int
yfs_client::mkdir(inum parent, const char *name, mode_t mode, inum &ino_out)
{
    int r = OK;

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
    std::string parent_content;
    r = ec->get(parent, parent_content);
    char content[BLOCK_SIZE * 100];
    strcpy(content, parent_content.c_str());
    const char a[2] = ";";
    char* current_file;
    current_file = strtok(content, a);
    if(current_file == NULL){
      found = false;
      r = NOENT;
      return r;
    }
    if(name == getFileName(current_file)){
      ino_out = getFileInum(current_file);
      found = true;
      return r;
    }
    while(true){
      current_file = strtok(NULL, a);
      if(current_file == NULL){
        found = false;
        r = NOENT;
        break;
      }
      if(name == getFileName(current_file)){
        ino_out = getFileInum(current_file);
        found = true;
        return r;
      }
    }
    /*
     * your code goes here.
     * note: lookup file from parent dir according to name;
     * you should design the format of directory content.
     */
    found = false;
    return r;

}

int
yfs_client::readdir(inum dir, std::list<dirent> &list)
{
    int r = OK;
    std::string dir_content;
    r = ec->get(dir, dir_content);
    printf("dir_content: %s", dir_content.c_str());
    char content[BLOCK_SIZE * 100];
    strcpy(content, dir_content.c_str());
    const char a[2] = ";";
    char* current_file;
    current_file = strtok(content, a);
    if(current_file == NULL){
      return r;
    }
    struct dirent entry;
    entry.name = getFileName(current_file);
    entry.inum = getFileInum(current_file);
    list.push_back(entry);
    while(true){
      current_file = strtok(NULL, a);
      if(current_file == NULL){
        break;
      }
      struct dirent current_entry;
      current_entry.name = getFileName(current_file);
      current_entry.inum = getFileInum(current_file);
      list.push_back(current_entry);
    }
    for(std::list<dirent>::iterator iter = list.begin();iter != list.end();iter++) 
    { 
      printf("listcontent:%s;%lld", (*iter).name.c_str(), (*iter).inum);
    } 
    //printf("listhead:%s;%lld", list.front().name.c_str(), list.front().inum);
    /*
     * your code goes here.
     * note: you should parse the dirctory content using your defined format,
     * and push the dirents to the list.
     */

    return r;
}

int
yfs_client::read(inum ino, size_t size, off_t off, std::string &data)
{
    int r = OK;

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
    int r = OK;

    /*
     * your code goes here.
     * note: write using ec->put().
     * when off > length of original file, fill the holes with '\0'.
     */

    return r;
}

int yfs_client::unlink(inum parent,const char *name)
{
    int r = OK;

    /*
     * your code goes here.
     * note: you should remove the file using ec->remove,
     * and update the parent directory content.
     */

    return r;
}

