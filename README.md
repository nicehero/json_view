# json_view

#### 介绍
* header-only c++ json库，致力于最好的平均性能，将c++开发变为Schemaless高效率开发的利器
* 支持bson格式
* 当使用parse对json/bson数据生成json_view时，只是数据生成一个view，减少内存拷贝，提高性能
* 当使用json的parse默认使用lazy模式，惰性求值
* 需要c++17以上版本

#### 使用说明


```
#include "json_view.hpp"
void func(){

using namespace nicehero;
json_view j;
j.parse(R"({"hello":"world"})");//parse
auto& w = j["hello"];//return hello ref
auto wb = w.is_string();
auto sw = w.as_string();//return string if json_view is a string type
auto it1 = j.find("world");//find world field iterator
if (it1 != j.end()) //iterator exist
{
    auto k = it1.key();//return key hello
    auto& v = it1.val();//return json_view value string hello
}
for (auto it: j)//traversal json_view
{
    auto k = it.key();//return key hello
    auto& v = it.val();//return json_view value string hello
}
std::string json = j.dump();//stringify json
std::string json2 = j.dump(1);//prettify json
std::vector<uint8_t> bson = j.dump_bson();//dump to bson
json_view j2;
j2.parse_bson(bson);//parse bson to json_view

}
```


