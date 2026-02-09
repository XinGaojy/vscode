# Khronos: 面向万亿规模时间线的性能监控引擎建设实践

![图片](https://mmbiz.qpic.cn/mmbiz_png/OmCbZ5JK30HUkAbjLFdSk9GC5KHQhEXUia1Oibkezw1K9hx4OEWZCAsM67sHkian0icrEiavhnAxibnyxfpicGmmmasTA/640?wx_fmt=png&tp=webp&wxfrom=5&wx_lazy=1#imgIndex=0)

这是2023年的第76篇文章

（ 本文阅读时间：15分钟 ）

阿里巴巴智能引擎事业部自研的 Khronos 系统是阿里内部接入规模最大的性能数据存储引擎。Khronos 支持动态生命周期的存储计算分离架构，采用 schemaless 的 data model 设计，在万亿数据规模下为业务提供易用、高效、经济的服务，团队近期的优化工作也被国际学术会议CIKM2023收录。本⽂总结了Khronos 在性能监控领域遇到的技术挑战，以及在这个场景下的一些价值判断。

**01**

**背景**

时序数据管理系统（Time series DBMS）近年来受到较多的关注，这是受到多方面因素推动的结果，包括：云原生可观测性的要求逐渐标准化，DevOps/AIOps 的发展，IoT 技术、车联网、智能⾦融等技术趋势对时序数据的存储需求。从图1左侧 DB-Engines 网站[1]的趋势可以看出，从2013年开始，Time series DBMS 受到的关注就在逐渐上升。

另一方面，时序数据库产品形态也呈现出多元化趋势。图中右侧列出的是 DB-Engines 根据时序数据库产品热度的 top10 排名。在这个榜单中，有的是为业务场景设计的专业时序数据库产品（例如 InfluxDB、Prometheus、Graphite），有的是基于关系型数据库的架构，针对业务场景进行了专门的设计（例如 Kdb+，TimescaleDB), 也有的面向更通用的大数据分析场景，时序分析是它能满⾜的⼀个子场景（例如 Druid）。

