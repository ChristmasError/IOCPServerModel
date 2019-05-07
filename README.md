# IOCP-HTTPServer
一个基于完成端口的设计的IOCPModel基类，使用该基类进行派生并重载回调函数，搭建HTTP服务器示例。    
PER_IO_CONTEXT结构体封装了用于每一个重叠操作所需的参数；  
PER_SOCKET_CONTEXT结构体封装了每一个客户端的信息；  
IOCPModel带有IO_CONTEXT缓存池，及SOCKET_CONTEXT缓存池（使用内存池设计）。  
HTTPServer使用IOCPModel派生搭建。
