#pragma once
#include <fcntl.h>
#include <unistd.h>
#include <sys/wait.h>
#include <sys/stat.h>
#include <sys/resource.h>
#include <string>
#include <iostream>
#include <json/json.h>

#include "oj_log.hpp"
#include "tools.hpp"

enum ErrorNo
{
    OK=0,
    COMPILE_ERROR,
    RUN_ERROR,
    PRAM_ERROR,
    INTERNAL_ERROR
};

class Compiler
{
    public:
        //有可能浏览器对不同的题目提交的数据是不同的
        //code="xxx"
        //code="xxx"&stdin="xxx" code+tail.cpp
        static void CompilerAndRun(Json::Value Req,Json::Value* Resp)
        {
            //0:编译运行没有错误
            //1：编译错误 2:运行错误 3.参数错误 4.内存错误
            //1.判空
            //{"code":"xxx","stdin":"xxx"}
            if(Req["code"].empty())
            {
                (*Resp)["errorno"]=PRAM_ERROR;
                (*Resp)["reason"]="Pram error";
                LOG(ERROR,"Request code is Empty")<<std::endl;
                return;
            }
            //2.将代码写到文件当中去
            std::string code=Req["code"].asString();
            //文件名称进行约定 tmp_时间戳.cpp
            std::string tmp_filename=WriteTmpFile(code);
            if(tmp_filename=="")
            {
                (*Resp)["errorno"]=INTERNAL_ERROR;
                (*Resp)["reason"]="Create file failed";
                LOG(ERROR,"Write Source failed");
                return;
            }
           //3.编译
           if(!Compile(tmp_filename))
           {
               (*Resp)["errorno"]=COMPILE_ERROR;
               std::string reason;
               std::string str=ErrorPath(tmp_filename);
               FileOper::ReadDataFromFile(str,&reason);
               (*Resp)["reason"]=reason;
               LOG(ERROR,"Cmpile Error")<<std::endl;
               return;

           }
           //4.运行
           int sig=Run(tmp_filename);
           if(sig!=0)
           {
               (*Resp)["errorno"]=RUN_ERROR;
               (*Resp)["reason"]="Program exit by sig"+std::to_string(sig);
               LOG(ERROR,"Run Error")<<std::endl;
               return;

           }
           //5.构造响应
           (*Resp)["errorno"]=OK;
           (*Resp)["reason"]="Cpmpile and run is okey!";
           //标准输出
           std::string stdout_reason;
           std::string str_stdout=StdoutPath(tmp_filename);
           FileOper::ReadDataFromFile(str_stdout,&stdout_reason);
           (*Resp)["stdout"]=stdout_reason;
           //标准错误
           std::string stderr_reason;
           std::string str_stderr=StderrPath(tmp_filename);
           FileOper::ReadDataFromFile(str_stderr,&stderr_reason);
           (*Resp)["stderror"]=stderr_reason;

           //6.清理掉临时文件
           Clean(tmp_filename);

           return;

        }
    private:
        static std::string WriteTmpFile(const std::string& code)
        {
            //1.组织文件名称，组织文件的前缀名称，用来区分源文件，可执行文件是同一组数据
                std::string tmp_filename="/tmp_"+std::to_string(LogTime::GetTimeStamp());
            //写文件
                int ret=FileOper::WriteDataToFile(SrcPath(tmp_filename),code);
                if(ret<0)
                {
                    LOG(ERROR,"Write code to source failed")<<std::endl;
                    return 0;
                }
            return tmp_filename;
        }

        static std::string SrcPath(const std::string& filename)
        {
            return "./tmp_files"+filename+".cpp";
        }
        static std::string ErrorPath(const std::string& filename)
        {
            return "./tmp_files"+filename+".err";
        }
        static std::string ExePath(const std::string& filename)
        {
            return "./tmp_files"+filename+".executable";
        }
        static std::string StdoutPath(const std::string& filename)
        {
            return "./tmp_files"+filename+".stdout";
        }
        static std::string StderrPath(const std::string& filename)
        {
            return "./tmp_files"+filename+".stderr";
        }

