#include "datanode.h"
#include <arpa/inet.h>
#include "extent_client.h"
#include <unistd.h>
#include <algorithm>
#include "threader.h"

#define DB 1

using namespace std;

int DataNode::init(const string &extent_dst, const string &namenode, const struct sockaddr_in *bindaddr) {
  #if DB
  cout << "init data node, name:"<< namenode << endl;
  cout.flush();
  #endif
  
  ec = new extent_client(extent_dst);

  // Generate ID based on listen address
  id.set_ipaddr(inet_ntoa(bindaddr->sin_addr));
  id.set_hostname(GetHostname());
  id.set_datanodeuuid(GenerateUUID());
  id.set_xferport(ntohs(bindaddr->sin_port));
  id.set_infoport(0);
  id.set_ipcport(0);

  // Save namenode address and connect
  make_sockaddr(namenode.c_str(), &namenode_addr);
  if (!ConnectToNN()) {
    #if DB
    cout << "connect name failed:" << endl;
    cout.flush();
    #endif

    delete ec;
    ec = NULL;
    return -1;
    
  }

  // Register on namenode
  if (!RegisterOnNamenode()) {
    #if DB
    cout << "register name failed:" << endl;
    cout.flush();
    #endif

    delete ec;
    ec = NULL;
    close(namenode_conn);
    namenode_conn = -1;
    return -1;
  }
  
  NewThread(this, &DataNode::hearttime);
  
  /* Add your initialization here */
  if (ec->put(1, "") != extent_protocol::OK)
      printf("error init root dir\n"); // XYB: init root dir
  
  return 0;
}

void DataNode::hearttime(){
  while(true){
    SendHeartbeat();
    sleep(1);
  }
}

bool DataNode::ReadBlock(blockid_t bid, uint64_t offset, uint64_t len, string &buf) {
  #if DB
  cout << "read block:" << bid << " offset:" << offset << " len:" << len << endl;
  cout.flush();
  #endif

  string buf1;
  int r;
  r = ec->read_block(bid, buf1);
  if (offset > buf1.size())
    buf = "";
  else
    buf = buf1.substr(offset, len);
  
  /* Your lab4 part 2 code */
  return true;
}

bool DataNode::WriteBlock(blockid_t bid, uint64_t offset, uint64_t len, const string &buf) {
  #if DB
  cout << "write block:" << bid << " offset:" << offset << " len:" << len << "buf size" << buf.size() << endl;
  cout.flush();
  #endif

  string buf2;
  ec->read_block(bid, buf2);
  buf2 = buf2.substr(0, offset) + buf + buf2.substr(offset + len);
  ec->write_block(bid, buf2);

  return true;
}

