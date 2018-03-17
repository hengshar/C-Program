#include <stdio.h>  
#include <stdlib.h>  
#include <string.h>  
#include <sys/ioctl.h>
#include <unistd.h>
#include <dirent.h>

int interactive = 1;
char cmd[256];

typedef struct keyAndValue
{
    char key[255];
    char value[255];
    short offset;
    struct keyAndValue *next;
}kv;

void addValue(kv **keyValue,char *key,char *value,int offset)
{
    kv *tmp = NULL,*ntmp = NULL;
    //char t[255]  =  {0};
    ntmp = (kv *)malloc(sizeof(kv));
    memset(ntmp,0,sizeof(kv));
    strcpy(ntmp->key,key);
    strcpy(ntmp->value,value);
    ntmp->offset = offset;
    if(*keyValue == NULL)
    {
       *keyValue = ntmp;
    }
    else
    {
       tmp = *keyValue;
       while (tmp->next != NULL)
       {
           tmp = tmp->next;
       }
       tmp->next = ntmp;
    }
}

void printKeyValue(kv *keyValue)
{
    kv *kvalue = keyValue;
    kv *pE = NULL;
    struct winsize info;
    int winWidth = 0;
    ioctl(STDIN_FILENO,TIOCGWINSZ,&info);
    winWidth = (info.ws_col)/3;
    //printf("当前终端为%d行%d列\n",info.ws_row,info.ws_col);
    //printf("新宽度:%d\n",winWidth);
    while(kvalue != NULL)
    {
        printf("%-*s",winWidth + kvalue->offset,kvalue->key);
        if(kvalue->value != NULL && 0 != strlen(kvalue->value))
        {
            printf("[");
            printf("\033[01;33m%s\033[0m",kvalue->value);
            printf("]\n");
        }
        else{printf("\n");}
        pE = kvalue;
        kvalue = kvalue->next;
        free(pE);
    }
    printf("\n");
}

int  parseKeyValue(kv ** keyValue,char *command,char *formart)
{
    char str[4096] = {0};
    char tmpLine[1024] = {0};
    FILE *pf;
    int i = 0;
    char key[512] = {0};
    char value[512] = {0};
    if(command != NULL)
    {
        pf = popen(command,"r");
        fread(str,sizeof(str),1,pf);
        pclose(pf);
        //printf("%s",str);
        if(str == NULL && str[0] != '\0')
        {
            return 0;
        }
        else
        {
            for(i = 0;str[i] != '\0';i++)
            {
                if(str[i] == '\n')
                {
                    if(tmpLine != NULL)
                    {
                        memset(key,0,sizeof(key));
                        memset(value,0,sizeof(value));
                        sscanf(tmpLine,formart,key,value);
                        addValue(keyValue,key,value,0);                    
                    }
                    memset(tmpLine,0,sizeof(tmpLine));
                }
                else
                {
                    tmpLine[strlen(tmpLine)] = str[i];
                }
            }
        }
    }
    else
    {
        return 1;
    }
}
//get file content
char * getFileContent(char *filepath)
{
    if(0 == strlen(filepath))
        return "";
    FILE *fp = fopen(filepath, "r");
    if(NULL == fp)
        printf("failed to open file %s\n",filepath);
   // char *szTest  = (char *)calloc(1000,sizeof(char *));
    char szTest[1024] = {0};
    memset(szTest, 0, sizeof(szTest));
    char *r;
    while(!feof(fp))
    {
        fgets(szTest, sizeof(szTest) - 1, fp); // leave out \n  
    }
    fclose(fp);
    r = szTest;
    if('\n' == szTest[strlen(szTest) - 1])
        szTest[strlen(szTest) - 1] = '\0';
    return r;
}

// read the linux config files to get system info  
void getOsInfo()  
{  
    FILE *fp = fopen("/proc/version", "r");    
    if(NULL == fp)     
        printf("failed to open version\n");     
    char szTest[1000] = {0};    
    while(!feof(fp))    
    {    
        memset(szTest, 0, sizeof(szTest));    
        fgets(szTest, sizeof(szTest) - 1, fp); // leave out \n  
        printf("%s", szTest);    
    }    
    fclose(fp);   
}  

