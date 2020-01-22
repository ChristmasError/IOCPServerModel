# IOCPServerModel
一个基于完成端口的设计的IOCPServerModel服务器基类：
WinSock项目将windows socket库部分库函数进行封装，方便在IOCPModel中管理socket.  
PER_IO_CONTEXT结构体封装了用于每一个重叠操作所需的参数.   
PER_SOCKET_CONTEXT结构体封装了每一个客户端的信息.   
IO_CONTEXT_POOL为I/O Context的缓存池.  
SOCKET_CONTEXT_POOL为管理连入客户端socket的内存池（使用MemoryPool内存池设计，内存池项目在MemroyPool文件夹）.    

使用IOCPModel必须将该基类进行派生并重载回调函数，Test_HTTPServer_IOCP项目中就是IOCPServerModel的一个简单的使用例子，对HTTP的GET请求实现简单的响应。  
