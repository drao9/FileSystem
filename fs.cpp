//CS560 Lab 1 Assignment - File System
/*The program is an implementation of a simple Unix-like file system. We created a file of size 100MB to simulate the virtual hard disk. It has a hierarchical directory system. Files and directories can grow and shrink, and you need to manage free disk space. Our file system is able to allow users to browse the directory structure, create and delete new files and directories, etc. It does not have permission control and user management.
*/

#include <iostream>
#include <cstdio>
#include <stdio.h>
#include <fstream>
#include <map>
#include <vector>
#include <stdlib.h>
#include <sstream>
#include <sys/stat.h>
#include <string>

//constants
#define TOTAL_INODES_BYTES 3200
#define SIZEOF_INODE 32
#define SIZEOF_DISK 100000000
#define SIZEOF_BLK 96000
#define NUMOF_INODES 100
#define NUMOF_FREE_LIST 3124 
#define NUMOF_FD 10
#define NUMOF_FILES_IN_DIR 50

using namespace std;

/*
   This is a struct to store the information about the directories. 
   It stores a vector of files, array of inodes and the
   number of files in each directory.
   */
struct Dir
{
  vector <string> files; 
  int num_of_files;
  int inodes[NUMOF_FILES_IN_DIR];
};

/*
   This stores the information for each inode.
   */
struct Inode
{
  int size;
  int type; //0 is file, 1 is directory
  int is_occ; // 0 if not occupied, 1 if occupied
  int position;
  int bid1; //indexes to the locations
};
/*
   This stores the information for each fd given 
   to a file when it is opened. 
   */
struct FD
{
  Inode * i; //pointer to the inode its fd for
  int offset; // file's internal pointer offeset
  int v_offset; //virtual offset for 
  int is_occ;
  int type; //0 for read
};
/*
   This stores information about the whole file system.
   */
struct FS 
{
  struct Inode inodeList[NUMOF_INODES]; //stores all the inodes
  int freeList[NUMOF_FREE_LIST]; //stores the number of free blocks
  struct FD fdList[NUMOF_FD]; // stores all the FDs
  int root_dir;
  int curr_dir;
};

//Functions used for the file system - explained in the documentation
void mkfs(FILE * dFile, FS * fs);
int open(FILE * dFile, FS * fs, string filename, int flag);
void write(FILE * dFile, FS * fs,int fd, string content); 
string read(FILE * dFile, FS * fs, int fd, int size);
void seek(FILE * dFile, FS * fs, int fd, int offset);
void close(FILE * dFile, FS * fs,int fd);
void mkdir(FILE * dFile, FS * fs, string dirname);
void rmdir(FILE * dFile, FS * fs, string dirname);
void cd(FILE * dFile, FS * fs, string dirname);
void ls(FILE * dFile, FS * fs);
void cat(FILE * dFile, FS * fs, string filename);
void imprt(FILE * dFile, FS * fs,string src,string dest);
void exprt(FILE * dFile, FS * fs,string src,string dest );
void tree(FILE * dFile, FS * fs, int level);

//helper functions
int getFD(FS * fs);
void removeBlock(FS *fs, int block);
int findBlock(struct FS *fs);
int getInode(struct FS *fs);

int getFD(FS * fs){
  int i;

  for(i=0;i<NUMOF_FD;i++){
    if(fs->fdList[i].is_occ == 0){ //getting a free fd
      fs->fdList[i].is_occ = 1;//setting it to occupied
      return i;

    }
  }
  cout << "No more FD available"<< endl;
}

int getInode(struct FS *fs){
  int i, rval;
  for(i=0;i<NUMOF_INODES;i++){
    if(fs->inodeList[i].is_occ == 0){//getting a free inode
      fs->inodeList[i].is_occ = 1;//setting it to occupied
      rval = i;
      break;
    }
  }
  return rval; //returning index to that inode found
}