void getCpuInfo()  
{
    kv * keyValue = NULL,*pE=NULL,*cpuInfo = NULL;
    char architecture[64] = {0};
    char modelname[64] = {0};
    char cpus[64] = {0};
    char threadspercore[64] = {0};
    char corespersocket[64] = {0};
    char sockets[64] = {0};
    parseKeyValue(&keyValue,"lscpu","%[^:]: %[^&]");
    while(keyValue != NULL)
    {
        if(strncmp("Architecture",keyValue->key,sizeof("Architecture")) == 0)
        {
             strcpy(architecture,keyValue->value);
        }
        else if(strncmp("CPU(s)",keyValue->key,sizeof("CPU(s)")) == 0)
        {
             strcpy(cpus,keyValue->value);
        }
        else if(strncmp("Model name",keyValue->key,sizeof("Model name")) == 0)
        {
             strcpy(modelname,keyValue->value);
        }
        else if(strncmp("Thread(s) per core",keyValue->key,sizeof("Thread(s) per core")) == 0)
        {
             strcpy(threadspercore,keyValue->value);
        }
        else if(strncmp("Core(s) per socket",keyValue->key,sizeof("Core(s) per socket")) == 0)
        {
             strcpy(corespersocket,keyValue->value);
        }
        else if(strncmp("Socket(s)",keyValue->key,sizeof("Socket(s)")) == 0)
        {
             strcpy(sockets,keyValue->value);
        }
        pE = keyValue;
        keyValue = keyValue -> next;
        free(pE);
    }
    addValue(&cpuInfo,"CPU 型号",modelname,2);
    addValue(&cpuInfo,"CPU 架构",architecture,2);
    addValue(&cpuInfo,"CPU 路数",sockets,2);
    addValue(&cpuInfo,"单路核心数",corespersocket,5);
    addValue(&cpuInfo,"每核心线程数",threadspercore,6);
    addValue(&cpuInfo,"总线程数",cpus,4);
    printKeyValue(cpuInfo);
}

void getMemoryInfo()  
{
    kv * keyValue = NULL,*pE=NULL,*memInfo = NULL;
    float intTotalMem = 0;
    int unit = 1;
    char tmpvalue[64] = {0};
    char tmpunit[8] = {0};
    parseKeyValue(&keyValue,"dmidecode -t memory | grep Size","%[^:]: %[^&]");
    while (keyValue != NULL)
    {
        sscanf(keyValue->value,"%[^A-Z]%s",tmpvalue,tmpunit);
        if(strncmp(tmpunit,"GB",2) == 0)
        {
            unit = 1024;
        }
        else if(strncmp(tmpunit,"MB",2) == 0)
        {
            unit = 1;
        }
        else{unit = 0;};
        intTotalMem += atoi(tmpvalue)*unit;
        pE = keyValue;
        keyValue = keyValue -> next;
        free(pE);
    }
    intTotalMem = intTotalMem / unit;
    sprintf(tmpvalue,"%d%s",(int)intTotalMem,tmpunit);
    addValue(&memInfo,"总缓存",tmpvalue,3);
    parseKeyValue(&keyValue,"free -h | grep 'Mem'","%[^:]: %*s %s");
    memset(tmpvalue,0,sizeof(tmpvalue));
    while (keyValue != NULL)
    {
        strcpy(tmpvalue,keyValue->value);
        pE = keyValue;
        keyValue = keyValue->next;
        free(pE);
    }
    addValue(&memInfo,"正在使用的缓存",tmpvalue,7);
    printKeyValue(memInfo);
}  

