# 智能仓储资产管理系统 — 完整说明文档

## 一、项目概述

本项目是一个基于 **STM32 + RFID + MQTT + Node.js + uni-app** 的智能仓储资产管理系统。系统通过 RFID 射频识别技术自动检测仓库中的资产标签，将数据通过 MQTT 协议传输到后端服务器存储，用户可通过移动端/网页端应用实时查看、管理和编辑资产信息。

### 1.1 系统组成

| 模块 | 技术栈 | 说明 |
|------|--------|------|
| **硬件层** | STM32F103 + ESP8266 + RFID读写器 + OLED显示屏 | 负责 RFID 标签读取、数据显示、网络通信 |
| **通信层** | Mosquitto MQTT Broker | 负责硬件与后端之间的消息代理 |
| **后端层** | Node.js + Express + MySQL + MQTT.js | 提供 RESTful API、MQTT 消息处理、数据持久化 |
| **前端层** | uni-app (Vue.js) | 跨平台移动端应用，提供资产管理界面 |

### 1.2 核心功能

- **自动资产检测**: RFID 读写器自动扫描范围内的资产标签
- **实时数据同步**: 通过 MQTT 协议将资产数据实时上传到服务器
- **资产管理**: 支持资产的增删改查（CRUD）操作
- **待确认资产处理**: 新检测到的资产需用户确认后录入数据库
- **搜索模式控制**: 远程开启/关闭 RFID 搜索功能
- **信号强度显示**: 显示 RFID 标签的 RSSI 信号强度

---

## 二、系统架构与运行原理

### 2.1 系统架构图


### 2.2 运行原理详解

#### 整体数据流

系统采用 **发布/订阅（Publish/Subscribe）** 的 MQTT 消息模式，数据流向如下：

```
RFID标签 → RFID读写器 → STM32 → ESP8266 → MQTT Broker → Node.js → MySQL
                                                              ↑
                                                    前端 ←── Node.js API
```

#### 详细工作流程

**步骤1: RFID 标签检测**
- RFID 读写器持续发射射频信号，检测范围内的 RFID 标签
- 标签响应读写器的查询，返回其唯一的 EPC 编码（12字节卡号）和信号强度（RSSI）
- 读写器通过 UART 串口将数据发送给 STM32

**步骤2: STM32 数据处理**
- STM32 接收串口数据，进行解包和校验和验证
- 使用去重算法（时间窗口 + LRU缓存）过滤重复读取
- 将新卡号存储到内部 FLASH（最多9个资产）
- 同时在 OLED 屏幕上显示读卡状态和卡号信息

**步骤3: MQTT 数据上传**
- STM32 通过 ESP8266 WiFi 模块建立 TCP 连接到 MQTT Broker（192.168.199.100:1883）
- 使用 MQTT 协议将资产数据封装为 JSON 格式，发布到 `asset/post` 主题
- 数据格式示例：
  ```json
  {
    "send_number": 2,
    "params": {
      "object1": {"value": {"id": 1, "RSSI": -45, "name": "电脑", "note": "办公用", "card": "A1B2C3D4E5F6"}},
      "object2": {"value": {"id": 2, "RSSI": -52, "name": "打印机", "note": "", "card": "F6E5D4C3B2A1"}}
    }
  }
  ```

**步骤4: 后端消息处理**
- Node.js 后端作为 MQTT 客户端订阅 `asset/post` 主题
- 收到消息后解析 JSON 数据，提取资产信息
- 检查数据库中是否已存在该资产：
  - 若已存在：直接更新 RSSI 和时间戳
  - 若不存在：加入待确认列表（pendingAssets）

**步骤5: 前端数据展示**
- 前端通过 HTTP 轮询（默认10秒间隔）调用后端 REST API
- 获取在库资产列表和待确认资产列表
- 以表格形式展示资产信息，包括序号、卡号、名称、信号强度、备注等

