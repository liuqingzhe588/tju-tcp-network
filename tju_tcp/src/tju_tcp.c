#include "tju_tcp.h"

/*
创建 TCP socket 
初始化对应的结构体
设置初始状态为 CLOSED
*/
tju_tcp_t* tju_socket(){
    tju_tcp_t* sock = (tju_tcp_t*)malloc(sizeof(tju_tcp_t));
    sock->state = CLOSED;
    
    pthread_mutex_init(&(sock->send_lock), NULL);
    sock->sending_buf = NULL;
    sock->sending_len = 0;

    pthread_mutex_init(&(sock->recv_lock), NULL);
    sock->received_buf = NULL;
    sock->received_len = 0;
    
    if(pthread_cond_init(&sock->wait_cond, NULL) != 0){
        perror("ERROR condition variable not set\n");
        exit(-1);
    }

    sock->window.wnd_send = NULL;
    sock->window.wnd_recv = NULL;

    return sock;
}

/*
绑定监听的地址 包括ip和端口
*/
int tju_bind(tju_tcp_t* sock, tju_sock_addr bind_addr){
    sock->bind_addr = bind_addr;
    return 0;
}

/*
被动打开 监听bind的地址和端口
设置socket的状态为LISTEN
注册该socket到内核的监听socket哈希表
*/
int tju_listen(tju_tcp_t* sock){
    sock->state = LISTEN;
    int hashval = cal_hash(sock->bind_addr.ip, sock->bind_addr.port, 0, 0);
    listen_socks[hashval] = sock;
    return 0;
}

/*
接受连接
返回与客户端通信用的socket
这里返回的socket一定是已经完成3次握手建立了连接的socket
因为只要该函数返回, 用户就可以马上使用该socket进行send和recv

第一周实现：直接创建已连接的socket，不进行三次握手
第二周改进：从半连接队列中取出已完成三次握手的连接
*/
tju_tcp_t* tju_accept(tju_tcp_t* listen_sock){
    tju_tcp_t* new_conn = (tju_tcp_t*)malloc(sizeof(tju_tcp_t));
    memcpy(new_conn, listen_sock, sizeof(tju_tcp_t));

    tju_sock_addr local_addr, remote_addr;

    // 第一周：直接设置远端地址，不从SYN包中获取
    // 第二周：这里应该从接收到的SYN包中提取对端的IP和PORT
    remote_addr.ip = inet_network("172.17.0.2");  // 客户端IP
    remote_addr.port = 5678;  // 客户端端口

    local_addr.ip = listen_sock->bind_addr.ip;  // 服务端监听IP
    local_addr.port = listen_sock->bind_addr.port;  // 服务端监听端口

    new_conn->established_local_addr = local_addr;
    new_conn->established_remote_addr = remote_addr;

    // 第一周：直接设置为ESTABLISHED状态，跳过三次握手
    // 第二周：这里应该经过三次握手后才能修改状态为ESTABLISHED
    new_conn->state = ESTABLISHED;

    // 将新的conn放到内核建立连接的socket哈希表中
    int hashval = cal_hash(local_addr.ip, local_addr.port, remote_addr.ip, remote_addr.port);
    established_socks[hashval] = new_conn;

    // 第二周改进：accept应该从一个已完成连接的队列中取出socket
    // 队列为空时阻塞等待
    return new_conn;
}