void getIscsiInfo()
{
    kv * keyValue = NULL,*pE=NULL,*iscsiInfo = NULL,*ethtoolInfo=NULL;
    int ishead = 1;
    char ipaddr[64] = {0};
    char netmask[64] = {0};
    char ethname[64] = {0};
    char iscsioffset[64] = "              ";
    char ipaddrhead[64] = {0};
    char netmaskhead[64] = {0};
    char porttypehead[64] = {0};
    char porttype[64] = {0};
    char linkmode1[64] = {0};
    char linkmodehead[64] = {0};
    char linkmode2[64] = {0};
    char linkmode3[64] = {0};
    char speed[64] = {0};
    char speedhead[64] = {0};
    char linkdetected[64] = {0};
    char linkdthead[64] = {0};
    char tmpvalue[64] = {0};
    char ethtoolcmd[64] = {0};
    
    memset(ethtoolcmd,0,sizeof(ethtoolcmd));
    sprintf(ipaddrhead,"%sIP 地址",iscsioffset);
    sprintf(netmaskhead,"%s子网掩码",iscsioffset);
    sprintf(porttypehead,"%s端口类型",iscsioffset);
    sprintf(linkmodehead,"%s支持连接模式",iscsioffset);
    sprintf(speedhead,"%s当前速率",iscsioffset);
    sprintf(linkdthead,"%s连接状态",iscsioffset);
    parseKeyValue(&keyValue,"ifconfig -a","%[^=]=");
    while(keyValue)
    {
        if(ishead == 1)
        {
            //网络接口名称
            memset(ethname,0,sizeof(ethname));
            sscanf(keyValue->key,"%s",ethname);
            sprintf(tmpvalue,"网络接口 %s",ethname);
            addValue(&iscsiInfo,tmpvalue,keyValue->value,0);
            ishead = 2;
            //ethtool
            sprintf(ethtoolcmd,"ethtool %s",ethname);
            parseKeyValue(&ethtoolInfo,ethtoolcmd," %[^:]: %[^&]");
            while(ethtoolInfo)
            {
                if(strncmp("Supported ports",ethtoolInfo->key,sizeof("Supported ports")) == 0)
                {
                     sscanf(ethtoolInfo->value,"[ %s ]",porttype);
                }
                else if(strncmp("Supported link modes",ethtoolInfo->key,sizeof("Supported link modes")) == 0)
                {
                     strcpy(linkmode1,ethtoolInfo->value);
                     pE = ethtoolInfo;
                     ethtoolInfo = ethtoolInfo->next;
                     free(pE);
                     memset(linkmode2,0,sizeof(linkmode2));
                     memset(linkmode3,0,sizeof(linkmode3));
                     if(0 == strncmp("Supported pause frame use",ethtoolInfo->key,sizeof("Supported pause frame use")))
                         continue;
                     else
                         strcpy(linkmode2,ethtoolInfo->key);
                     pE = ethtoolInfo;
                     ethtoolInfo = ethtoolInfo->next;
                     free(pE);
                     if(0 == strncmp("Supported pause frame use",ethtoolInfo->key,sizeof("Supported pause frame use")))
                         continue;
                     else
                         strcpy(linkmode3,ethtoolInfo->key);

                }
                else if(strncmp("Speed",ethtoolInfo->key,sizeof("Speed")) == 0)
                {
                     strcpy(speed,ethtoolInfo->value);
                }
                else if(strncmp("Link detected",ethtoolInfo->key,sizeof("Link detected")) == 0)
                {
                     strcpy(linkdetected,ethtoolInfo->value);
                }

                pE = ethtoolInfo;
                ethtoolInfo = ethtoolInfo->next;
                free(pE);
            }
            if(0 == strncmp("yes",linkdetected,sizeof("yes")))
            {
                 strcpy(linkdetected,"已连接");
            }
            else if(0 == strncmp("no",linkdetected,sizeof("no")))
            {
                 strcpy(linkdetected,"未连接");
            }
            else
            {
                 strcpy(linkdetected,"未知");
            }
            memset(tmpvalue,0,sizeof(tmpvalue));
            sscanf(speed,"%[0-9]",tmpvalue);
            if(strlen(tmpvalue) > 0)
                 strcat(tmpvalue,"Mbps ");
            else
                 strcpy(speed,"未知");
            if(0 == strncmp("TP",porttype,sizeof("TP")))
            {   
                 //sprintf(porttype,"%sRJ-45 电口",tmpvalue);
                 strcpy(porttype,"RJ-45 电口");
            }   
            else if(0 == strncmp("FIBRE",porttype,sizeof("FIBRE")))
            {   
                 //sprintf(porttype,"%s单模光口",tmpvalue);
                 strcpy(porttype,"单模光口");
            }   
            else
            {   
                 strcpy(porttype,"未知");
            }   
            addValue(&iscsiInfo,porttypehead,porttype,4);
            addValue(&iscsiInfo,linkdthead,linkdetected,4);
            addValue(&iscsiInfo,speedhead,speed,4);
            addValue(&iscsiInfo,linkmodehead,linkmode1,6);
            if(strlen(linkmode2) > 0)
                 addValue(&iscsiInfo,"",linkmode2,0);
            if(strlen(linkmode3) > 0)
                 addValue(&iscsiInfo,"",linkmode3,0);
        }
        else if(ishead == 2)
        {
            //网络数据
            memset(ipaddr,0,sizeof(ipaddr));
            memset(netmask,0,sizeof(netmask));
            //printf("%s\n",keyValue->key);
            sscanf(keyValue->key," inet %s netmask %s",ipaddr,netmask);
           // printf("%s\n",ipaddr);
            //printf("%s\n",netmask);
            addValue(&iscsiInfo,ipaddrhead,ipaddr,2);
            addValue(&iscsiInfo,netmaskhead,netmask,4);
            ishead = 0;
        }
        else if(keyValue->key == NULL || 0 == strlen(keyValue->key))
        {
            ishead = 1;
        }
        pE = keyValue;
        keyValue = keyValue->next;
        free(pE);
    }
    printKeyValue(iscsiInfo);
}