**步骤6: 用户操作反馈**
- 用户在界面上确认/拒绝待确认资产
- 修改资产名称和备注
- 删除不需要的资产
- 开启/关闭 RFID 搜索模式
- 这些操作通过 HTTP API 发送到后端，后端再通过 MQTT 将命令下发到 STM32

#### MQTT 主题说明

| 主题 | 方向 | 说明 |
|------|------|------|
| `asset/post` | 设备 → 服务器 | 资产数据上报 |
| `asset/command` | 服务器 → 设备 | 控制命令下发（搜索开关、删除等） |

#### 心跳与断线重连

- STM32 每60秒发送一次 MQTT PING 心跳包
- 若10次心跳无响应，认为连接断开
- 自动重新初始化 ESP8266 并重连 MQTT Broker

---

## 三、硬件层（STM32）详细说明

### 3.1 目录结构

```
STM32/
├── user/                    # 用户代码
│   ├── main.c              # 主程序入口
│   ├── Object.h            # 资产对象和发送设置结构体定义
│   ├── NVIC.c              # 中断控制器配置
│   └── NVIC.h              # 中断控制器头文件
├── NET/                     # 网络通信模块
│   ├── Broker/             # MQTT Broker 通信
│   │   ├── inc/Broker.h    # Broker 接口声明
│   │   └── src/Broker.c    # Broker 实现（连接、发送、订阅、消息解析）
│   ├── MQTT/               # MQTT 协议栈
│   │   ├── MqttKit.c/h     # MQTT 数据包封装/解包
│   │   ├── Common.h        # 公共定义
│   │   └── SDK使用说明.txt # SDK 使用说明
│   └── CJSON/              # JSON 解析库
│       ├── cJSON.c         # cJSON 实现
│       └── cJSON.h         # cJSON 头文件
├── RFID/                    # RFID 读写模块
│   ├── RFID.c              # RFID 驱动实现
│   └── RFID.h              # RFID 驱动头文件
├── OLED/                    # OLED 显示模块
├── hardware/                # 硬件驱动（delay, usart, Timer, Flash等）
├── fwlib/                   # STM32 标准外设库
├── system/                  # 系统初始化
└── stm32f103.uvprojx        # Keil MDK 工程文件
```

### 3.2 核心文件解析

#### main.c — 主程序

**功能**: 系统初始化、主循环控制、RFID 读卡处理、MQTT 通信管理

**主要流程**:
1. `Hardware_Init()` — 初始化 NVIC、延时、串口、定时器、OLED、RFID
2. `ESP8266_Init()` — 初始化 WiFi 模块
3. 连接 MQTT Broker（TCP 连接到 192.168.199.100:1883）
4. `Broker_Link()` — MQTT 协议连接
5. `Broker_Subscribe()` — 订阅 `asset/command` 主题
6. 主循环：
   - 根据 `search` 标志位决定是否执行 RFID 轮询
   - 读卡 → 解包 → 去重 → 存储到 FLASH
   - 处理 MQTT 下行消息（删除命令、搜索开关）
   - 定时发送数据到服务器
   - 心跳检测和断线重连

**关键数据结构**:
```c
// Object.h 定义
typedef struct {
    uint8_t serial_number;  // 序号 (1-9)
    char card[64];          // 卡号（十六进制字符串）
    char name[16];          // 名称
    char note[32];          // 备注
    int RSSI;               // 信号强度
} Object;

typedef struct {
    uint8_t send_reagy;     // 发送标志位
    u32 time;               // 上次发送时间
    u16 sendwhich;          // 发送哪些 object（位掩码）
    u8 send_number;         // 发送数量
} Send_Setting;
```

#### Broker.c — MQTT Broker 通信

**功能**: 实现与 MQTT Broker 的完整通信流程