int findBlock(struct FS *fs)
{
  int i, rval;
  for(i =0; i< NUMOF_FREE_LIST; i++)
  {
    if(fs->freeList[i] == 0) {//find freeblock 
      fs->freeList[i] = 1; //setting it to occupied
      rval = i; //getting the index
      break;
    }
  }
  return  TOTAL_INODES_BYTES + (SIZEOF_BLK*rval); //getting the location of that index.
}

void removeBlock(FS *fs, int block){


  block = block - (TOTAL_INODES_BYTES);  //getting index of the block
  block = block / SIZEOF_BLK;
  fs->freeList[block] = 0; //setting it to free again
  return;
}
//end of helper functions

void mkfs(FILE * dFile, FS * fs)
{
  //allocate 100 mb
  //decide where the inode stops and where the blocks start
  fclose(dFile);
  //wipes everything and rewrtrites mkfs
  dFile = fopen("virtual_disk.txt", "wb+");

  int i;
  Dir dir, dir2;

  if (dFile == NULL) perror ("Error opening file");

  //setting inodes,free blocks and fd's to being not occupied
  for(i=0;i<NUMOF_INODES;i++){
    fs->inodeList[i].is_occ = 0;
  }
  for(i=0;i<NUMOF_FREE_LIST;i++) {
    fs->freeList[i] = 0;
  }
  for(i=0;i<NUMOF_FD;i++){
    fs->fdList[i].is_occ = 0;
  }

  //set up root dir
  fs->root_dir = 0;
  fs->curr_dir = 0;
  fs->inodeList[0].is_occ=1;
  fs->inodeList[0].type=1;
  fs->inodeList[0].bid1 = findBlock(fs); //write this
  fs->inodeList[0].size=sizeof(struct Dir);

  rewind(dFile); // seek to begin
  fwrite(fs, sizeof(struct FS), 1, dFile);

  //write a subruoutine to set up root dir coz we can keep using it
  dir.inodes[0] = 0;
  dir.inodes[1] = 0;

  dir.files.push_back(".");
  dir.files.push_back("..");
  for(i = 2; i< NUMOF_FILES_IN_DIR; i++) {
    dir.inodes[i] = -1;
  }
  dir.num_of_files = dir.files.size();

  //write dir to blk1
  fseek(dFile, fs->inodeList[0].bid1, SEEK_SET);

  int ret =fwrite(&dir.num_of_files,sizeof(int),1,dFile);
  for(int a =0; a <= dir.files.size()-1; a++)
  {
    int num = dir.files[a].size()+1;
    fwrite(&num,sizeof(int),1,dFile);
    fwrite(dir.files[a].c_str(),sizeof(char),num,dFile);
    fwrite(&dir.inodes[a], sizeof(int),1,dFile);
  }

  rewind(dFile); 
  //fclose (dFile);
 
  return;
}// end of mkfs