void getFcInfo()
{
    kv *fcInfo=NULL;
    DIR *dirp; 
    struct dirent *dp;
    char dirpath[64] = {0};
    char filepath[64] = {0};
    char t_speed[128] = {0};
    char t_nodename[128]= {0};
    char t_supportspeed[128]= {0};
    char t_supportspeeds[256]= {0};
    char t_state[128]= {0};
    char *speed = NULL,*nodename = NULL,*supportspeed = NULL,*state = NULL;
    char fcoffset[64] = "              ";
    char speedhead[128] = {0};
    char nodenamehead[128] = {0};
    char supportspeedhead[128] = {0};
    char statehead[128] = {0};
    char typehead[128] = {0};
    char title[128] = "光纤接口";
    char titlestr[128] = {0};
    char tmpvalue[128] = {0};
    char unit[8] = "G";
    char tmp[1] = "";
    memset(tmpvalue,0,sizeof(tmpvalue));
    sprintf(speedhead,"%s当前速率",fcoffset);
    sprintf(nodenamehead,"%sWWPN",fcoffset);
    sprintf(supportspeedhead,"%s支持连接模式",fcoffset);
    sprintf(statehead,"%s连接状态",fcoffset);
    sprintf(typehead,"%s端口类型",fcoffset);
    strcpy(dirpath,"/sys/class/fc_host/");
    if(access(dirpath,0))
    {
        printf("directory '%s' is not exist\n",dirpath);
        return;
    }
    dirp = opendir(dirpath); //打开目录指针
    while ((dp = readdir(dirp)) != NULL) { //通过目录指针读目录
        if(0 != strncmp(".",dp->d_name,sizeof(".")) && 0 != strncmp("..",dp->d_name,sizeof(".")))
        {
            memset(titlestr,0,sizeof(titlestr));
            sprintf(titlestr,"%s %s:",title,dp->d_name);
            addValue(&fcInfo,titlestr,"",4);
 
            addValue(&fcInfo,typehead,"16Gbps 自适应端口",4);
            //state
            memset(t_state,0,sizeof(t_state));
            sprintf(filepath,"%s%s/port_state",dirpath,dp->d_name);
            state = getFileContent(filepath);
            strcpy(tmpvalue,state);
            sscanf(tmpvalue,"%[^\n]\n",t_state);
            if(0 == strncmp("Online",t_state,sizeof("Online")))
                 strcpy(t_state,"已连接");
            else if(0 == strncmp("Linkdown",t_state,sizeof("Linkdown")))
                 strcpy(t_state,"未连接");
            else
                 strcpy(t_state,"未知");
            addValue(&fcInfo,statehead,t_state,4);

            //speed
            sprintf(filepath,"%s%s/speed",dirpath,dp->d_name);
            speed = getFileContent(filepath);
            strcpy(tmpvalue,speed);
            if(0 == strncmp("unknown",tmpvalue,sizeof("unknown")))
                strcpy(t_speed,"未连接");
            else
            {
                sscanf(tmpvalue,"%[^ ] %1s",t_speed,unit);
                sprintf(t_speed,"%s%sbps",t_speed,unit);
            }
            addValue(&fcInfo,speedhead,t_speed,4);
            
            //support speed
            sprintf(filepath,"%s%s/supported_speeds",dirpath,dp->d_name);
            supportspeed = getFileContent(filepath);
            strcpy(tmpvalue,supportspeed);
            memset(t_supportspeeds,0,sizeof(t_supportspeeds));
            strcpy(tmp, ",");
            while(0 == strncmp(tmp,",",sizeof(",")))
            {  
                //memset(unit,0,sizeof(unit));
                sscanf(tmpvalue,"%[^ ] %1s%*[^,]%1s %[^&]",t_supportspeed,unit,tmp,tmpvalue);
                sprintf(t_supportspeeds,"%s%sGbps/",t_supportspeeds,t_supportspeed);
            }
            t_supportspeeds[strlen(t_supportspeeds) - 1] = '\0';
            addValue(&fcInfo,supportspeedhead,t_supportspeeds,6);
            //WWPN
            sprintf(filepath,"%s%s/node_name",dirpath,dp->d_name);
            nodename = getFileContent(filepath);
            strcpy(tmpvalue,nodename);
            sscanf(tmpvalue,"0x%s",t_nodename);
            addValue(&fcInfo,nodenamehead,t_nodename,0);
        }
    } 
    (void) closedir(dirp); //关闭目录
    printKeyValue(fcInfo);
}