| 函数 | 功能 |
|------|------|
| `Broker_Link()` | 建立 MQTT 连接，发送 CONNECT 包，等待 CONNACK |
| `Broker_SendData()` | 封装资产数据为 JSON，通过 MQTT PUBLISH 发送 |
| `Broker_Subscribe()` | 订阅 `asset/command` 主题 |
| `Broker_Ping()` | 发送 PINGREQ 心跳包 |
| `Broker_RevPro()` | 解析接收到的 MQTT 消息（PUBLISH/PUBACK/SUBACK/PINGRESP） |
| `Broker_FillBuf()` | 将资产数据填充为 JSON 格式 |
| `Broker_Publish()` | 发布消息到指定主题 |

**消息解析逻辑** (`Broker_RevPro`):
- 接收 `asset/command` 主题的消息
- 解析 JSON 中的 `params.delete.id` → 设置 `flash_w_flag` 删除对应 FLASH 中的资产
- 解析 JSON 中的 `params.search` → 控制 RFID 搜索开关

#### RFID.c — RFID 读写驱动

**功能**: RFID 读写器通信、数据解包、去重过滤

| 函数 | 功能 |
|------|------|
| `RFID_SearchOnce()` | 发送单次轮询命令（`BB 00 22 00 00 22 7E`） |
| `RFID_GetRxFlag()` | 获取接收标志位（自动清零） |
| `RFID_Unpacket()` | 解包接收数据，提取12字节卡号和RSSI |
| `RFID_CheckDuplicate()` | 去重检测（时间窗口10ms + LRU缓存10个标签） |
| `RFID_CleanExpired()` | 清理过期标签 |
| `RSSI_Filter()` | 一阶低通滤波：`(last*7 + new)/8` |
| `Get_Checksum()` | 校验和验证 |
| `USART1_IRQHandler()` | 串口1中断服务函数（接收RFID数据） |

**去重算法**:
- 维护一个大小为10的标签缓存（LRU）
- 每个标签记录 EPC、最后读取时间、RSSI
- 同一标签在10ms时间窗口内重复读取会被过滤
- RSSI 使用一阶低通滤波平滑处理
- 信号强度低于 -70dBm 的标签会被忽略

#### RFID.h — RFID 头文件

```c
#define TAG_CACHE_SIZE   10      // 标签缓存大小
#define EPC_LEN          12      // EPC 长度（字节）
#define TIME_WINDOW_MS   10      // 去重时间窗口（毫秒）
#define RSSI_THRESHOLD_DEFAULT -70  // RSSI 阈值（dBm）
```

### 3.3 硬件连接

| 设备 | 连接方式 | 引脚/接口 |
|------|----------|-----------|
| RFID 读写器 | UART1 | PA9(TX), PA10(RX), 波特率115200 |
| ESP8266 | UART2/UART3 | 波特率115200 |
| OLED 显示屏 | I2C/SPI | 根据具体驱动 |
| FLASH 存储 | 内部FLASH | 存储9个资产卡号 |

---

## 四、后端层（Node.js）详细说明

### 4.1 目录结构

```
后端/
├── app.js                  # 应用入口，HTTP路由和MQTT消息处理
├── package.json            # 项目依赖配置
├── config/
│   └── db.js              # MySQL 数据库连接配置
├── controllers/
│   └── assetController.js  # 资产控制器（业务逻辑）
├── routes/
│   └── asset.js           # 资产路由定义
└── services/
    ├── mqttClient.js       # MQTT 客户端（连接、发布、订阅）
    └── assetService.js     # 资产数据服务（数据库操作）
```

### 4.2 核心文件解析

#### app.js — 应用入口

**功能**: Express 服务器启动、HTTP 路由注册、MQTT 消息处理

**启动流程**:
1. 创建 Express 应用，配置 CORS 和 JSON 解析中间件
2. 连接 MySQL 数据库（localhost, root/123456, asset库）
3. 初始化 MQTT 客户端，连接到本地 Mosquitto Broker
4. 设置 MQTT 消息处理函数
5. 启动 HTTP 服务器（端口 3000）

**HTTP API 路由**:

