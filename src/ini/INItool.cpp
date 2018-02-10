#include "INItool.h"
#include <fstream>
#include <sstream>
#include <vector>
using namespace std;
using namespace ini;

char* LTrim(char * buff)
{
    while (*buff == '\t' || *buff == ' ')++buff;
    return buff;
}
void RTrim(char * buff)
{
    while (*buff != 0)++buff; --buff;
    while (*buff == '\t' || *buff == ' ')--buff;
    *(buff + 1) = 0;
}
char* trim(char * buff){ RTrim(buff); return LTrim(buff); }

#define buffmax 8096
void INItool::解析(istream&in)
{
    char buff[buffmax + 1];
    Comments commentses;
    Sections*nowsections = nullptr;
    while (!in.eof())
    {
        in.getline(buff, buffmax);
        buff[buffmax] = 0;//防止异常..
        auto start = LTrim(buff);//删除前导空白
        switch (*start)
        {
            //注释
        case ';':
        case '#':
            commentses.push_back(start);
            break;
            //段
        case '[':
        {
                    auto end = start;
                    while (*end != 0)++end;//到末尾
                    ++start;
                    while (*end != ']'&&end != start)--end;//倒找文本
                    if (end == start)break;//无效段名
                    *end = 0;//取文本中间
                    if (has(start))
                    {
                        nowsections = &get(start);
                    }
                    else
                    {
                        sectionses.emplace_back(start, commentses);//新建段
                        commentses.clear();
                        nowsections = &sectionses.back();
                    }
                    break;
        }
        default:
            //过滤无效行
            auto str = start;
            while (*start != '=' && *start != 0)
                ++start;
            if (start == str //空key
                //|| *(tep + 1)==0//空值,但是这是允许的,所以注释掉
                || *start == 0 //空行
                )break;
            //节
            *start = 0; ++start;
            if (nowsections == nullptr)break;
            nowsections->push_back({ trim(str), start, commentses });
            commentses.clear();
            break;
        }
    }
}

INItool::INItool(string filename)
:filename(filename)
{
    ifstream in(filename);
    if (!in.is_open())
        return;

    解析(in);
}

void INItool::load(string string)
{
    解析(stringstream(string));
}

bool INItool::has(std::string name)
{
    for (auto&s : sectionses)
    if (s.name == name)
        return true;
    return false;
}

Sections&INItool::operator[](string sectionsName)
{
    return get(sectionsName);
}

Sections& INItool::get(std::string sectionsName)
{
    for (auto&s : sectionses)
    if (s.name == sectionsName)
        return s;
    sectionses.emplace_back(sectionsName);
    return sectionses.back();
}

ostream&ini::operator<<(ostream& out, INItool&t)
{
    for (auto&y : t.sectionses)
        out << y;
    return out;
}



Parameters::Parameters(string key, string val, Comments commentses)
:key(key), val(val), commentses(commentses), noshow(false)
{}

Parameters::Parameters(std::string key)
: key(key), noshow(true)
{}

Parameters& Parameters::operator=(std::string val)
{
    this->val = val;
    return *this;
}

ostream& ini::operator<<(ostream& out, Comments&t)
{
    for (auto&y : t)
    {
        auto c = LTrim(const_cast<char*>(y.c_str()));
        if (*c != ';' && *c != '#')
            out << "; ";
        out << c << endl;
    }
    return out;
}
ostream& ini::operator<<(ostream& out, Parameters&t)
{
    out << t.commentses
        << t.key.c_str() << "=" << t.val.c_str()
        << endl;
    return out;
}

Parameters& ini::operator>>(Parameters& t, std::string s)
{
    s = t.val;
    return t;
}

Sections::Sections(std::string name) 
:name(name)
{}

Sections::Sections(string name, Comments commentses)
:name(name), commentses(commentses)
{}

Parameters& Sections::operator[](std::string parametersName)
{
    return get(parametersName, 0);
}

Parameters& Sections::get(std::string parametersName, int index)
{
    int thisIndex = 0; Parameters *tep;
    for (auto&v : *this)
    if (v.key == parametersName)
    {
        if (thisIndex == index) return v;//找到指定序列的键,返回去
        else { ++thisIndex; tep = &v; }
    }
    if (thisIndex == 0)
    {
        this->emplace_back(parametersName);
        return this->back();
    }
    else return*tep;
}

ostream& ini::operator<<(ostream& out, Sections&t)
{
    if (t.size())
    {
        out << t.commentses
            << "[" << t.name.c_str() << "]" << endl;
        for (auto&p : t)
            out << p;
    }
    return out;
}
