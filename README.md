# C++ HTTP/1.1

Self-implemented HTTP/1.1 based on winsock2 and IOCP.

## Todo

- [x] IOCP Model
    - [x] Accept 
    - [x] Recv
    - [x] Send
    - [x] Close

- [ ] Server & Socket interface
    - [x] Server start
    - [x] Server close
    - [x] Server onAccept
    - [x] Server onClose
    - [x] Socket onRecv
    - [x] Socket onClose
    - [x] Socket Write
    - [x] Socket End
    - [ ] Socket Close


- [x] HTTP interface
    - [x] HttpReq
        - [x] HttpReq::headers, HttpReq::method, HttpReq::path
        - [x] HttpReq::clientAddr, other client infos
        - [x] HttpReq::OnData
        - [x] HttpReq::OnEnd
    - [x] HttpRes
        - [x] Status
        - [x] SetHeader
        - [x] Write

- [x] Refactor
    - [x] Make HttpReq, HttpRes and Scoket a class
        - [x] HttpReq
        - [x] HttpRes
        - [x] Socket
    - [x] privatefiy properties
        - [x] HttpReq
        - [x] HttpRes
        - [x] Socket
