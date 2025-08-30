#include "logger.h"

#include <stdio.h>

//#ifdef _LINUX_
#include <unistd.h>
#include <sys/syscall.h>
//#include <sys/stat.h>
//#else define _WIN32

//#endif
//#include "config.h"

zlog_category_t* _dev_category = NULL;


static const char* _LOG_CONFIG_FILE = "/home/ethan/dvp/sdpf/log_config.conf";
//static const char* _LOG_CONFIG_FILE = "/mnt/app/config/log_config.conf";
//static const char* _Content_Config_File =
//"[global]\n"
//"strict init = true\n"
//"buffer min = 1024\n"
//"buffer max = 8192\n"
//"[levels]\n"
//"TRACE = 10\n"
//"[formats]\n"
//"normal = \"%d(%m%d %H%M%S).%ms %v %p %m [%F:%L]%n\"\n"
//"simple = \"%d(%d %H%M%S).%ms %v %m%n\"\n"
//"[rules]\n"
//"dev_cat.trace     \"/home/ethan/projects/utest/log7.log\", 256K * 2; normal\n"

//static const char* _LOG_PATH = "/data/dvr/log/";
static const char* _LOG_CATEGORY_DEFAULT = "dev_cat";


int log_get_tid() {
//#ifdef _LINUX_
    return (int)syscall(SYS_gettid);
//#else

//#endif
}


int log_init(const char* cat) {
    if (_dev_category != NULL) {
        return 0;
    }
    if(cat != NULL)
    {
        _LOG_CATEGORY_DEFAULT = cat;
    }
    int rc = dzlog_init(_LOG_CONFIG_FILE, _LOG_CATEGORY_DEFAULT);
    if (0 != rc) {
        printf("log init failed:%d %s\n", rc,_LOG_CONFIG_FILE);
        return -1;
    }

    if (0 != setvbuf(stdout, NULL, _IOLBF, 0)) {
        printf("log init set buffer failed\n");
    }
//    if (0 != mkdir(_LOG_PATH, 0777)) {
//        printf("log init mkdir failed\n");
//    }

    _dev_category = zlog_get_category(_LOG_CATEGORY_DEFAULT);
    if (_dev_category == NULL) {
        printf("log get category failed!\n");
        zlog_fini();
        return -2;
    }

    printf("log init success, config file: %s\n", _LOG_CONFIG_FILE);
    return 0;
}

int log_uninit() {
    zlog_fini();
    _dev_category = NULL;
    return 0;
}