int open(FILE * dFile, FS * fs, string filename, int flag )
{     
  int i, fdIndex,num_of_files,position, exists=0;
  struct FD *fd;
  Dir dir;
  vector<string> c;
  int nodes[NUMOF_FILES_IN_DIR];

  //seek to the location of of file to open
  fseek(dFile,fs->inodeList[fs->curr_dir].bid1,SEEK_SET);
  fread(&num_of_files, sizeof(int), 1, dFile);
  dir.num_of_files = num_of_files;

  //reading from the file
  for(int a =0; a < dir.num_of_files; a++)
  {
    int num2, in;
    fread(&num2, sizeof(int), 1, dFile);
    char mystring[num2+1];//null ter
    mystring[num2] = 0; //set it to null
    fread(&mystring[0],sizeof(char),(size_t)num2,dFile);
    c.push_back(string(mystring));
    fread(&in, sizeof(int),1,dFile);
    nodes[a]=in;
  }
  for(i =0; i< c.size(); i++) {
    dir.files.push_back(c[i]);
    dir.inodes[i] = nodes[i];
  }
  rewind(dFile);

  fdIndex = getFD(fs);

  //check if file exists
  for(i=0;i< dir.files.size();i++){

    //see if that file is in there    
    if(dir.files[i].compare(filename)== 0){
      fs->fdList[fdIndex].is_occ = 1;
      fs->fdList[fdIndex].offset = 0;

      //get the virtual offset and set it
      fs->fdList[fdIndex].v_offset = fs->inodeList[dir.inodes[i]].bid1;


      fs->fdList[fdIndex].i = &(fs->inodeList[dir.inodes[i]]);
      if(flag == 1) { //read
        fs->fdList[fdIndex].type = 0;
      }
      else if(flag == 2){
        fs->fdList[fdIndex].type = 1;
      }
      exists = 1;
    }
  }

  if(exists == 0){

    if(flag == 2){ //write or create file if dont exist

      fs->fdList[fdIndex].is_occ = 1;
      fs->fdList[fdIndex].offset = 0;
      fs->fdList[fdIndex].type = 1;

      //set struct vars
      position = getInode(fs);
      fs->inodeList[position].is_occ=1;
      fs->inodeList[position].type= 0;
      fs->inodeList[position].bid1 = findBlock(fs);
      fs->inodeList[position].size = 0;

      dir.files.push_back(filename);
      dir.num_of_files = dir.num_of_files+1;
      dir.inodes[dir.num_of_files-1] = position;

      fs->fdList[fdIndex].v_offset = fs->inodeList[position].bid1;
      fs->fdList[fdIndex].i = &(fs->inodeList[position]);

      fseek(dFile,fs->inodeList[fs->curr_dir].bid1,SEEK_SET);
      fwrite(&dir.num_of_files,sizeof(int),1,dFile);
      for(int a =0; a < dir.files.size(); a++)
      {
        int num = dir.files[a].size()+1;
        fwrite(&num,sizeof(int),1,dFile);
        fwrite(dir.files[a].c_str(),sizeof(char),num,dFile);
        fwrite(&dir.inodes[a], sizeof(int),1,dFile);
      }
      rewind(dFile);

      Dir dir2;
      vector <string> c2;
      int num_of_files2,in2;
      int nodes2[NUMOF_FILES_IN_DIR];

      fseek(dFile,fs->inodeList[fs->curr_dir].bid1,SEEK_SET);
      fread(&num_of_files2, sizeof(int), 1, dFile);
      dir2.num_of_files = num_of_files2;

      for(int a =0; a < dir2.num_of_files; a++)
      {
        int num2;
        fread(&num2, sizeof(int), 1, dFile);
        char mystring[num2+1];//null ter
        mystring[num2] = 0; //set it to null
        fread(&mystring[0],sizeof(char),(size_t)num2,dFile);
        c2.push_back(string(mystring));
        fread(&in2, sizeof(int),1,dFile);
        nodes2[a]=in2;
      }
      for(i =0; i< c2.size(); i++) {
        dir2.files.push_back(c2[i]);
        dir.inodes[i] = nodes2[i];
      }
      rewind(dFile);
    }
    else
    {
      //error occured
      fs->fdList[fdIndex].is_occ = 0;
      fdIndex = -1;
    }
  }
  return fdIndex;
}

void write(FILE * dFile, FS * fs,int fd,string content){

  //if the file exists
  if(fs->fdList[fd].is_occ == 1)
  {
    fseek(dFile,fs->fdList[fd].v_offset, SEEK_SET);

    int csize = content.size()+1;
    fwrite(content.c_str(),sizeof(char),csize,dFile);

    fs->fdList[fd].i->size += csize;
    fs->fdList[fd].offset += csize;
    fs->fdList[fd].v_offset += csize;

    rewind(dFile);
  }
  else
    cout<< "The file you are trying to write to is closed." <<endl;

}

string read(FILE * dFile, FS * fs, int fd, int size) {
  char content[size + 1];
  content[size] = 0;
  //if the file exits
  if(fs->fdList[fd].is_occ == 1) {
    fseek(dFile,fs->fdList[fd].v_offset,SEEK_SET);
    int rval = fread(&content[0],sizeof(char),(size_t)size,dFile);
    cout<< content << endl;
    rewind(dFile);
  }
  else{
    cout<< "The file you are trying to read from is closed." <<endl;
  }
  return content;
}

