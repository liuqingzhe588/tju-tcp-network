# TJU TCP 网络实践项目

## 项目简介

天津大学计算机网络实践课程 - TCP 协议实现

本项目实现一个简化版的 TCP 协议栈，包括连接管理、可靠传输、流量控制和拥塞控制等功能。

## 开发环境

- **虚拟化平台**: VirtualBox + Vagrant
- **操作系统**: Ubuntu 20.04
- **网络配置**:
  - Server IP: 172.17.0.3
  - Client IP: 172.17.0.2
  - 网络延迟: 20ms
  - 带宽: 100Mbps

## 项目结构

```
tju_tcp/
├── inc/                    # 头文件目录
│   ├── global.h           # 全局定义和数据结构
│   ├── kernel.h           # 内核模拟相关
│   ├── tju_packet.h       # 数据包处理
│   └── tju_tcp.h          # TCP Socket API
├── src/                    # 源文件目录
│   ├── client.c           # 客户端测试程序
│   ├── server.c           # 服务端测试程序
│   ├── kernel.c           # 内核模拟实现
│   ├── tju_packet.c       # 数据包处理实现
│   ├── tju_tcp.c          # TCP 核心实现
│   └── Makefile           # 编译配置
```

## 第一周实现

### 实现内容

- ✅ 环境搭建完成
- ✅ 理解项目框架和数据结构
- ✅ 实现基础的 Socket API（tju_socket, tju_bind, tju_listen, tju_connect, tju_accept）
- ✅ 实现基础的数据收发功能（tju_send, tju_recv）
- ✅ 实现数据包的创建和解析
- ✅ 实现简单的数据包处理逻辑

### 第一周特点

**第一周不需要实现**：
- ❌ 三次握手和四次挥手（第二周实现）
- ❌ 可靠传输机制（重传、超时）
- ❌ 流量控制和拥塞控制
- ❌ 序列号和确认号管理

**第一周实现方式**：
- 直接设置连接为 ESTABLISHED 状态
- 使用固定的序列号
- 简单的数据收发，不考虑丢包和重传
- 能够在无丢包环境下正常传输数据

## 编译和运行

### 启动虚拟机

```bash
# 在项目根目录
vagrant up
```

### 编译代码

```bash
# SSH 登录到虚拟机
vagrant ssh server  # 或 vagrant ssh client

# 进入代码目录
cd /vagrant/tju_tcp/

# 编译
make
```

### 运行测试

```bash
# 在 server 虚拟机上运行服务端
vagrant ssh server
cd /vagrant/tju_tcp/
./server

# 在 client 虚拟机上运行客户端（等待 5 秒后启动）
vagrant ssh client
cd /vagrant/tju_tcp/
./client
```

### 预期输出

```
# Server 端
server recv hello world
server recv hello tju

# Client 端
client recv hello world
client recv hello tju
```

## 核心数据结构

### tju_tcp_t - TCP Socket 结构体

```c
typedef struct {
    int state;                              // TCP 状态
    tju_sock_addr bind_addr;                // 绑定的 IP 和端口
    tju_sock_addr established_local_addr;   // 本地地址
    tju_sock_addr established_remote_addr;  // 远端地址
    pthread_mutex_t send_lock;              // 发送锁
    char* sending_buf;                      // 发送缓冲区
    int sending_len;                        // 发送缓冲区长度
    pthread_mutex_t recv_lock;              // 接收锁
    char* received_buf;                     // 接收缓冲区
    int received_len;                       // 接收缓冲区长度
    pthread_cond_t wait_cond;               // 条件变量
    window_t window;                        // 发送和接收窗口
} tju_tcp_t;
```

### TCP 状态

```c
#define CLOSED 0
#define LISTEN 1
#define SYN_SENT 2
#define SYN_RECV 3
#define ESTABLISHED 4
#define FIN_WAIT_1 5
#define FIN_WAIT_2 6
#define CLOSE_WAIT 7
#define CLOSING 8
#define LAST_ACK 9
#define TIME_WAIT 10
```

## 开发计划

- [x] **第一周**: 环境搭建 + 基础框架实现
- [ ] **第二周**: 三次握手 + 四次挥手 + 可靠传输
- [ ] **第三周**: 拥塞控制（Reno 算法）
- [ ] **后续周**: 性能优化 + 高级功能

## 参考资料

- RFC 9293: Transmission Control Protocol (TCP)
- RFC 6298: Computing TCP's Retransmission Timer
- RFC 5681: TCP Congestion Control

## 作者

- 学号: 3024244048
- 姓名: 刘庆喆
- 课程: 计算机网络实践

## 许可

本项目为课程作业项目，仅供学习参考。
