import socket
import struct
import sys

def build_dns_query(domain):
    """构造DNS查询包"""
    # Transaction ID
    transaction_id = 0x1234
    
    # Flags: 标准查询
    flags = 0x0100
    
    # Questions: 1, Answers: 0, Authority: 0, Additional: 0
    questions = 1
    answers = 0
    authority = 0
    additional = 0
    
    # Header
    header = struct.pack('>HHHHHH', transaction_id, flags, questions, answers, authority, additional)
    
    # Question
    question = b''
    for part in domain.split('.'):
        question += struct.pack('B', len(part)) + part.encode()
    question += b'\x00'  # 结束符
    
    # Type A (1), Class IN (1)
    question += struct.pack('>HH', 1, 1)
    
    return header + question

def send_dns_query(server_ip, server_port, domain):
    """发送DNS查询"""
    print(f"\n========================================")
    print(f"测试DNS服务器: {server_ip}:{server_port}")
    print(f"查询域名: {domain}")
    print(f"========================================\n")
    
    # 创建UDP socket
    sock = socket.socket(socket.AF_INET, socket.SOCK_DGRAM)
    sock.settimeout(5)
    
    try:
        # 构造查询包
        query = build_dns_query(domain)
   
        print(f"[1] 发送DNS查询包 ({len(query)} 字节)...")
        print(f"十六进制: {query.hex()}")
        
        # 发送查询
        sock.sendto(query, (server_ip, server_port))
        print(f"✓ 查询已发送到 {server_ip}:{server_port}")
        
        # 接收响应
        print(f"\n[2] 等待响应...")
        response, addr = sock.recvfrom(512)
        
        print(f"✓ 收到响应 ({len(response)} 字节)")
        print(f"    来自: {addr}")
        print(f"    十六进制: {response.hex()}")
  
        # 简单解析响应
        if len(response) >= 12:
            transaction_id, flags, questions, answers = struct.unpack('>HHHH', response[:8])
          print(f"\n[3] 响应解析:")
            print(f" Transaction ID: 0x{transaction_id:04x}")
            print(f"    Answers: {answers}")
       
        if answers > 0:
            print(f"\n✓✓✓ DNS服务器工作正常！")
    return True
        else:
       print(f"\n⚠ 收到响应但没有答案")
 return False
  
    except socket.timeout:
    print(f"\n✗ 超时！服务器没有响应")
   print(f"\n可能原因：")
  print(f"  1. DNS服务器没有运行")
   print(f"  2. 防火墙阻止了连接")
print(f"  3. 服务器没有收到查询包")
     return False
        
    except Exception as e:
        print(f"\n✗ 错误: {e}")
        return False
     
    finally:
        sock.close()

if __name__ == "__main__":
# 测试本地DNS服务器
    server = "127.0.0.1"
    port = 53
    domain = "www.baidu.com"
  
    if len(sys.argv) > 1:
        domain = sys.argv[1]
    if len(sys.argv) > 2:
        port = int(sys.argv[2])
    
    result = send_dns_query(server, port, domain)
    
    if result:
        print(f"\n========================================")
  print(f"✓ 测试成功！DNS服务器正常工作")
        print(f"========================================")
        sys.exit(0)
    else:
        print(f"\n========================================")
        print(f"✗ 测试失败！请检查服务器")
     print(f"========================================")
    sys.exit(1)