| 方法 | 路径 | 功能 |
|------|------|------|
| POST | `/api/mqtt/search-command` | 发送搜索命令到设备 |
| GET | `/api/assets/pending` | 获取待确认资产列表 |
| GET | `/api/assets/search` | 搜索检测（返回待确认资产） |
| POST | `/api/assets/approve` | 确认录入资产 |
| POST | `/api/assets/reject` | 拒绝资产 |
| GET | `/api/mqtt/status` | 获取 MQTT 连接状态 |
| GET | `/api/assets` | 获取所有资产 |
| GET | `/api/assets/:id` | 获取单个资产 |
| POST | `/api/assets` | 新增资产 |
| PUT | `/api/assets/:id` | 修改资产 |
| DELETE | `/api/assets/:id` | 删除资产（同时发送MQTT删除命令） |

**MQTT 消息处理流程**:
```
收到 asset/post 消息 → 解析JSON → 提取资产数据 → 检查数据库
    → 已存在: 直接更新
    → 不存在: 加入待确认列表
```

#### mqttClient.js — MQTT 客户端

**功能**: 管理 MQTT 连接、发送命令到设备

| 函数 | 功能 |
|------|------|
| `initMQTT()` | 初始化 MQTT 连接，订阅 `asset/post` 主题 |
| `sendSearchCommand(val)` | 发送搜索命令到 `asset/command` 主题 |
| `sendDeleteCommand(id)` | 发送删除命令到 `asset/command` 主题 |
| `sendAssetToDevice(id, data)` | 发送资产数据到设备 |
| `getMQTTStatus()` | 获取连接状态 |
| `setMessageHandler(handler)` | 设置消息处理回调 |

**连接配置**:
- Host: 127.0.0.1
- Port: 1883
- ClientID: local_client
- 重连间隔: 3000ms
- 保活时间: 60秒

#### assetController.js — 资产控制器

**功能**: 处理 HTTP 请求的业务逻辑

**待确认资产机制**:
- 维护一个内存中的 `pendingAssets` 数组
- MQTT 收到新资产时加入待确认列表
- 前端轮询 `/api/assets/search` 获取待确认资产
- 用户确认后写入数据库并从待确认列表移除
- 用户拒绝时直接从待确认列表移除

**删除流程**:
1. 从数据库删除资产记录
2. 通过 MQTT 发送删除命令到 STM32 设备
3. STM32 收到命令后清除对应 FLASH 中的卡号

#### assetService.js — 数据服务层

**功能**: 封装所有数据库操作

| 函数 | SQL 操作 |
|------|----------|
| `getAllAssets()` | SELECT all FROM asset ORDER BY id |
| `getAssetById()` | SELECT * WHERE id = ? |
| `addAsset()` | INSERT INTO asset |
| `updateAsset()` | UPDATE asset SET ... WHERE id = ? |
| `deleteAsset()` | DELETE FROM asset WHERE id = ? |
| `upsertAsset()` | INSERT ... ON DUPLICATE KEY UPDATE |
| `getAssetByCard()` | SELECT * WHERE card = ? |

#### db.js — 数据库配置

```javascript
// MySQL 连接配置
host: 'localhost'
user: 'root'
password: '123456'
database: 'asset'
charset: 'utf8mb4'
```

**数据库表结构**（asset 表）:

| 字段 | 类型 | 说明 |
|------|------|------|
| id | INT | 主键，资产序号（1-9） |
| card | VARCHAR | RFID 卡号 |
| name | VARCHAR | 资产名称 |
| note | VARCHAR | 备注（值为'delete'表示已删除） |
| RSSI | INT | 信号强度（dBm） |
| time | BIGINT | 更新时间戳 |

### 4.3 依赖包

| 包名 | 用途 |
|------|------|
| express | HTTP 服务器框架 |
| mysql | MySQL 数据库驱动 |
| mqtt | MQTT 客户端库 |
| cors | 跨域资源共享 |
| axios | HTTP 请求库 |
| node-cron | 定时任务 |
| ip | IP 地址工具 |

---

## 五、前端层（uni-app）详细说明

