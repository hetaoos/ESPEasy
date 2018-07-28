# ESPEasy
Easy MultiSensor device based on ESP8266 https://github.com/letscontrolit/ESPEasy

# How To
- [Tutorial Arduino Firmware Upload](https://www.letscontrolit.com/wiki/index.php/Tutorial_Arduino_Firmware_Upload)

# 修改内容
## 合并
- 2018-07-28 合并 [mega-20180723](https://github.com/letscontrolit/ESPEasy/commit/e596403c0a5cc3ffb0f02c972753941bbb32b3bf) [相关提交](https://github.com/hetaoos/ESPEasy/commit/c47d1c5303cc916e79e37a915d1f3b283d97cfeb)

## 补丁
- 增加达特甲醛传感器 `WZ-S` 支持 [Plugin 200: Dart WZ-S](ESPEasy/_P200_WZ_S.ino)
- 增加攀藤PM2.5+甲醛+温湿度传感器 `PMS5003ST` 支持 [Plugin 201: Plantower PMS5003ST](ESPEasy/_P201_PMS5003ST.ino)

## 控制器
- 新增 `百度 IOT MQTT` 支持 [Baidu MQTT](ESPEasy/_C014.ino) , 详情请参见 [天工-智能物联网](https://cloud.baidu.com/solution/iot/index.html)

## 系统
- 将控制器名称大小从26改为64，因为百度MQTT的用户名比较长！！[相关提交](https://github.com/hetaoos/ESPEasy/commit/5d6178165ccef5dd0e6dd6114f4d0c9afca12209)
- MQTT 配置支持设置 `Client Id`，因为百度MQTT要求`Client Id` 比较特殊，自动生成的无效或者不方便配置。[相关提交](https://github.com/hetaoos/ESPEasy/commit/14c33edc02204bbb59d0edb48d6d31b536ce2c5a)
- 将默认设备最大数量`DEVICES_MAX`从50改为60 [相关提交](https://github.com/hetaoos/ESPEasy/commit/f48a1eb1c84332f0378afc349824736c7b2c6dc1)
- 将默认每个任务的最大输出变量数`VARS_PER_TASK`从4改为8 [相关提交](https://github.com/hetaoos/ESPEasy/commit/f48a1eb1c84332f0378afc349824736c7b2c6dc1)
- 增加传感器类型`SENSOR_TYPE_*`，也就是标明传感器有多少个输出数据的定义：`SENSOR_TYPE_PENTA`、`SENSOR_TYPE_HEXA`、 `SENSOR_TYPE_HEPTA`、`SENSOR_TYPE_OCTA`[相关提交](https://github.com/hetaoos/ESPEasy/commit/f48a1eb1c84332f0378afc349824736c7b2c6dc1)


# 如何编译源码

## 配置编译环境
PS：主要是对[Tutorial Arduino Firmware Upload](https://www.letscontrolit.com/wiki/index.php/Tutorial_Arduino_Firmware_Upload)的简要说明
- 下载和安装 [`Arduino IDE`](https://www.arduino.cc/en/Main/Software?setlang=cn)
- `文件`-`首选项` 中，修过`附加开发板管理器网址` 为 `http://arduino.esp8266.com/stable/package_esp8266com_index.json`
- `工具`-`开发版"xxxx"`-`开发板管理器` 中，搜索 **ESP8266** ，选中**esp8266 by ESP8266 Community**，并选择最新安装。
- `工具`-`开发版"xxxx"` 中，选择**NodeMCU 1.0 (ESP-12E) Module**
- `工具`-`端口"xxxx"` 中，寻找接入NodeMCU后识别的串口。

## 编译源码
- 下载或者 clone 源码到本地
- 将 `Libraries` 目录下的全部文件拷贝到 `文档\Arduino\Libraries`目录下
- 双击打开 [`ESPEasy\ESPEasy.ino`](ESPEasy/ESPEasy.ino) 即可打开 `Arduino IDE`
- 更新完成源码后，点击`项目`-`上传`，即可编译固件并推送到开发板上

## 启用自定义设置（可选）
- 将工作目录下的 [`Custom-sample.h`](ESPEasy/Custom-sample.h) 复制并命名为 `Custom.h`
- 打开 `%UserProfile%\AppData\Local\Arduino15\packages\esp8266\hardware\esp8266\2.4.1\platform.txt` 文件
  - 找到 `build.extra_flags` 该行
  - 在后面添加参数 ` -DUSE_CUSTOM_H`
  - 注意：路径名根据版本不同，又有些许变化
