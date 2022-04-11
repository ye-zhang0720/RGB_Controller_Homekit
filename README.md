# RGB_Controller_Homekit

该灯带控制器采用Arduino IDE开发，使用esp8266 12f作为控制器。本控制器兼容12v 24v灯带。

电路图中稳压二极管采用12v稳压二极管，若采用12v电源，该二极管可以不加。

使用方法：
打开串口调试工具，连接电源，待LED闪烁，手机连接名为Homekit_Light的Wi-Fi，地址栏输入地址192.168.4.1，按照提示配网。

然后在串口调试工具中查找Setup_Code,打开家庭，添加配件，输入Setup_Code，添加成功。