void seek(FILE * dFile, FS * fs,int fd, int offset) {
  fs->fdList[fd].offset = offset;
  fs->fdList[fd].v_offset = fs->fdList[fd].i->bid1+offset;

}

void close(FILE * dFile, FS * fs,int fd) {

  if(fd>=0 && fd < NUMOF_FILES_IN_DIR )
  {
    fs->fdList[fd].is_occ = 0;
  }
  else
    cout<< "Invalid fd." <<endl;

}

void mkdir(FILE * dFile, FS * fs, string dirname) {

  int num_of_files, i, position;
  Dir dir, dir2;
  vector <string> c;
  int nodes[NUMOF_FILES_IN_DIR];

  //read in current dir from file
  fseek(dFile,fs->inodeList[fs->curr_dir].bid1,SEEK_SET);
  fread(&num_of_files, sizeof(int), 1, dFile);
  dir.num_of_files = num_of_files;

  for(int a =0; a < dir.num_of_files; a++)
  {
    int num2, in;
    fread(&num2, sizeof(int), 1, dFile);
    char mystring[num2+1];//null ter
    mystring[num2] = 0; //set it to null
    fread(&mystring[0],sizeof(char),(size_t)num2,dFile);
    c.push_back(string(mystring));
    fread(&in, sizeof(int),1,dFile);
    nodes[a]=in;
  }
  for(i =0; i< c.size(); i++) {
    dir.files.push_back(c[i]);
    dir.inodes[i] = nodes[i];
  }
  rewind(dFile);

  //check if it already exists in the curr_dir
  for(i =0; i< dir.files.size(); i++) {
    if(dir.files[i].compare(dirname) == 0) {
      cout << "Directory already exists." << endl;
      return;
    }
  } 
  //search for open inodes
  position = getInode(fs);
  fs->inodeList[position].is_occ=1;
  fs->inodeList[position].type= 1;
  fs->inodeList[position].bid1 = findBlock(fs); //write this
  //fs->inodeList[position].bid2 = 0;
  //fs->inodeList[position].bid3 = 0;
  fs->inodeList[position].size = sizeof(struct Dir); //cannot do this

  dir.files.push_back(dirname);
  dir.num_of_files = dir.num_of_files+1;
  dir.inodes[dir.num_of_files-1] = position;

  //create new dir
  dir2.inodes[0] = position;
  dir2.inodes[1] = fs->curr_dir;
  dir2.files.push_back(".");
  dir2.files.push_back("..");
  for(i = 2; i< NUMOF_FILES_IN_DIR; i++) {
    dir2.inodes[i] = -1;
  }
  dir2.num_of_files = dir2.files.size();

  //write dir to blk1
  fseek(dFile, fs->inodeList[fs->curr_dir].bid1, SEEK_SET);
  fwrite(&dir.num_of_files,sizeof(int),1,dFile);
  for(int a =0; a <= dir.files.size()-1; a++)
  {
    int num = dir.files[a].size()+1;
    fwrite(&num,sizeof(int),1,dFile);
    fwrite(dir.files[a].c_str(),sizeof(char),num,dFile);
    fwrite(&dir.inodes[a], sizeof(int),1,dFile);
  }
  rewind(dFile); 

  //writing the new dir
  fseek(dFile, fs->inodeList[position].bid1, SEEK_SET);
  fwrite(&dir2.num_of_files,sizeof(int),1,dFile);
  for(int a =0; a <= dir2.files.size()-1; a++)
  {
    int num = dir2.files[a].size()+1;
    fwrite(&num,sizeof(int),1,dFile);
    fwrite(dir2.files[a].c_str(),sizeof(char),num,dFile);
    fwrite(&dir2.inodes[a], sizeof(int),1,dFile);
  }
  rewind(dFile);
}