        static bool Compile(const std::string& filename)
        {
            //1.构造编译命令-->g++ src -o [exec] -std=c++11
            const int commandCount=20;
            char buf[commandCount][50]={{0}};
            char* Command[commandCount]={0};
            for(int i=0;i<commandCount;i++)
            {
                Command[i]=buf[i];
            }
            snprintf(Command[0],49,"%s","g++");
            snprintf(Command[1],49,"%s",SrcPath(filename).c_str());
            snprintf(Command[2],49,"%s","-o");
            snprintf(Command[3],49,"%s",ExePath(filename).c_str());
            snprintf(Command[4],49,"%s","-std=c++11");
            snprintf(Command[5],49,"%s","-D");
            snprintf(Command[6],49,"%s","CompileOnline");
            Command[7]=NULL;
            //2.创建一个子进程
            //2.1父进程-->等待子进程推出
            //2.2子进程-->进程程序替换-->g++
            int pid=fork();
            if(pid<0)
            {

                LOG(ERROR,"Create child fork failed");
                return false;
            }
            else if(pid==0)
            {
                //child
                int fd=open(ErrorPath(filename).c_str(),O_CREAT|O_RDWR,0664);
                if(fd<0)
                {
                    LOG(ERROR,"open compile erroefile failed")<<ErrorPath(filename)<<std::endl;
                    exit(1);
                }

                //重定向
                dup2(fd,2);
                //程序替换
                execvp(Command[0],Command);
                perror("execvp");
                LOG(ERROR,"execvp failed")<<std::endl;
                exit(0);
            }
            else
            {
                //father
                waitpid(pid,NULL,0);
            }
            //3.验证是否生成可执行程序
            struct stat st;
            int ret=stat(ExePath(filename).c_str(),&st);
            if(ret<0)
            {
                LOG(ERROR,"Compile ERROR!Exe filename is")<<ExePath(filename)<<std::endl;
                return false;
            }
            return true;

        }

        static int Run(const std::string& filename)
        {
            //可执行程序
            //1.创建子进程
            //  父进程---》进程等待
            //  子进程---》替换编译出来的程序

            int pid=fork();
            if(pid<0)
            {
                LOG(ERROR,"Exec pragma failed! Create child failed")<<std::endl;
                return -1;
            }
            else if(pid==0)
            {
                //对于子进程执行的限制
                //1.时间限制
                // alarm
                alarm(1);
                //2.内存大小限制 #include<sys/resource.h>
                //int setrlimit(int type,struct rlimit* rlim)
                //type:RLIMIT_AS:进程最大的虚拟内存空间，限制进程内存大小
                //RLIMIT_DATA:进程数剧段的最大值 RLIMIT_STACK:最大的线程栈，以字节为单位
                //struct rlimit:
                //   rlim_t rlim_cur:软限制--》内核强加给进程响应资源的限制值 rlimi_t rlim_max：硬限制--》操作系统所能提供的最大值--》PLIM_INFINITY(无限制)
                struct rlimit r1;
                r1.rlim_cur=1024*30000;
                r1.rlim_max=RLIM_INFINITY;//无限制
                setrlimit(RLIMIT_AS,&r1);
                //child
                // 获取：标准输出--重定向到文件
                int stdout_fd=open(StdoutPath(filename).c_str(),O_CREAT|O_RDWR,0664);
                if(stdout_fd<0)
                {
                    LOG(ERROR,"Open stdout file failed")<<StdoutPath(filename)<<std::endl;
                    return -1;
                }
                dup2(stdout_fd,1);
                //            标准错误--重定向到文件
                int stderr_fd=open(StderrPath(filename).c_str(),O_CREAT|O_RDWR,0664);
                if(stderr_fd<0)
                {
                    LOG(ERROR,"Open stderr file failed")<<StderrPath(filename)<<std::endl;
                    return -1;
                }
                dup2(stdout_fd,2);

                execl(ExePath(filename).c_str(),ExePath(filename).c_str(),NULL);
                exit(1);
            }

            //father
            int Status=-1;
            waitpid(pid,&Status,0);
            //将是否收到的信息返回给调用者，如果调用者判断是0，则正常运行完毕，否则收到某个信号异常结束
            return Status & 0x7f;
        }

        static void Clean(std::string filename)
        {
            unlink(SrcPath(filename).c_str());
            unlink(ExePath(filename).c_str());
            unlink(ErrorPath(filename).c_str());
            unlink(StdoutPath(filename).c_str());
            unlink(StderrPath(filename).c_str());
        }
};