void getHardwareInfo()
{
    kv * keyValue = NULL;
    int i = 0;
    char key[32] = {0};
    char value[32] = {0};
    for(i = 0;i < 5 ;i++)
    {   sprintf(key,"key%d",i);
        sprintf(value,"value%d",i);
    } 
   for(i = 0;i < 5 ;i++)
    {   sprintf(key,"kkkkkkkkkkkkkkkkkkkkey%d",i);
        sprintf(value,"value%d",i);
    }
    printKeyValue(keyValue);
}

void getSystemInfo()
{
    kv * systemInfo = NULL, *pE = NULL,*kvalue = NULL,*ethtoolInfo = NULL;
    char *hostname = NULL;
    int ishead = 1;
    char ethname[64] = {0};
    char tmpvalue[128] =  {0};
    char ipaddr[64] = {0};
    char macaddr[64] = {0};
    char cpus[8] = {0};
    char cpuname[64] = {0};
    char loginip[32] = {0};
    float intTotalMem = 0;
    int unit = 1;
    char tmpunit[8] = {0};
    char dirpath[64] = {0};
    char filepath[64] = {0};
    char *wwpn;
    DIR *dirp;
    struct dirent *dp;
    char ethtoolcmd[128] = {0};
    char speed[64] = {0};
    hostname = getFileContent("/etc/hostname");
    //printf("%s",hostname);
    addValue(&systemInfo,"服务器名称",hostname,5);
   // printKeyValue(systemInfo);
    //登录用户名

    parseKeyValue(&kvalue,"whoami","%s");
       
    addValue(&systemInfo,"登录用户名",kvalue->key,5);
    free(kvalue);
    kvalue = NULL;
   //以哪个ip登录
    //addValue(&systemInfo,"登录计算机名称","NULL",7);
     //CPU
     //lscpu | grep -E '^Model name|^CPU\(s\)'

    parseKeyValue(&kvalue,"lscpu | grep -E '^Model name|^CPU\\(s\\)'","%[^:]: %[^&]");
    while(kvalue)
    {
         if(0 == strncmp("CPU(s)",kvalue->key,sizeof("CPU(s)")))
         {
             strcpy(tmpvalue,kvalue->value);
             sprintf(cpus,"处理器 1 - %s",tmpvalue);
         }
         else if(0 == strncmp("Model name",kvalue->key,sizeof("Model name")))
         {
             strcpy(cpuname,kvalue->value);
         }
         pE = kvalue;
         kvalue = kvalue -> next;
         free(pE);
    }
    
    addValue(&systemInfo,cpus,cpuname,3);
    //内存
    parseKeyValue(&kvalue,"dmidecode -t memory | grep Size","%[^:]: %[^&]");
    while (kvalue != NULL)
    {
        sscanf(kvalue->value,"%[^A-Z]%s",tmpvalue,tmpunit);
        if(strncmp(tmpunit,"GB",2) == 0)
        {
            unit = 1024;
        }
        else if(strncmp(tmpunit,"MB",2) == 0)
        {
            unit = 1;
        }
        else{unit = 0;};
        intTotalMem += atoi(tmpvalue)*unit;
        pE = kvalue;
        kvalue = kvalue -> next;
        free(pE);
    }
    intTotalMem = intTotalMem / unit;
    sprintf(tmpvalue,"%d%s",(int)intTotalMem,tmpunit);
    addValue(&systemInfo,"内存",tmpvalue,2);

    //ifconfig
    parseKeyValue(&kvalue,"ifconfig -a","%[^=]=%[^&]");
    while(kvalue)
    {
        //头位置
        if(1 == ishead)
        {
            memset(ethname,0,sizeof(ethname));
            sscanf(kvalue->key,"%[^:]:",ethname);
            sprintf(ethtoolcmd,"ethtool %s | grep 'Speed'",ethname);
            parseKeyValue(&ethtoolInfo,ethtoolcmd," %[^:]: %[^&]");
            while(ethtoolInfo)
            {
                 if(0 == strncmp("Speed",ethtoolInfo->key,sizeof("Speed")))
                 {
                     memset(tmpvalue,0,sizeof(tmpvalue));
                     sscanf(ethtoolInfo->value,"%[0-9]",tmpvalue);
                     if(strlen(tmpvalue) > 0)
                         sprintf(speed," - %sMbps ",tmpvalue);
                     else
                         memset(speed,0,sizeof(speed));
                 }
                 pE = ethtoolInfo;
                 ethtoolInfo = ethtoolInfo ->next;
                 free(pE);
            }
            sprintf(ethname,"%s%s",ethname,speed);
            //addValue(&systemInfo,"网络接口",tmpvalue,4);
            ishead = 2;

        }
        else if(2 == ishead)
        {
            //第二行
            memset(ipaddr,0,sizeof(ipaddr));
            sscanf(kvalue->key," %*s %s ",tmpvalue);
            if(strlen(tmpvalue) > 0)
                 sprintf(ipaddr," inet %s",tmpvalue);
            //第三行
            pE = kvalue;
            kvalue = kvalue->next;
            free(pE);
            sscanf(kvalue->key," %s %s ",tmpvalue,macaddr);
            if(0 != strncmp("ether",tmpvalue,sizeof("ether")))
            {
                 pE = kvalue;
                 kvalue = kvalue->next;
                 free(pE);
                 sscanf(kvalue->key," %s %s ",tmpvalue,macaddr);
                 if(0 != strncmp("ether",tmpvalue,sizeof("ether")))
                      memset(macaddr,0,sizeof(macaddr));
            }
            if(strlen(macaddr) > 0)
            {
                 memset(tmpvalue,0,sizeof(tmpvalue));
                 strcpy(tmpvalue,macaddr);
                 sprintf(macaddr," mac %s",tmpvalue);
            }
            if(0 != strncmp("lo",ethname,2))
            {
                sprintf(tmpvalue,"%s%s%s",ethname,ipaddr,macaddr);
                addValue(&systemInfo,"网络接口",tmpvalue,4);
            }
            ishead = 0;
        }
        else if(kvalue->key == NULL || 0 == strlen(kvalue->key))
            ishead = 1;
        pE = kvalue;
        kvalue = kvalue->next;
        free(pE);
    }
  
    addValue(&systemInfo,"管理模式","读/写",4);
    addValue(&systemInfo,"服务器状态","在线",5);

    strcpy(dirpath,"/sys/class/fc_host/");
    //fc
    if(access(dirpath,0))
    {
        printf("directory '%s' is not exist\n",dirpath);
//        return;
    }
    dirp = opendir(dirpath); //打开目录指针
    while ((dp = readdir(dirp)) != NULL) { //通过目录指针读目录
         if(0 != strncmp(".",dp->d_name,sizeof(".")) && 0 != strncmp("..",dp->d_name,sizeof(".")))
         {
            sprintf(filepath,"%s%s/speed",dirpath,dp->d_name);
          /*  speed = getFileContent(filepath);
            strcpy(tmpvalue,speed);
            if(0 == strncmp("unknown",tmpvalue,sizeof("unknown")))
                strcpy(t_speed,"未连接");
            else
            {
                sscanf(tmpvalue,"%[^ ] %1s",t_speed,unit);
                sprintf(t_speed,"%s%sbps",t_speed,unit);
            }
            addValue(&fcInfo,speedhead,t_speed,4);
*/
            //WWPN
            sprintf(filepath,"%s%s/node_name",dirpath,dp->d_name);
            wwpn = getFileContent(filepath);
            strcpy(tmpvalue,wwpn);
            sscanf(tmpvalue,"0x%s",tmpvalue);
            addValue(&systemInfo,"光纤通道WWPN",tmpvalue,4);
         }
     }
    (void) closedir(dirp); //关闭目录

    printKeyValue(systemInfo);
}