void rmdir(FILE * dFile, FS * fs, string dirname) {

  //read dir from file
  int num_of_files, i, position,num_of_files2, flag = 0, p;
  Dir dir, dir2;
  vector <string> c;
  vector <string> c2;
  int nodes[NUMOF_FILES_IN_DIR];
  int nodes2[NUMOF_FILES_IN_DIR];

  //read in current dir from file
  fseek(dFile,fs->inodeList[fs->curr_dir].bid1,SEEK_SET);
  fread(&num_of_files, sizeof(int), 1, dFile);
  dir.num_of_files = num_of_files;

  for(int a =0; a < dir.num_of_files; a++)
  {
    int num2, in;
    fread(&num2, sizeof(int), 1, dFile);
    char mystring[num2+1];//null ter
    mystring[num2] = 0; //set it to null
    fread(&mystring[0],sizeof(char),(size_t)num2,dFile);
    c.push_back(string(mystring));
    fread(&in, sizeof(int),1,dFile);
    nodes[a]=in;
  }
  for(i =0; i< c.size(); i++) {
    dir.files.push_back(c[i]);
    dir.inodes[i] = nodes[i];
  }
  rewind(dFile);


  //ensure its not a . or ..
  if(dirname.compare(".") == 0 || dirname.compare("..") == 0 ) {
    cout << "Cant delete roots dirs . and .. " << endl;
  }

  //check if it already exists in the curr_dir
  for(p =0; p< dir.files.size(); p++) {
    if(dir.files[p].compare(dirname) == 0) {
      flag = 1; // it exists
      i = p;
      break;
    }
  }

  if(flag == 1) {
  
    if(fs->inodeList[dir.inodes[i]].type == 1){ //its a dir 

      //read the dir to remove
      fseek(dFile,fs->inodeList[dir.inodes[i]].bid1,SEEK_SET);
      fread(&num_of_files2, sizeof(int), 1, dFile);
      dir2.num_of_files = num_of_files2;

      for(int a =0; a < dir2.num_of_files; a++)
      {
        int num2, in;
        fread(&num2, sizeof(int), 1, dFile);
        char mystring[num2+1];//null ter
        mystring[num2] = 0; //set it to null
        fread(&mystring[0],sizeof(char),(size_t)num2,dFile);
        c2.push_back(string(mystring));
        fread(&in, sizeof(int),1,dFile);
        nodes2[a]=in;
      }
      for(int j =0; j< c2.size(); j++) {
        dir2.files.push_back(c2[j]);
        dir2.inodes[j] = nodes2[j];
      }
      rewind(dFile);

      //free block
      removeBlock(fs,fs->inodeList[dir.inodes[i]].bid1);
      fs->inodeList[dir.inodes[i]].is_occ = 0;
      dir.files.erase(dir.files.begin() + i);
      dir.num_of_files = dir.num_of_files -1;
      
      
      //writing with the file removed
      fseek(dFile, fs->inodeList[fs->curr_dir].bid1, SEEK_SET);
      fwrite(&dir.num_of_files,sizeof(int),1,dFile);
      for(int a =0; a <= dir.files.size()-1; a++)
      {
        int num = dir.files[a].size()+1;
        fwrite(&num,sizeof(int),1,dFile);
        int rv = fwrite(dir.files[a].c_str(),sizeof(char),num,dFile);
        fwrite(&dir.inodes[a], sizeof(int),1,dFile);
      }
      rewind(dFile); 
    }
    else
    {
      cout << "It is not a directory." << endl;
    }
  }
  else {
    cout << "The directory does not exist." << endl;
  }

}

