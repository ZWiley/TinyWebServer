

服务器压力测试
===============
Webbench是有名的网站压力测试工具，它是由[Lionbridge](http://www.lionbridge.com)公司开发。

> * 测试处在相同硬件上，不同服务的性能以及不同硬件上同一个服务的运行状况。
> * 展示服务器的两项内容：每秒钟响应请求数和每秒钟传输数据量。




测试规则
------------
* 测试示例

    ```C++
	webbench -c 500  -t  30   http://127.0.0.1/phpionfo.php
    ```
* 参数

> * `-c` 表示客户端数
> * `-t` 表示时间


测试结果
---------
Webbench对服务器进行压力测试，经压力测试可以实现上万的并发连接.
> * 并发连接总数：10500
> * 访问服务器时间：5s
> * 每秒钟响应请求数：552852 pages/min
> * 每秒钟传输数据量：1031990 bytes/sec
> * 所有访问均成功

<div align=center><img src="https://github.com/twomonkeyclub/TinyWebServer/blob/master/root/testresult.png" height="201"/> </div>

模块详解
------------
请关注公众号 **“两猿社”**.
> * 选择底部菜单栏：**互联网 -> 练手项目 -> 13 服务器测试**
> * **带你丰富互联网相关项目经验，轻松应对校招！！！**
> * **项目模块详细讲解，在公众号内持续更新！！！**

<div align=center><img src="https://github.com/twomonkeyclub/TinyWebServer/blob/master/root/test1.jpg" height="350"/> </div>