void getAboutInfo()
{
    //printf("NetStor VTL Server v8.20 (Build 9310)\n");
    //printf("Copyright (c) 2003-2017 NetStor Software by TOYOU. All Rights Reserved.\n");
    printf("\nNetStor VTL Server v9.00 (Build 9862)\n");
    printf("Copyright (c) 2003-2016 NetStor Software. All Rights Reserved.\n\n");
}  

void printTitle(char *title , int offset)
{
    struct winsize info;
    int winWidth = 0,i = 0;
    ioctl(STDIN_FILENO,TIOCGWINSZ,&info);
    winWidth = (info.ws_col)/3;
    char star = '*';
    for(i = 0;i < winWidth;i++)
    {
      printf("%c",star);
    }
    printf("%s",title);
    for(i = 0;i< winWidth-offset;i++)
    {
      printf("%c",star);
    }
    printf("\n");
}
void getAllInfo()
{
    printTitle("系统信息",8);
    getSystemInfo();
    printTitle("内存信息",8);
    getMemoryInfo();
    printTitle("处理器信息",10);
    getCpuInfo();
    printTitle("网络端口信息",12);
    getIscsiInfo();
    printTitle("光纤通道信息",12);
    getFcInfo();    
}

char *optarg;		// global argument pointer
int optind; 	// global argv index