void cd(FILE * dFile, FS * fs, string dirname) {

  string rest;
  int num_of_files,i, flag;
  vector <string> c;
  int nodes[NUMOF_FILES_IN_DIR];
  Dir dir;
  //if it starts wit a / then its root
  if(dirname[0] == '/') {
    fs->curr_dir = fs->root_dir;
    dirname.erase(0,1);
  }
  //find the next /
  size_t pos = dirname.find('/');
  if(!rest.empty()) {
    //get name after /
    rest = dirname.substr(pos);
    rest.erase(0,1);
  }

  //read from the file
  fseek(dFile,fs->inodeList[fs->curr_dir].bid1,SEEK_SET);
  fread(&num_of_files, sizeof(int), 1, dFile);
  dir.num_of_files = num_of_files;

  for(int a =0; a < dir.num_of_files; a++)
  {
    int num2, in;
    fread(&num2, sizeof(int), 1, dFile);
    char mystring[num2+1];//null ter
    mystring[num2] = 0; //set it to null
    fread(&mystring[0],sizeof(char),(size_t)num2,dFile);
    c.push_back(string(mystring));
    fread(&in, sizeof(int),1,dFile);
    nodes[a]=in;
  }
  for(i =0; i< c.size(); i++) {
    dir.files.push_back(c[i]);
    dir.inodes[i] = nodes[i];
  }
  rewind(dFile);

  for(int a =0; a < dir.files.size(); a++){
    if(dir.files[a].compare(dirname) == 0) {
      flag = 1;
      if(fs->inodeList[dir.inodes[a]].type == 1){
        fs->curr_dir = dir.inodes[a];
        if(!rest.empty()) cd(dFile, fs, rest);
      }
      else
      {
        cout<< "Bad Input: Cannot cd to a non directory."<<endl;
      }
      break;   
    }
  }
  if(flag == 0) cout << "Cannot find the directory." << endl;
}

void ls(FILE * dFile, FS * fs) {

  int num_of_files,i, flag;
  vector <string> c;
  int nodes[NUMOF_FILES_IN_DIR];
  Dir dir;

  fseek(dFile,fs->inodeList[fs->curr_dir].bid1,SEEK_SET);
  fread(&num_of_files, sizeof(int), 1, dFile);
  dir.num_of_files = num_of_files;

  for(int a =0; a < dir.num_of_files; a++)
  {
    int num2, in;
    fread(&num2, sizeof(int), 1, dFile);
    char mystring[num2+1];//null ter
    mystring[num2] = 0; //set it to null
    fread(&mystring[0],sizeof(char),(size_t)num2,dFile);
    c.push_back(string(mystring));
    fread(&in, sizeof(int),1,dFile);
    nodes[a]=in;
  }
  for(i =0; i< c.size(); i++) {
    dir.files.push_back(c[i]);
    dir.inodes[i] = nodes[i];
  }
  rewind(dFile);

  //printing out files in current directory
  for(i = 0; i< dir.files.size(); i++) {
    cout << dir.files[i] << endl;
  }

}

void cat(FILE * dFile, FS * fs, string filename) {

  int mode = 0;
  int fd = open(dFile, fs, filename, mode);
  int size = fs->fdList[fd].i->size;
  read(dFile, fs, fd,size);  
}

void imprt(FILE * dFile, FS *fs, string src,string dest){


  ifstream f(src.c_str());
  string content((istreambuf_iterator<char>(f) ),istreambuf_iterator<char>() );
  //check if size is greater than 96000
  int size = content.size();
  if( size > SIZEOF_BLK){
    cout << "File is too large" << endl;
  }
  cout << content << endl;
  int fd = open(dFile,fs,dest,2);
  cout << "fd is" << fd <<endl; 
  write(dFile,fs,fd,content);

  f.close();
  close(dFile,fs,fd);

}

void exprt(FILE * dFile, FS * fs,string src,string dest ){

  //open the file and set the size
  int fd = open(dFile, fs, src,1);
  int size = fs->fdList[fd].i->size;
  //call read func and get return vals
  string content = read(dFile, fs, fd,size);
  ofstream outfile(dest.c_str());
  //output to the file in virtual file
  outfile << content << endl;

  close(dFile,fs,fd);
  outfile.close();

}

