#!/usr/bin/env python3
import asyncio
import websockets
import json
import os

class PaaSServer:
    def __init__(self):
        self.sequence_counter = 1000
        self.responses_received = 0
        
    async def handle_client(self, websocket):
        print(f"客户端已连接: {websocket.remote_address}")
        
        try:
            # 发送测试请求
            await asyncio.sleep(1)  # 等待客户端准备好
            
            # 测试读取文件功能
            test_file_path = "test_file.txt"
            
            # 创建测试文件
            with open(test_file_path, 'w', encoding='utf-8') as f:
                f.write("这是测试文件的内容\n测试中文字符\n123456")
            
            # 1. 发送成功的读取文件请求
            success_request = {
                "n": "rf",
                "p": os.path.abspath(test_file_path),
                "s": self.sequence_counter
            }
            
            print(f"发送成功测试请求: {json.dumps(success_request, ensure_ascii=False)}")
            await websocket.send(json.dumps(success_request))
            self.sequence_counter += 1
            
            # 2. 发送失败的读取文件请求（文件不存在）
            await asyncio.sleep(0.5)  # 稍微延迟一下
            
            error_request = {
                "n": "rf", 
                "p": "/nonexistent/file.txt",
                "s": self.sequence_counter
            }
            
            print(f"发送失败测试请求: {json.dumps(error_request)}")
            await websocket.send(json.dumps(error_request))
            self.sequence_counter += 1
            
            # 等待并处理响应（只处理两个响应）
            async for message in websocket:
                print(f"收到响应: {message}")
                
                try:
                    response = json.loads(message)
                    
                    if response.get("c") == 200:
                        print("✅ 请求成功处理")
                        if response.get("r") and "content" in response["r"]:
                            print(f"文件内容: {response['r']['content']}")
                    else:
                        print(f"❌ 请求失败: {response.get('e')}")
                        print(f"错误原因: {response.get('er')}")
                        
                    self.responses_received += 1
                    
                    # 收到两个响应后退出循环
                    if self.responses_received >= 2:
                        print("✅ 所有测试完成！")
                        break
                        
                except json.JSONDecodeError:
                    print(f"❌ 无效的 JSON 响应: {message}")
                
        except websockets.exceptions.ConnectionClosed:
            print("客户端连接已关闭")
        except Exception as e:
            print(f"处理客户端时发生错误: {e}")

async def main():
    server = PaaSServer()
    
    print("启动 PaaS 测试服务器，监听 localhost:8765")
    print("等待客户端连接...")
    
    async with websockets.serve(server.handle_client, "localhost", 8765):
        await asyncio.Future()  # 永远运行

if __name__ == "__main__":
    try:
        asyncio.run(main())
    except KeyboardInterrupt:
        print("\n服务器已停止")