int getoption(int argc, char *argv[], char *optstring)
{
	static char *next = NULL;
	char c;
	char *cp = NULL;
	if (optind == 0)
		next = NULL;

	optarg = NULL;

	if (next == NULL || *next == '\0')
	{
		if (optind == 0)
			optind++;

		if (optind >= argc || argv[optind][0] != '-' || argv[optind][1] == '\0')
		{
			optarg = NULL;
			if (optind < argc)
				optarg = argv[optind];
			return EOF;
		}

		if (strcmp(argv[optind], "--") == 0)
		{
			optind++;
			optarg = NULL;
			if (optind < argc)
				optarg = argv[optind];
			return EOF;
		}

		next = argv[optind];
		next++;		// skip past -
		optind++;
	}

	c = *next++;
	cp = optstring;

	if (cp == NULL || c == ':')
		return '?';

	cp++;
	if (*cp == (':'))
	{
		if (*next != '\0')
		{
			optarg = next;
			next = NULL;
		}
		else if (optind < argc)
		{
			optarg = argv[optind];
			optind++;
		}
		else
		{
			return '?';
		}
	}

	return c;
}

int parseCommandLine(int argc, char *argv[])
{
    int c;
    optind = 0;

    while ((c = getoption(argc, argv, ("h:c"))) != EOF)
    {
        switch (c)
        {
            case ('c'):
            //case ('C'):
              memset(cmd, 0, 1024);
              strcpy(cmd, optarg);
	            if (optind < argc)
	            {
		            while (optind < argc)
		            {
                  strcat(cmd, " ");
			            strcat(cmd, argv[optind]);
			            optind++;
		            }
                }
                break;

            case ('h'):
            case ('?'):
                printf("Usage:  nscli -c command\n");
                return 0;

            default:
                printf("Not support option %c, please use -h option to get the manual\n", c);
                return 0;
        }
    }

	// all options processed, return success

    return 1;
}