void tree(FILE * dFile, FS * fs, int level){

  int num_of_files,i,j;
  vector<string> c;
  int nodes[NUMOF_FILES_IN_DIR];
  Dir dir;
  //seek to the location of file and read
  fseek(dFile,fs->inodeList[fs->curr_dir].bid1,SEEK_SET);
  fread(&num_of_files, sizeof(int), 1, dFile);
  dir.num_of_files = num_of_files;

  for(int a =0; a < dir.num_of_files; a++)
  {
    int num2, in;
    fread(&num2, sizeof(int), 1, dFile);
    char mystring[num2+1];//null ter
    mystring[num2] = 0; //set it to null
    fread(&mystring[0],sizeof(char),(size_t)num2,dFile);
    c.push_back(string(mystring));
    fread(&in, sizeof(int),1,dFile);
    nodes[a]=in;
  }
  for(i =0; i< c.size(); i++) {
    dir.files.push_back(c[i]);
    dir.inodes[i] = nodes[i];
  }
  rewind(dFile);

  for(i=0; i< dir.files.size(); i++) {
    //cout << dir.files[i] << endl;

    for(j=0; j< level; j++) {
      cout << "-";
    }
    //check if its a regular file
    if(fs->inodeList[dir.inodes[i]].type == 0 || dir.files[i].compare(".") == 0 ||     dir.files[i].compare("..") == 0 ) {

      if( dir.files[i].compare(".") == 0 || dir.files[i].compare("..") == 0) {
        cout << dir.files[i] << endl;
      }
      else{
        cout << dir.files[i] << " " << fs->inodeList[dir.inodes[i]].size << endl;
      }
    }
    else { //its a dir
      cout << dir.files[i] << endl;

      cd(dFile,fs,dir.files[i]);

      tree(dFile,fs,level+1);

      cd(dFile,fs,"..");

    }
  }
  return;
}

int main(int argc, char **argv ) {
  string dirname,tmp,source,dest,command,cm2,cm3,filename,content,c;
  int flag,fd,given_fd,size,offset;
  FS * fs = (struct FS*)malloc(sizeof(struct FS));
  FILE *dFile;

  struct stat buffer;
  if (stat("virtual_disk.txt", &buffer) != 0)
    dFile = fopen ("virtual_disk.txt","wb+");
  else
    dFile = fopen ("virtual_disk.txt","ab+");
  //create file of 100mb
  fseek(dFile,100000000,SEEK_SET);
  fputs("EOF",dFile);
  rewind(dFile);

  istringstream iss;
  //read in commands until user terminates the program
  while(1){
    cout << "Divron:  " << endl;
    getline(cin,tmp);
    istringstream iss(tmp);
    iss >> command; 

    if(command == "mkfs"){
      mkfs(dFile, fs);
    }
    else if(command == "open"){
      iss >> filename >> flag;
      fd = open(dFile, fs,filename, flag);
      cout << "SUCCESS,fd=" << fd << endl;
    }
    else if(command == "write"){
      iss >> given_fd;
      while(iss >> c){
        content += c + " "; 
      }
      write(dFile,fs,given_fd, content);
    }
    else if(command == "read"){
      iss >> given_fd >> size;
      read(dFile, fs,given_fd, size);
    }
    else if(command == "seek"){
      iss >> given_fd >> offset;
      seek(dFile,fs, given_fd, offset);
    }
    else if(command == "close"){
      iss >> given_fd;
      close(dFile,fs,given_fd);
    }
    else if(command == "mkdir"){
      iss >> dirname;
      mkdir(dFile,fs,dirname);
    }
    else if(command == "rmdir"){
      iss >> dirname;
      rmdir(dFile,fs,dirname);
    }
    else if(command == "cd"){
      iss >> dirname;
      cd(dFile,fs,dirname);
    }
    else if(command == "ls"){
      ls(dFile,fs);
    }
    else if(command == "cat"){
      cat(dFile,fs,filename);
    }
    else if(command == "tree"){
      tree(dFile,fs, 0);
    }
    else if(command == "import"){
      iss>> source >> dest;
      imprt(dFile,fs,source,dest);
    }
    else if(command == "export"){
      iss>> source >> dest;
      exprt(dFile,fs,source,dest);

    }
  }
}

