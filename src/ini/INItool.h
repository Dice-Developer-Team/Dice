#pragma once
#include <string>
#include <vector>

namespace ini{
    class Sections;
    class INItool;

    //注释集合
    class Comments :public std::vector<std::string>
    {
        friend std::ostream & operator<<(std::ostream &out, Comments &t);
    };
    std::ostream & operator<< (std::ostream &out, Comments &t);

    //键值对
    class Parameters
    {
        std::string key; std::string val; Comments commentses; bool noshow;

        friend std::ostream & operator<<(std::ostream &out, Parameters &t);
        friend Parameters & operator>>(Parameters &t, std::string s);
        friend Sections;

    public:
        Parameters(std::string key, std::string val, Comments commentses);
        Parameters(std::string key);

        Parameters&operator=(std::string val);
        operator std::string() const
        {
            return val;
        }
    };
    std::ostream & operator<<(std::ostream &out, Parameters &t);
    Parameters & operator>>(Parameters &t, std::string s);

    //键值对集合(又名 段)
    class Sections :
//        public //不想公开...
        std::vector<Parameters>
    {
        std::string name;
        Comments commentses;
        friend std::ostream & operator<<(std::ostream &out, Sections &t);
        friend INItool;

    public:
        Sections() = default;
        Sections(std::string name);
        Sections(std::string name, Comments commentses);

        //std::string getName()const{ return name; }

        Parameters& operator[](std::string parametersName);
        Parameters& get(std::string parametersName, int index);


    };
    std::ostream & operator<<(std::ostream &out, Sections &t);

    //代表一个ini
    class INItool
    {
        std::string filename;
        std::vector<Sections> sectionses;
        void 解析(std::istream& in);

        friend std::ostream & operator<<(std::ostream &out, INItool &t);

    public:
        INItool(std::string filename);
        INItool() = default;
        ~INItool() = default;

        //解析文本
        void load(std::string string);

        //判断
        bool has(std::string sectionsName);

        //取
        Sections& operator[](std::string sectionsName);
        Sections& get(std::string sectionsName);

        //删
        void del(std::string sectionsName);
    };
    std::ostream & operator<<(std::ostream &out, INItool &t);
}