### 5.1 目录结构

```
前端/
├── App.vue                 # 应用根组件
├── main.js                 # 应用入口
├── pages.json              # 页面路由配置
├── manifest.json           # 应用配置清单
├── uni.scss                # 全局样式
├── index.html              # H5 入口 HTML
├── pages/
│   └── detail/
│       ├── detail.vue      # 主页面模板
│       ├── methods.js      # 页面逻辑（数据和方法）
│       └── styles.css      # 页面样式
├── static/                 # 静态资源
└── uni_modules/            # uni-app 插件
```

### 5.2 核心文件解析

#### detail.vue — 主页面模板

**页面结构**:
1. **头部区域**: 显示"资产管理"标题和在库资产数量
2. **功能入口**: 资产管理卡片（点击查看列表）
3. **快捷操作**: 开始搜索、资产列表、刷新数据
4. **待处理提示条**: 有待确认资产时显示
5. **最近动态**: 显示在库资产数量和平均信号强度
6. **资产管理弹窗**: 表格形式展示所有在库资产
7. **资产详情弹窗**: 查看/编辑单个资产详情
8. **待处理资产弹窗**: 展示待确认资产，支持录入/取消

#### methods.js — 页面逻辑

**数据状态**:
```javascript
{
  cardList: [],              // 9个资产槽位
  isSearching: false,        // 搜索模式开关
  pendingAssetsList: [],     // 待确认资产列表
  mqttStatus: {},            // MQTT 连接状态
  searchInterval: 500,       // 搜索轮询间隔（毫秒）
}
```

**核心方法**:

| 方法 | 功能 |
|------|------|
| `fetchAllAssets()` | 获取所有在库资产 |
| `searchDetect()` | 搜索检测，获取待确认资产 |
| `toggleSearchMode()` | 切换搜索模式（通过后端发送MQTT命令） |
| `approveAsset()` | 录入单个资产 |
| `rejectAsset()` | 拒绝单个资产 |
| `approveAllAssets()` | 批量录入 |
| `rejectAllAssets()` | 批量拒绝 |
| `confirmDelete()` | 删除资产（通过后端发送MQTT删除命令） |
| `saveEdit()` | 保存编辑 |
| `startPolling()` | 启动普通轮询（10秒间隔） |
| `startSearchPolling()` | 启动搜索轮询（500毫秒间隔） |

**轮询机制**:
- 普通模式：每10秒轮询一次 `/api/assets`
- 搜索模式：每500毫秒轮询一次 `/api/assets/search`
- 页面隐藏时停止轮询

**API 配置**:
```javascript
const ENV_CONFIG = {
  API_BASE_URL: 'http://localhost:3000'  // 后端地址
}
const PRODUCT_ID = '9H1EHSRbeo'
const DEVICE_NAME = 't1'
const ASSET_COUNT = 9
```

### 5.3 页面配置

**pages.json**:
```json
{
  "pages": [{
    "path": "pages/detail/detail",
    "style": {
      "navigationBarTitleText": "智能仓储"
    }
  }],
  "globalStyle": {
    "navigationBarTextStyle": "black",
    "navigationBarBackgroundColor": "#F8F8F8"
  }
}
```

---

## 六、系统部署与运行

### 6.1 环境要求

| 组件 | 要求 |
|------|------|
| MQTT Broker | Mosquitto（本地运行，端口1883） |
| 数据库 | MySQL（数据库名: asset，用户: root，密码: 123456） |
| 后端 | Node.js ≥ 14 |
| 前端 | HBuilderX（编译运行 uni-app） |
| 硬件 | STM32F103 + ESP8266 + RFID读写器 + OLED |

### 6.2 启动步骤

**步骤1: 启动 MQTT Broker**
```bash
# 启动 Mosquitto 代理服务器
mosquitto
```

**步骤2: 启动后端服务器**
```bash
cd 后端
npm install          # 安装依赖（首次运行）
node app.js          # 启动服务器
```
后端将在 `http://0.0.0.0:3000` 启动

