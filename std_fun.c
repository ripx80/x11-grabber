#include <stdio.h>
#include <stdarg.h>
#include <unistd.h>
#include <stdlib.h>

char DEBUG = 1;
va_list args;

void std_msg(const char lvl,const char* format,va_list args){    
    switch(lvl){
        case 1:
            fprintf( stderr, "[*] " );
        break;
        case 2:
            fprintf( stderr, "[!] " );
        break;
        case 3:
            fprintf( stderr, "[ERROR] " );
        break;
        default:
            printf("[!] Unkown Message Level");
        break;        
    }
    vfprintf( stderr, format, args );
    fprintf( stderr, "\n" );
}

void debug(const char* format,...){    
    if(DEBUG){
        va_start( args, format );
        std_msg(1,format,args);
        va_end( args );
    }
}

void fatal(const char* format,...){    
    va_start( args, format );
    std_msg(3,format,args);
    va_end( args );
    exit(1);    
}

void warn(const char* format,...){ 
    va_list args;   
    va_start( args, format );
    std_msg(2,format,args);    
    va_end( args );
}

uint8_t s2ui8(char *buf){
    if(atoi(optarg) < 254){
        return (uint8_t) atoi(optarg);
    }else{
        printf("[ Error ] This number is too long. maxium 254\n");
        exit(1);
    }
}

int s2i(char *buf){
    if(atoi(buf) < 4294967294){
        return (int) atoi(buf);
    }else{
        printf("[ Error ] This number is too long.\n");
        exit(1);
    }    
}

int exec_cmd(char *cmd, char *cmd_args[]){
    return execv(cmd, cmd_args);
}
