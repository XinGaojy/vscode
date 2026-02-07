
前置知识文档:
*************************************************************************************
(id=1)时间线1
cpu host=10.0.0.1 domain=beijing zone=x timestamp=1 0.5

cpu hostid=10.0.0.1 domain=beijing zone=x timestamp=2 0.5

cpu host=10.0.0.1 domain=beijing zone=x timestamp=3 0.5

(id=2)时间线2
mem host=10.0.0.1 domain=beijing zone=y timestamp=2 0.6

(id=3)时间线3
mem host=10.0.0.2 domain=shanghai zone=y timestamp=3 0.7

20s  -> table 20s --->{1,3,5}   60*60/20==60*3 =180

1min  -> table 1min --->{3}   60*60/60==60*1 =60

10min  -> table 10min --->{}   60*60/10/60==6

将线文件和具体的点文件分开存储,主要是线文件会有很多相同的serieskey,可以减少存储成本,将其放入serieskey文件中

所以具体的存储方式是通过线文件+点文件的方式

将cpu host=10.0.0.1 domain 写入serieskey(metric+tag)文件

下面是点文件(存储三个点)
1 0.5
2 0.6
3 0.7
domain=beijing,host=10.0.0.1------>{tagkey,tagvalue}
一个时间线分成多个tagtoken----->tagtoken={host=10.0.0.1}   {domain=beijing}    {zone=x}

cpu host=10.0.0.1 domian {metric+tag} 写入线serieskey文件

token1=hash{host:10.0.0.1}
token2=hash{host:10.0.0.2}
token3=hash{domain:beijing}
token4=hash{domain:shanghai}
token5=hash{zone:x}
token6=hash{zone:y}

在内存中记录token并分词构建倒排链
token1--->{1,2}
token2--->{3}
token3--->{1,2}
token4--->{3}
token5--->{1}
token6--->{2,3}

写入到倒排索引文件中:词典,倒排posting列表
[token1-hash,offset1]
[token2-hash,offset2]
[token3-hash,offset3]
[token4-hash,offset4]
[token5-hash,offset5]
[token6-hash,offset6]

根据上面的token-hash找到对应的offset偏移,然后通过定位到具体的行,然后就是orc的读写流程


正排索引
还是上面的三个点,记录每个点的时间写入范围
[1,3] --->写入三个点
[2,2] --->写入一个点
[3,3] --->写入一个点
如果用户需要查询的时候,定位到某个时间线之后,把所有数据捞到内存中,然后内存再根据需要的时间范围进行过滤













2.具体实现
***********************************************************************************
1.1从本地文件中读取时间线数据(serieskey+point)
min_value,max_value,avg_value,sum_value,count_value;
    (1)创建数据文件
        对于一行数据有下面这些信息:
        metric tagkey1=tagvalue1,tagkey2=tagvalue2,tagkey3=tagvalue3,timestamp,min_value,max_value,avg_value,sum_value,count_value;
    struct point{
        string metric;
        vector<string>tagkeyvalue;
        double timestamp;
        vector<string>fieidname;
        vector<double>fieidvalue;
    };





1.
cpu host=10.0.0.1 domain=beijing zone=x timestamp=1769866020 1 2 3 4 5

cpu host=10.0.0.1 domain=beijing zone=x timestamp=1769866021 1 2 3 4 5

cpu host=10.0.0.1 domain=beijing zone=x timestamp=1769866022 1 2 3 4 5

cpu host=10.0.0.1 domain=beijing zone=x timestamp=1769866023 1 2 3 4 5

cpu host=10.0.0.1 domain=beijing zone=x timestamp=1769866024 1 2 3 4 5

2.读取数据源内存中build 每一行数据可以称为一篇消息
需要哪些数据结构
    1.整理serieskey=metric + tag
    2.点数据的buffer
    3.分词拆token--->计算hash值
      倒排链:一个token有哪些线有
    4.每一条线写入的timerange
3.dump落盘:
    时间线排序 seriesid
    写serieskey文件(orc)
    写点文件(orc)
    正排索引:写每条线在点文件中的行号:rowrange<0,9> <10,12>
    倒排索引:词典+倒排链
4.查询链路:
    1.实现request接口:
        struct request{
            metric;
            tagkey1=tagvalue1;
            tagkey2=tagvalue2;
            timerange;
            vector<string>searchfieidname;
        };

        struct response{
            string serieskey;
            vector<double>points;
        };

    2.读磁盘索引文件,根据倒排链求交得到seriesid,确定查询哪些线 
      根据线id--->serieskey和rowrange------->point orc点文件(调用orc接口)(读取用户需要的列的数据)






3.merge的链路
************************************************************************************