**步骤3: 配置硬件**
- 修改 `main.c` 中的 MQTT Broker 地址为实际 IP
- 将 STM32 程序烧录到开发板
- 确保 ESP8266 连接到同一网络

**步骤4: 启动前端**
- 使用 HBuilderX 打开前端项目
- 运行到浏览器或编译为 APP

### 6.3 网络配置

- MQTT Broker 地址: `192.168.199.100:1883`（STM32端配置）
- 后端 MQTT 连接: `127.0.0.1:1883`
- 后端 HTTP 端口: `3000`
- 前端 API 地址: `http://localhost:3000`

---

## 七、数据流程图

### 7.1 资产检测上报流程

```
┌──────────┐     ┌──────────┐     ┌──────────┐     ┌────────────┐     ┌──────────┐
│ RFID标签  │────▶│ RFID读写器 │────▶│  STM32   │────▶│ ESP8266    │────▶│ MQTT Broker│
└──────────┘     └──────────┘     └──────────┘     └────────────┘     └──────────┘
                                                              射频信号      UART        MQTT Publish
                                                                                                    │
                                                                                                    ▼
                                                                                           ┌──────────────┐
                                                                                           │  Node.js     │
                                                                                           │  后端服务器   │
                                                                                           └──────┬───────┘
                                                                                                    │
                                                                                                    ▼
                                                                                           ┌──────────────┐
                                                                                           │   MySQL      │
                                                                                           │   数据库     │
                                                                                           └──────────────┘
```

### 7.2 控制命令下发流程

```
┌──────────┐     ┌──────────┐     ┌────────────┐     ┌──────────┐     ┌──────────┐
│   用户    │────▶│  uni-app  │────▶│  Node.js   │────▶│ MQTT     │────▶│ ESP8266  │
│  (前端操作)│     │  前端     │     │  后端服务器  │     │ Broker   │     │          │
└──────────┘     └──────────┘     └────────────┘     └──────────┘     └────┬─────┘
                                                                             HTTP API    MQTT Publish              │
                                                                             ▼                                       ▼
                                                                      ┌──────────┐
                                                                      │  STM32   │
                                                                      │          │
                                                                      └────┬─────┘
                                                                           UART
                                                                           ▼
                                                                    ┌──────────┐
                                                                    │ RFID读写器 │
                                                                    └──────────┘
```

---

## 八、关键技术点

### 8.1 RFID 去重算法

采用 **时间窗口 + LRU 缓存** 的双重去重策略：
- 维护10个标签的缓存池
- 同一标签在10ms内重复读取被过滤
- 缓存满时使用 LRU（最近最少使用）替换
- RSSI 一阶低通滤波：`filtered = (last * 7 + new) / 8`

### 8.2 MQTT 心跳机制

- 每60秒发送一次 PINGREQ
- 10次无响应判定为断线
- 断线后自动重连 ESP8266 + MQTT Broker

### 8.3 待确认资产机制

新检测到的资产不会直接写入数据库，而是：
1. 加入内存中的待确认列表
2. 前端轮询获取待确认列表
3. 用户确认后写入数据库
4. 用户拒绝时丢弃

### 8.4 FLASH 存储

STM32 内部 FLASH 分9个区域存储卡号：
- 每个区域 0x400 (1024) 字节
- 存储12字节的卡号数据
- 支持断电保存

---

## 九、总结

本系统是一个完整的物联网资产管理解决方案，涵盖了从硬件数据采集到云端存储再到用户交互的完整链路。系统采用 MQTT 作为设备通信协议，RESTful API 作为前后端通信接口，MySQL 作为数据持久化层，实现了资产的高效管理和实时监控。

**技术亮点**:
- RFID 自动识别，无需人工录入
- MQTT 轻量级协议，适合物联网场景
- 待确认机制，避免误录入
- 跨平台前端，支持多端部署
- 心跳保活和断线重连，保证通信可靠性