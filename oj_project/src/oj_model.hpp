#pragma once
#include <iostream>
#include <unordered_map>
#include <string>
#include <algorithm>
#include <fstream>

#include "tools.hpp"
#include "oj_log.hpp"

//试题id 试题名称 试题路径 试题难度
typedef struct Question
{
    std::string id_;
    std::string name_;
    std::string path_;
    std::string star_;
}QUES;

class OjModel
{
    public:
        OjModel()
        {
            LoadQuestions("./config_oj.cfg");
        }
        bool GetAllQuestions(std::vector<Question>* ques)
        {
            for(const auto& kv:model_map_)
            {
                ques->push_back(kv.second);
            }
            
            //针对内置类型进行操作
            //std::greater降序排序 std::less 升序排序
            std::sort(ques->begin(),ques->end(),[](const Question& l,const Question& r){
                    return std::atoi(l.id_.c_str()) <std::atoi(r.id_.c_str());
                    });

        return true;
        }
         bool GetOneQuestion(const std::string& id,std::string* desc,std::string* header,Question* ques)
        {
            //1.根据id去查找对应题目信息，最重要的就是这个题目在哪里
            auto iter=model_map_.find(id);
            if(iter==model_map_.end())
            {
                LOG(ERROR,"Not Found Question id is")<<id<<std::endl;
                return false;
            }

            //iter->second.path_;+文件名称（desc.txtheader.cpp)
            *ques=iter->second;
            //加载具体的单个题目信息，从保存的路径之中加载
            //从具体的题目文件当中去获取两部分信息，描述，header头
            
            std::string str1=DescPath(iter->second.path_);
            
            int ret=FileOper::ReadDataFromFile(str1,desc);
            if(ret==-1)
            {
                LOG(ERROR,"Read desc failed")<<std::endl;
                return false;
            }


            str1=HeaderPath(iter->second.path_);
            ret=FileOper::ReadDataFromFile(str1,header); 
            if(ret==-1)
            {
                LOG(ERROR,"Read desc failed")<<std::endl;
                return false;
            }
            return true;
        }

         bool SplicingCode(std::string user_code,const std::string& ques_id,std::string* code)
         {
             //1.查找下对应id的题目是否存在
             auto iter=model_map_.find(ques_id);
             if(iter==model_map_.end())
             {
                 LOG(ERROR,"can not find question id is")<<ques_id<<std::endl;
                 return false;
             }

             std::string tail_code;
             std::string str=TailPath(iter->second.path_);
             int ret=FileOper::ReadDataFromFile(str,&tail_code);
             if(ret<0)
             {
                 LOG(ERROR,"Open tail.cpp falied");
                 return false;
             }
             *code=user_code+tail_code;
             return true;

         }

    private:
        bool LoadQuestions(const std::string& configfile_path)
        {
            //使用C++文件中的文件流来加载文件，并获取文件当中的内容 
            //iostream处理控制台IO fstream处理命名文件的 stringstream完成内存中对象的IO
            //ofstream:output文件流，文件当中写
            //ifstream:input文件流，从文件当中读
            //在定义istream对象的时候，就可以指定打开哪一个文件，和文件进行绑定
            //ifstream(const char* filename,ios_base::openmode mode=ios_base::in);//in 以读的方式打开 out:以写的方式打开
            //void open(const char*filename,ios_base::penmode mode=ios_base::in);
            
            std::ifstream file(configfile_path.c_str());
            if(!file.is_open())
            {
                return false;
            }

            std::string line;
            while(std::getline(file,line))
            {
                //1 单链表 ./xxx  难度
                //1.需要切割字符串
                std::vector<std::string> vec;
                
                StringTools::Split(line," ",&vec);
                if(vec.size()!=4)
                {
                    continue;
                }
                //2.切割后的内容保存到unordered_map
                Question ques;
                ques.id_=vec[0];
                ques.name_=vec[1];
                ques.path_=vec[2];
                ques.star_=vec[3];
                model_map_[ques.id_]=ques;
            }
            file.close();
            return true;

        }

    private:
        std::string DescPath(const std::string& ques_path)
        {
            return ques_path+"desc.txt";
        }
        std::string HeaderPath(const std::string& ques_path)
        {
            return ques_path+"header.cpp";
        }
        std::string TailPath(const std::string& ques_path)
        {
            return ques_path+"tail.cpp";
        }


    private:
        //map<key(id),value(TestQues)> model_map;红黑树 有序的树形结构，查询的效率不高
        //unordered_map<key,value>   //哈希表-无序-查询效率高，基本上就是常数时间完成的
        std::unordered_map<std::string ,Question> model_map_;

};