int runOneCmd(const char * cmdBuf)
{
    getAboutInfo();
    if (strncmp(cmdBuf, "vtlshow system", 14) == 0 || strncmp(cmdBuf, "system", 6) == 0)
        getSystemInfo();
    else if (strncmp(cmdBuf, "vtlshow cpu", 11) == 0 || strncmp(cmdBuf, "cpu", 3) == 0)
        getCpuInfo();
   // else if (strncmp(cmdBuf, "vtlshow os", 10) == 0 || strncmp(cmdBuf, "os", 2) == 0)
   //     getOsInfo();
    else if (strncmp(cmdBuf, "vtlshow mem", 11) == 0 || strncmp(cmdBuf, "mem", 3) == 0)
        getMemoryInfo();
    else if (strncmp(cmdBuf, "vtlshow iscsiport", 17) == 0 || strncmp(cmdBuf, "iscsiport", 9) == 0)
        getIscsiInfo();
    else if (strncmp(cmdBuf, "vtlshow fcport", 14) == 0 || strncmp(cmdBuf, "fcport", 6) == 0)
        getFcInfo();
    else if (strncmp(cmdBuf, "vtlshow all", 11) == 0 || strncmp(cmdBuf, "all", 3) == 0) 
        getAllInfo();
    else if (strncmp(cmdBuf, "vtlshow about", 13) == 0 ||  strncmp(cmdBuf, "about", 5) == 0){}
 //       getAboutInfo();
    else
    {
          printf("vtlshow system\n");
          printf("vtlshow mem\n");
          printf("vtlshow cpu\n");
          printf("vtlshow iscsiport\n");
          printf("vtlshow fcport\n");
          printf("vtlshow all\n");
          printf("vtlabout\n");

    }
        //printf("Not support command %s.\n", cmdBuf);

    return 0;
}

int isAllSpaces(char *buffer)
{
    int i;
    int len = strlen(buffer);

    for (i = 0; i < len - 1; i++)
    {
        if (buffer[i] != 0x20)
        {
            return 0;
        }
    }

    return 1;
}

void runLoopCmd(int argc, char **argv)
{
    int lineno = 0, i = 0;
    int ret = 0, firstPos = -1, secPos = -1;
    char key[16];

    for (;;) {
        char buffer[256];
        memset(buffer, 0, 256);
        printf ("nscli> ");
        fflush (stdout);
        ++lineno;
        if (fgets (buffer, sizeof(buffer), stdin) == NULL) {
            fputs("\n", stdout);
            return;
        }

        if (!buffer || (isAllSpaces(buffer)) ||
            (buffer[0] == 0xa))
            continue;
        else if (strlen(buffer) < 3)
            printf("Invalid command\n");
        else {
            if (strncmp(buffer, "exit", 4) == 0 ||
                strncmp(buffer, "quit", 4) == 0 ) {
                break;
            } else if ( strncmp(buffer, "help", 4) == 0 ) {
                //TODO: need store this command to the database;
                printf("vtlshow system\n");
                printf("vtlshow mem\n");
                printf("vtlshow cpu\n");
                printf("vtlshow iscsiport\n");
                printf("vtlshow fcport\n");
                printf("vtlshow all\n");
                printf("vtlshow about\n");
                continue;
            }
            else if (strncmp(buffer, "about", 5) == 0) {
                printf("NetStor VTL Server v8.20 (Build 9310)\n");
                printf("Copyright (c) 2003-2017 NetStor Software by TOYOU. All Rights Reserved.\n");
                continue;
            }

            runOneCmd(buffer);
        }
    }

    return;
}

int main(int argc, char **argv)
{
    char *pCh = NULL;
    int retOk = 0, i = 0, pos = 0;
    if(2 == argc)
    {
        return runOneCmd(argv[1]);
    }
    retOk = parseCommandLine(argc, argv);
    if (retOk)
    {
        if (argc == 1)
        {
            interactive = 0;
        }
    }
    else
        return 1;

    if (!interactive)
    {
        if (strlen(cmd) != 0)
        {
           return runOneCmd(cmd);
        }
    }
    runLoopCmd(argc, argv);

    return 0;
}