/*
连接到服务端
该函数以一个socket为参数
调用函数前, 该socket还未建立连接
函数正常返回后, 该socket一定是已经完成了3次握手, 建立了连接
因为只要该函数返回, 用户就可以马上使用该socket进行send和recv

第一周实现：直接建立连接，不进行三次握手
第二周改进：实现完整的三次握手流程
*/
int tju_connect(tju_tcp_t* sock, tju_sock_addr target_addr){

    sock->established_remote_addr = target_addr;

    tju_sock_addr local_addr;
    local_addr.ip = inet_network("172.17.0.2");
    local_addr.port = 5678; // 连接方进行connect连接的时候 内核中是随机分配一个可用的端口
    sock->established_local_addr = local_addr;

    // 第一周：直接设置为ESTABLISHED状态，跳过三次握手
    // 第二周：这里需要发送SYN包，等待SYN-ACK，发送ACK，状态机转换
    sock->state = ESTABLISHED;

    // 将建立了连接的socket放入内核 已建立连接哈希表中
    int hashval = cal_hash(local_addr.ip, local_addr.port, target_addr.ip, target_addr.port);
    established_socks[hashval] = sock;

    return 0;
}

int tju_send(tju_tcp_t* sock, const void *buffer, int len){
    // 第一周：简单的发送实现，不考虑可靠传输
    char* data = malloc(len);
    memcpy(data, buffer, len);

    char* msg;
    uint32_t seq = 1000; // 第一周使用固定序列号，第二周会实现正确的序列号管理
    uint16_t plen = DEFAULT_HEADER_LEN + len;
    uint16_t advertised_window = 65535; // 第一周使用固定窗口大小

    // 创建数据包
    msg = create_packet_buf(sock->established_local_addr.port, sock->established_remote_addr.port,
                           seq, 0, DEFAULT_HEADER_LEN, plen, NO_FLAG, advertised_window, 0, data, len);

    // 发送到网络层
    sendToLayer3(msg, plen);

    free(data);
    free(msg);

    return len;
}
int tju_recv(tju_tcp_t* sock, void *buffer, int len){
    // 第一周：简单的接收实现
    // 等待数据到达（忙等待，第二周可以改进为条件变量）
    while(sock->received_len <= 0){
        // 阻塞等待数据
    }

    while(pthread_mutex_lock(&(sock->recv_lock)) != 0); // 加锁

    int read_len = 0;
    if (sock->received_len >= len){
        // 接收缓冲区有足够的数据，读取len长度
        read_len = len;
    }else{
        // 接收缓冲区数据不足，全部读出来
        read_len = sock->received_len;
    }

    // 复制数据到用户缓冲区
    memcpy(buffer, sock->received_buf, read_len);

    if(read_len < sock->received_len) {
        // 还有剩余数据，需要保留
        char* new_buf = malloc(sock->received_len - read_len);
        memcpy(new_buf, sock->received_buf + read_len, sock->received_len - read_len);
        free(sock->received_buf);
        sock->received_len -= read_len;
        sock->received_buf = new_buf;
    }else{
        // 数据全部读完，清空缓冲区
        free(sock->received_buf);
        sock->received_buf = NULL;
        sock->received_len = 0;
    }

    pthread_mutex_unlock(&(sock->recv_lock)); // 解锁

    return read_len;
}

int tju_handle_packet(tju_tcp_t* sock, char* pkt){
    // 第一周：简单的数据包处理，只处理数据接收
    // 第二周会添加：三次握手、四次挥手、ACK处理、重传等

    // 计算数据长度
    uint32_t data_len = get_plen(pkt) - DEFAULT_HEADER_LEN;

    // 如果有数据负载，将其放入接收缓冲区
    if(data_len > 0){
        while(pthread_mutex_lock(&(sock->recv_lock)) != 0); // 加锁

        if(sock->received_buf == NULL){
            // 接收缓冲区为空，直接分配
            sock->received_buf = malloc(data_len);
        }else {
            // 接收缓冲区有数据，扩展空间
            sock->received_buf = realloc(sock->received_buf, sock->received_len + data_len);
        }

        // 复制数据到接收缓冲区
        memcpy(sock->received_buf + sock->received_len, pkt + DEFAULT_HEADER_LEN, data_len);
        sock->received_len += data_len;

        pthread_mutex_unlock(&(sock->recv_lock)); // 解锁
    }

    return 0;
}

int tju_close (tju_tcp_t* sock){
    return 0;
}