![图片](https://mmbiz.qpic.cn/mmbiz_png/OmCbZ5JK30HUkAbjLFdSk9GC5KHQhEXUCZD3fB2SdN6S8Rzwlriad7traJFtC7kBuKiaXOw237rSmyOP8WLTsjVQ/640?wx_fmt=png&tp=webp&wxfrom=5&wx_lazy=1#imgIndex=1)

在我们对业内产品的调研过程中发现，⽬前没有产品能够很好的满⾜大规模性能监控中台对时序数据库的要求。因此在2019年，阿里智能引擎团队就基于 AIOS 技术栈体系[2]和 Havenask 开源搜索引擎[3]，自研了⼀款面向大规模性能监控场景的时序存储引擎 Khronos。

经过几年的持续建设，它已经成为阿里内部两大性能监控平台 Kmonitor 和 Sunfire 的底层时序数据存储引擎。2023年是 Khronos 上线生产系统的第4年，从数据接入量上看，它俨然已成长为阿里巴巴内部规模最大的性能数据存储引擎。

**02**

**技术挑战**

在性能监控场景，TSDB 的主要的使用场景有大盘展示、系统问题调查、根因分析、异常检测和报警等。随着内部业务逐渐向云上环境迁移，基于云上的性能监控，面临以下几大挑战：

2.1 写入规模巨大

随着 DevOps 概念、云原生概念、系统可观测性概念的普及，集团内部应用大量使用指标、日志等手段实时反馈系统的性能状态和业务状态。

以 Kmonitor 业务平台为例，指标接入量从2019年的每秒写入46Million/second 增长到2022年的255 Million/second，每年都有1倍左右的写入量增长。这些数据需要被 Khronos 实时消费、索引并且存储起来，这对数据的接入pipeline 是⼀个巨大的压力。另⼀方面，业务层面对指标数据的保留时间限制(time-to-live) 存在需求，大部分的指标数据保留1-3个月，但是也有⼀定比例的指标要求永久保留。

2.2 维度诅咒

我们用时间线的基数（cardinality）来衡量性能监控场景的规模。这里先简单介绍⼀下时间线的概念。性能监控数据通常被建模为多维时间序列（multi-dimensional time series）,  每⼀个 time series 包含⼀个 metric、⼀组 tags（其中，每个 tag 由 tag key 和 tag value构成）和⼀组带时间戳的样本值（timestamped samples）。⼀条时间线可以由 SeriesKey 进行唯⼀标识，SeriesKey =  metric + tags。

以下表为例，包含了4个series（红、绿、蓝、黄）：

![图片](https://mmbiz.qpic.cn/mmbiz_png/OmCbZ5JK30HUkAbjLFdSk9GC5KHQhEXUFwlReNENWf4cmbgAX7FYlVrs7K4LARuRJC8V5r0hWsDPFUYXlNlvfQ/640?wx_fmt=png&tp=webp&wxfrom=5&wx_lazy=1#imgIndex=2)

（表1）

在Kmonitor业务场景中，我们在多个租户都观察到时间线的基数逐渐膨胀。图2给出了4个典型租户时间线基数的变化趋势。可以看出，他们的时间线规模都在持续增长，个别租户(HI) 的时间线规模甚至超过万亿级别（1e12）。另一个值得注意的统计特征是60%以上的时间线生命周期并不长，在⼀个小时以内。

![图片](https://mmbiz.qpic.cn/mmbiz_png/OmCbZ5JK30HUkAbjLFdSk9GC5KHQhEXU0HbqqOXGbnjv60kzniaicEq9OWF1gf6piaV0bOjYOTesPMN3ia9GKhLeicQ/640?wx_fmt=png&tp=webp&wxfrom=5&wx_lazy=1#imgIndex=3)

（图2）

时间线基数膨胀的主要原因是时间线存在⼀定的流动率（churn rate）：active 的时间线停⽌接受指标样本，变为 inactive 状态。同时，又不断有新的 active 时间线进入系统。

在具体业务中，series churn 的来源是多方面的，例如：

* 在线系统会在电商大促活动期间进行弹性扩缩容操作，扩容操作发生时，新启动的业务进程就会产生大量新的时间线；而缩容操作发生时，大量的进程消亡，对应的时间线变成inactive状态。
* 随着大规模混布技术和容器技术的应用，云上部署的服务进程会在物理机之间进行迁移。如果某些指标以物理机IP作为tag key，那么每当进程迁移到新的物理机时，就会产生⼀批新的active的时间线集合（IP 的tag value 发生更新）。

表2对来自不同租户的30分钟区间的汇报指标进行了统计。可以看出时间线基数（#series）就达到百万级别（1e7）。例如ASI租户，时间线基数到了4100万+。主要原因是 tags 的平均维度（#tags）超过31个，tag values 的基数（tag values列）超过446万。我们把 tags 维度太多导致的时间线基数膨胀问题，称作时序场景的“维度诅咒”。

![图片](https://mmbiz.qpic.cn/mmbiz_png/OmCbZ5JK30HUkAbjLFdSk9GC5KHQhEXUicLvAWMaibSZ1ib7kWzJx5DlLgEwuyf2XZXzuGmYGrm8MhmsA4C6libbSw/640?wx_fmt=png&tp=webp&wxfrom=5&wx_lazy=1#imgIndex=4)

（表2 ）

2.3及时可见性

线上业务对于监控数据的时效性要求越来越高。我们把可见性延迟（Visible-Delay）定义为时序数据产生的时间（event-time）到它可以被检索到的时间（visible-time）的gap。部分时效性敏感的业务要求可见性延迟在维持在几秒之内。举个例子，弹性扩缩容服务可能会基于某个应用最近5分钟的聚合QPS指标进行弹性扩缩决策，如果QPS指标的 visible-delay 达到分钟级别，那么弹性扩缩容服务就⽆法做出及时的决策，甚至可能基于部分聚合结果，产生错误的决策。

指标数据从产生到被存储，大致上要经历 SDK 收集、agent 采集、引擎消费这么几个阶段。其中前两个步骤运行在端上（容器、物理机），第三步通过中心化的时序引擎进行处理。因此 visible-delay 可以细分为端延迟和引擎构建延迟。

上述的三个挑战对TSDB意味着什么呢？

 **首先，** 高写入压力要求引擎提供极高吞吐的写入pipeline，同时业务场景要求数据能够被长期保留下来，意味着引擎需要提供高可靠、低成本和低访问延迟的存储方案； **第二，** 秒级的及时可见性要求系统具备实时索引（realtime-indexing）的能力；

 **最后，** 高维度高基数的数据特点对实时索引性能带来巨大的挑战，TSDB在设计上需要能控制时间线规模，且提供高吞吐的索引方案。

**03**

**系统架构介绍**

3.1 整体架构设计

指标数据采集的入口是部署在各个物理机上的指标采集模块 kmon-agent。kmon-agent 会将本地采集的原始指标降精度（down-sample）为4个精度：20秒、1分钟、10分钟、60分钟，并将降精度后的数据，写入该租户对应的4个消息队列（MessageQueue）中。

Khronos 会直接消费消息队列中的数据。如图3所示，Khronos整体架构采用了类似lambda 架构的设计，分为在线和离线两个模块：

* 在线模块：使用 Havenask 引擎（以下简称Ha3）作为在线模块，提供实时数据的消费和查询服务。Ha3 分为 QRS 和 Searcher 两种角色。QRS 模块接受用户的查询请求，进行SQL 解析、生成查询计划、将查询计划转发给 searcher 并对 searcher 返回的结果进行全局聚合和排序。Searcher 是查询计划的执行者，在时序场景，它召回符合查询条件（Metric, Tags, TimeRange）的所有时间线，并根据 group by 条件进行数据的本地聚合。为了提供高时效性的服务， searcher 会在内存中构建时序索引，并将内存中的索引定期刷写到本地磁盘上。查询时，searcher 会从内存、本地磁盘和分布式⽂件系统盘古这三种存储介质中进行数据召回。其中本地磁盘和离线盘古中的数据，会通过 BlockCache 的方式缓存（LRU）在内存中。
* 离线模块：使用 BuildService 引擎作为离线模块，提供离线数据的产出和整理服务。BuildService 会周期性启动分布式构建进程，消费MessageQueue中的数据，将时序索引产出到盘古系统上。BuildService 还会周期性调度分布式的索引整理进程，对时序索引进行整理、优化。优化后的索引版本信息会被推送到在线模块，用于替换 Ha3 searcher 本地内存中和本地磁盘上的时序索引。

![图片](https://mmbiz.qpic.cn/mmbiz_png/OmCbZ5JK30HUkAbjLFdSk9GC5KHQhEXUoF6YK3ibrNamvSw6q7st0rDeLrzzTG1wxMVdUJEdfZwEtGDnicIU154Q/640?wx_fmt=png&tp=webp&wxfrom=5&wx_lazy=1#imgIndex=5)

（图3）

采用 lambda 架构的好处在于，在线模块通过直接消费消息队列,  能够保障时序数据的时效性。而消耗 CPU 计算资源的索引整理优化逻辑可以放在离线模块进行，避免了在线服务的 CPU 抖动。但是目前的架构版本中，存在在线模块和离线模块消费两遍消息队列的构建 CPU 成本浪费，后续考虑将 Ha3 Searcher 产出的实时索引直接刷写到盘古上来节省这部分资源。

3.2 基于数据动态生命周期的存储计算分离架构

在存储结构设计上，Khronos ⽀持了在线直接访问离线盘古的存储计算分离架构：BsWorker 将离线优化后的索引直接产出到盘古（HDD) 上，在线 searcher 通过网络直接访问，省去了分发巨量索引到 searcher本地的过程。⼀方面离线盘古提供了数据的可靠存储保障和理论上⽆限制的存储空间，另⼀方面 searcher本地状态很小，利于其弹性扩缩容。

在业务场景中，时序场景的数据有明显的冷热特征，但这种冷热变化并不是静态的。比如：场景A建⽴了数据大盘，希望能快速召回最近3天的数据查询；场景B希望能进行2021、2022年大促期间的性能对比和业务数据聚合分析；场景C希望将基于 blink 流式聚合报警任务下线，有30万条报警规则需要直接从 khronos 实时聚合进行计算，要求最近1分钟的数据能够提供秒级的构建时效性和毫秒级的查询延迟。

![图片](https://mmbiz.qpic.cn/mmbiz_png/OmCbZ5JK30HUkAbjLFdSk9GC5KHQhEXU4P0NwyIWxXMf0ARWh5hlU39xyOPZaWpOV4ibWzVjv4LF3GEmHW0MHQQ/640?wx_fmt=png&tp=webp&wxfrom=5&wx_lazy=1#imgIndex=6)

（图4）

我们通过为引擎增加“动态生命周期管理” 能力来解决这部分需求。

具体而⾔，Khronos ⽀持将⼀张表内的数据分为N个冷热层级。我们可以为每一层定义多个时间窗口和一个存储介质。以图4为例配置了3阶段的生命周期：Hot Layer 配置为最近12小时，访问介质配置为内存（Ram）；Warm Layer 配置了两个时间窗口：一个是最近3天至最近12小时，另一个是去年双十一当天（用于业务上支持同期数据对比）；访问介质配置为本地SSD磁盘；Cold Layer 的时间窗口配置为最近一年至最近3天，介质配置为DFS。在这种配置下，时序数据进入引擎的12小时内。它会被加载到全内存中提供高速访问；当数据的 eventTime 和当前时间差值在3天到12小时之间时，或者命中去年双十一当天的时间区间，这部分时序数据会被迁移至本地磁盘介质（SSD），以更经济的方式提供访问。

当数据的eventTime和当前时间的差值超过3天时，它就被存放在分布式文件系统上，通过网络IO的形式提供访问。

**04**

**Data-Model 设计**

data-model 被认为是时序数据管理系统的核心“世界观”，它代表了数据库是如何对数据进行建模的。按照建模方式是否需要预定义schema, 可以将TSDB的建模方案大致分类为schematized 和 schemaless 两类。

4.1 Schematized Data Model

基本上采用类似关系数据库建模时序数据的产品中普遍需要预先定义schema。

例如TimescaleDB、QuestDB、Druid、TDEngine，我们称这类设计为 schematized data model。在数据写入引擎前，定义schema 可以带来⼀些明确的好处，包括利于查询引擎实现，尤其是提供 SQL 标准（或类似 SQL 语义）的查询引擎；其次它强制用户在写入数据前对数据建模进行仔细地设计，写入流程中可以根据schema对数据进行校验，从而避免异常数据进入引擎；另外，这也限制用户随意增加维度列，从⼀定程度上避免上文提到的“维度诅咒”。

以 TimescaleDB [4] 的数据写入为例。用户需要：

* Step1.  定义⼀个表并关联到⼀个超表 (hyper table)

```
CREATE TABLE stocks_real_time (
  time TIMESTAMPTZ NOT NULL,
  symbol TEXT NOT NULL,
  price DOUBLE PRECISION NULL,
  day_volume DOUBLE PRECISION NULL
);
SELECT create_hypertable('stocks_real_time','time');
```

* Step2. 定义索引

```
CREATE INDEX ix_symbol_time ON stocks_real_time (symbol, time DESC);
```

* Step3. 写入数据

```
INSERT INTO stocks_real_time(time, symbol, price, day_volumn)
  VALUES (NOW(), 'product', 22.2, 3300.0);
```

* Step4. 如果需要增加⼀个维度列的话，需要显式修改表结构

```
ALTER TABLE stocks_real_time ADD COLUMN week_volumn DOUBLE PRECISION NULL;
```

4.2 Khronos 的选择：Schemaless Data Model

从上述的过程中可以看出， schematized data model 的缺点就是使用体验上不够灵活。Khronos对接的业务指标的数量超过百万规模，为每个指标都定义⼀个 schema 将会带来巨量的表管理成本，且同⼀个业务存在多个代码版本，对同⼀个指标的建模也不尽相同。增减metrics、tags 和fields 都是相对高频的操作。例如代码版本升级、开发人员临时的问题调查等场景都可能更新metrics、 tags 和fields。总之如果用户需要在汇报指标前先定义 schema，并显式地为某些 tag 列建⽴索引的话，恐怕产品就要收到很多吐槽投诉了。

考虑到业务上灵活多变的指标数据建模需求，Khronos 采用了完全 schemaless 的 data model。上述流程中的 Step1、Step2 都可以省略，用户只需描述数据本⾝，并推送到系统的消息队列中就行了。这是⼀个 Khronos 的消息示例：

```
{Metric=stocks_real_time, time=1668417257, tags={symbol=product}, fields={day_volumn=3300.0, price=22.2}}
{Metric=stocks_real_time, time=1668417258, tags={symbol=product}, fields={day_volumn=3300.0, price=23.2}}
{Metric=stocks_real_time, time=1668417259, tags={symbol=pre}, fields={day_volumn=3300.0, price=24.2}}
```

Khronos 会自动为所有 tags 建⽴合适的索引。当用户想要修改数据建模时，也不需要上面 Step4、Step2 的 AlterTable/CreateIndex 过程，仍然只需要描述新数据本⾝的变化就可以。例如用户想为指标新增一个tag: "market" 和一个field: "week_volumn"，直接推送下面的消息即可：

```
{Metric=stocks_real_time, time=1668417259, tags=
{symbol=product,market=shanghai}, fields={day_volumn=3300.0, price=22.1,week_volumn=26400.0}}
{Metric=stocks_real_time, time=1668417260, tags=
{symbol=product,market=shanghai}, fields={day_volumn=3300.0, price=23.2,week_volumn=26401.0}}
```

Khronos 采用了schema-on-read 的设计理念，这种数据建模的变化是即刻生效的。当执行查询{Metric=stocks_real_time, symbol=product, field=week_volumn}时, 将召回以下数据：

![图片](https://mmbiz.qpic.cn/mmbiz_png/OmCbZ5JK30HUkAbjLFdSk9GC5KHQhEXUyjMgLeZzRsBgm3A6ib1jOic3sbP6RSe3x4VSwqiaQRZxqBic21dUo4sAyg/640?wx_fmt=png&tp=webp&wxfrom=5&wx_lazy=1#imgIndex=7)

而执行查询{Metric=stocks_real_time, market=shanghai, field=price} 时，只有第⼆批数据被召回：

![图片](https://mmbiz.qpic.cn/mmbiz_png/OmCbZ5JK30HUkAbjLFdSk9GC5KHQhEXUBk1RGsa1q0WXibDLReGxwc92nqK5wvpQBIvicc5PKyD7bb9LtHpGzshA/640?wx_fmt=png&tp=webp&wxfrom=5&wx_lazy=1#imgIndex=8)

Khronos 的 data model 可以服务于两种主流的指标数据建模方式：

**Model by Metric**

按照指标建模的方式下，tags 是用来描述指标的属性的，fields 用来存储指标对应的⼀个或多个值，知名的开源时序监控引擎 Prometheus[5] 也是采用这种方式。表3给了⼀个具体的例子，metric 列标识了指标的名称，tags 则表明对应的指标的属性。例如 http_requests_total 包含 method 和 handler 两个维度的属性，而 api_http_requests_client 则包含 method、url、ip 3个维度的属性。

![图片](https://mmbiz.qpic.cn/mmbiz_png/OmCbZ5JK30HUkAbjLFdSk9GC5KHQhEXUQBxMFmo9U67I6ulQ5JmXztjiatbuQBRHCIVg1VzY5uE4FrZwvgE0GCA/640?wx_fmt=png&tp=webp&wxfrom=5&wx_lazy=1#imgIndex=9)（表3）

**Model by Data Source**

这种建模方案下，Data Source 代表产生数据的⼀类任务或者进程。metric 标识 data source 的名称，tags 用于描述数据源的属性，field names 用于存储指标名称，field values 用于存储指标数值。集团⼴泛使用的sunfire平台[9] 可以认为采用了按照data source建模的方案。表4给了⼀个具体的例子:

![图片](https://mmbiz.qpic.cn/mmbiz_png/OmCbZ5JK30HUkAbjLFdSk9GC5KHQhEXUYp8141BWX45O0IZKEsCtib4ByoiatzDERGZhLdEaQAFgS8h868Dg3QsQ/640?wx_fmt=png&tp=webp&wxfrom=5&wx_lazy=1#imgIndex=10)

（表4）

这两种建模方式各有千秋，但对于 Khronos 来说没有差别，即每个 sample 包含⼀个 metric 字段、⼀个 timestamp、多个 tags 和多个 fields。相比于其它时序产品，Khronos 在设计上不需要预先定义的 schema，它相信每个 sample 都可以自解释，且允许模型发生变化。

**05**

**索引方案设计**

5.1 Sample-Oriented vs. Series-Oriented

时序数据的索引策略按照索引对象来划分，可以被分类为 Sample-Oriented 方案和 Series-Oriented 方案。Druid、TimescaleDB 是采用Sample-Oriented 策略的典型数据库。以 Druid为例，它为每⼀列的每个 distinct value 建⽴⼀个倒排索引。倒排链表指向了包含这个 token 的所有 samples 所在的行号。

下图给了⼀个具体的例子：Justin Bieber 的倒排链指向了row 0 和 row 1。

![图片](https://mmbiz.qpic.cn/mmbiz_png/OmCbZ5JK30HUkAbjLFdSk9GC5KHQhEXUslJBA7iaXh1bNR53OeribmiaWKpPEar1ibH5o8gRChJVsBTgPTeFtIJic8Q/640?wx_fmt=png&tp=webp&wxfrom=5&wx_lazy=1#imgIndex=11)

（图5）

但在性能监控场景，sample-oriented 策略的⼀个大问题就是sample的数量太大，索引每个samples 会带来巨大的计算和存储成本。通常，点数量和时间线数量的比值大约是30 ~ 100倍，意味着为时间线建⽴索引就比为 sample 建⽴索引要轻的多。

因此 series-oriented 索引策略也越来越成为监控领域的主流，InfluxDB、Prometheus、Google Monarch等都采用了这样的方案。扩展来说，在⽇志分析领域， ElasticSearch 可以被认为采用了 sample-oriented 方案，而 Loki 采用了 series-oriented方案。

5.2 时间线索引的构建性能瓶颈分析

时间线索引的构建过程可以被抽象成下面的算法过程，简单描述⼀下：当时间线进入引擎时，先判断⼀下是否已经被索引过。如果没有索引过，那么进入索引构建流程。如果已经被索引过，即只需把它的点数据插入对应的 sample buffer 中。

![图片](https://mmbiz.qpic.cn/mmbiz_png/OmCbZ5JK30HUkAbjLFdSk9GC5KHQhEXUgg4Hw1LbsjNBDHok67aEicvdYatnJ3mpCp4PSa2SBk6icjvntao93Hyw/640?wx_fmt=png&tp=webp&wxfrom=5&wx_lazy=1#imgIndex=12)

（图6)

⽬前，倒排索引和基于树的索引被⼴泛应用在 TSDB 系统中。下图展示了⼀个具体的索引构建流程:  时间线的 SeriesKey ⾸先被分配⼀个 seriesId 作为它的标识符，然后它被分词为 metric-token 和⼀组 tagk-tagv token。seriesId 会被插入到这些 token 对应的倒排链表中，Metric 和 tagk-tagv token 也会被插入到前缀树中用来⽀持时间线的模糊匹配和meta查询。

![图片](https://mmbiz.qpic.cn/mmbiz_png/OmCbZ5JK30HUkAbjLFdSk9GC5KHQhEXUxTMicLRB3Lbf3CnicgibPLwvf4dVPyicFBmvvqhbEL8A6gn4ianuepSxIDQ/640?wx_fmt=png&tp=webp&wxfrom=5&wx_lazy=1#imgIndex=13)

(图7)

在超高基数的时间线写入负载下，该算法的容易出现以下两个方面的问题：

* 时间线索引构建性能达到瓶颈：主流的时间线索引方案包括倒排索引、前缀树索引等，构建开销正比于时间线基数（#series）和每个时间线包含的 tags 个数（#tags)，从表2可以看出，两个数字都不是小数⽬。
* 冷启动问题：这指的是时间线索引刚创建时，索引构建吞吐会剧烈下降，索引构建延迟会迅速上升，索引构建性能要经历较长的时间才能恢复平稳的现象。这是由于时间线索引的内存不能能⽆限制增长，需要周期性地序列化到磁盘上，并重新创建空索引。空索引创建时，进入系统的大部分的时间线会进入开销昂贵的索引构建分⽀。图8(b) 画出了⼀个 Khronos 老版本下的周期性冷启动现象。可以看出，⽆论是数据写入吞吐还是索引构建延迟，都会发生周期性的抖动，这对前面章节提到的“及时可见性”造成严重影响。

![图片](https://mmbiz.qpic.cn/mmbiz_png/OmCbZ5JK30HUkAbjLFdSk9GC5KHQhEXUewicXgpqoj3Y9vnfnEeA6mEcO4Qaahnjb6AlQIIyic6hGkThhSMBpU9A/640?wx_fmt=png&tp=webp&wxfrom=5&wx_lazy=1#imgIndex=14)

(图8)

图8(a)中，我们对比了两种索引方案在 igraph 性能监控数据集下的峰值写入吞吐能力对比，InfluxDB(series-oriented) 的峰值吞吐能力是 TimescaleDB(sample-oriented) 的3倍左右，但是冷启动问题发生时，性能就会严重下降。

5.3 构建性能优化

今年，Khronos 针对上述的索引构建性能瓶颈问题进行了专门的优化。设计了⼀个新的“补集索引构建算法” 。这部分⼯作的细节我们总结为了⼀篇 paper 发表在 CIKM2023[6] 上："Khronos: A Real-time Indexing Framework for Time Series Databases on Large-Scale Performance Monitoring Systems" [7]。在高基数时间序列负载下，该算法表现要显著优于 InfluxDB。

下图对比了Khronos、InfluxDB、TimescaleDB 在不同基数的数据集下的单实例峰值写入吞吐能力。

![图片](https://mmbiz.qpic.cn/mmbiz_png/OmCbZ5JK30HUkAbjLFdSk9GC5KHQhEXUq59EC83X7rjYwtTuGlLHy4PKT3H0ERSr6HDoiblgsFVldNiamtAM73Cg/640?wx_fmt=png&tp=webp&wxfrom=5&wx_lazy=1#imgIndex=15)

(图9)

随着数据集的时间线基数从 0.6Million 增长到 9 Million，InfluxDB 的写入性能下降了约66%。而 Khronos 的构建性能⼀直稳定保持在 1Million samples/second 以上，是 InfluxDB 的18倍以上。在和TimescaleDB 对比时，我们进行两种配置: TimeSNon表示不对tag列配置索引，TimeSInd 表示对所有tag列都配置索引。显而易见，即使对比TimeSNon方案，Khronos 仍然有很高的性能表现（x4.5）。

在优化构建延迟方面，Khronos 新版本解决了周期性的冷启动问题，下图给出了新老两个版本在ASI数据集下的单实例构建性能和构建延迟对比。可以看出，新版本在均值250K的写入压力下，索引构建延迟仍稳定在2s以内。

![图片](https://mmbiz.qpic.cn/mmbiz_png/OmCbZ5JK30HUkAbjLFdSk9GC5KHQhEXU8cjG6Ny3muXTX0rfRYJc7lMDAgTian1JwPTCQAhnePWhD4NVPDgpMbQ/640?wx_fmt=png&tp=webp&wxfrom=5&wx_lazy=1#imgIndex=16)

(图10)

**06**

**总结和展望**

以上是 Khronos 团队在应对超大规模性能监控场景的⼀些实践经验。后续引擎会在完善可观测数据模型体系上持续发力，例如接入⽇志、trace、event、profiling、用户行为序列数据等。⽬前行业内还没有⼀款产品称得上是可观测性领域的“六边形”战⼠，还有很多机会可以挖掘。

参考资料

[01] DB-engiens ranking

**https://db-engines.com/en/ranking**

[02] AIOS技术栈

**https://developer.aliyun.com/article/674167**

[03] Havenask 搜索引擎

**https://github.com/alibaba/havenask**

[04] TimescaleDB Docs

**https://docs.timescale.com/**

[05] Prometheus data model

**https://prometheus.io/docs/concepts/data_model/**

[06]  CIKM2023 Accepted Papers

**https://uobevents.eventsair.com/cikm2023/accepted-papers **

[07] Khronos 论文原文（点击**阅读原文**可直接查看）

**https://dl.acm.org/doi/10.1145/3583780.3614944#**

![图片](https://mmbiz.qpic.cn/mmbiz_gif/OmCbZ5JK30GbpbpADRYqgC6MMvNfRPY8cGySPF2f9miavJibvCOiaUelcS2LX6uSMGEic1ztD9ECCvN2rcupEseySg/640?wx_fmt=gif&wxfrom=5&wx_lazy=1&tp=webp#imgIndex=17)

![图片](https://mmbiz.qpic.cn/mmbiz_gif/OmCbZ5JK30GMpsz1kdxGBRiclB5yjhBh5iamZLYrF2BOK4YNhXbjibibx63B6o5ubgOvYGRyUa0DK6gpI9YibkKUUCQ/640?wx_fmt=gif&wxfrom=5&wx_lazy=1&tp=webp#imgIndex=18)

欢迎留言一起参与讨论~
