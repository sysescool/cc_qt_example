#!/usr/bin/env python3
import asyncio
import websockets
import json
import os

class PaaSServer:
    def __init__(self):
        self.sequence_counter = 1000
        self.responses_received = 0
        self.total_tests = 0
        
    async def handle_client(self, websocket):
        print(f"客户端已连接: {websocket.remote_address}")
        print("=" * 60)
        
        try:
            # 等待客户端准备好
            await asyncio.sleep(1)
            
            # 发送所有测试请求
            await self.send_all_test_requests(websocket)
            
            # 等待并处理所有响应
            async for message in websocket:
                await self.handle_response(message)
                
                # 收到所有响应后退出循环
                if self.responses_received >= self.total_tests:
                    print("=" * 60)
                    print("✅ 所有测试完成！")
                    break
                
        except websockets.exceptions.ConnectionClosed:
            print("客户端连接已关闭")
        except Exception as e:
            print(f"处理客户端时发生错误: {e}")

    async def send_all_test_requests(self, websocket):
        """发送所有测试请求"""
        
        # 1. 测试读取文件功能
        await self.test_read_file(websocket)
        
        # 2. 测试写入文件功能  
        await self.test_write_file(websocket)
        
        # 3. 测试列出目录功能
        await self.test_list_directory(websocket)
        
        # 4. 测试获取系统信息功能
        await self.test_get_system_info(websocket)
        
        print(f"📤 已发送 {self.total_tests} 个测试请求，等待响应...")
        print("-" * 60)

    async def test_read_file(self, websocket):
        """测试读取文件功能"""
        print("🔍 测试读取文件功能...")
        
        # 创建测试文件
        test_file_path = "test_file.txt"
        with open(test_file_path, 'w', encoding='utf-8') as f:
            f.write("这是测试文件的内容\n测试中文字符\n123456")
        
        # 1. 测试成功读取文件
        success_request = {
            "n": "rf",
            "p": os.path.abspath(test_file_path),
            "s": self.sequence_counter
        }
        print(f"  📄 发送读取文件请求: {success_request['p']}")
        await websocket.send(json.dumps(success_request))
        self.sequence_counter += 1
        self.total_tests += 1
        
        await asyncio.sleep(0.1)
        
        # 2. 测试读取不存在的文件
        error_request = {
            "n": "rf", 
            "p": "/nonexistent/file.txt",
            "s": self.sequence_counter
        }
        print(f"  📄 发送读取不存在文件请求: {error_request['p']}")
        await websocket.send(json.dumps(error_request))
        self.sequence_counter += 1
        self.total_tests += 1

    async def test_write_file(self, websocket):
        """测试写入文件功能"""
        print("✍️ 测试写入文件功能...")
        
        # 1. 测试写入新文件
        write_request = {
            "n": "wf",
            "p": {
                "path": os.path.abspath("test_write.txt"),
                "content": "这是通过WebSocket写入的内容\n第二行内容",
                "append": False
            },
            "s": self.sequence_counter
        }
        print(f"  ✏️ 发送写入新文件请求: {write_request['p']['path']}")
        await websocket.send(json.dumps(write_request))
        self.sequence_counter += 1
        self.total_tests += 1
        
        await asyncio.sleep(0.1)
        
        # 2. 测试追加到文件
        append_request = {
            "n": "wf",
            "p": {
                "path": os.path.abspath("test_write.txt"),
                "content": "\n追加的内容",
                "append": True
            },
            "s": self.sequence_counter
        }
        print(f"  ➕ 发送追加文件请求: {append_request['p']['path']}")
        await websocket.send(json.dumps(append_request))
        self.sequence_counter += 1
        self.total_tests += 1

    async def test_list_directory(self, websocket):
        """测试列出目录功能"""
        print("📁 测试列出目录功能...")
        
        # 1. 测试列出当前目录
        list_request = {
            "n": "ld",
            "p": {
                "path": os.path.abspath("."),
                "includeHidden": False
            },
            "s": self.sequence_counter
        }
        print(f"  📂 发送列出目录请求: {list_request['p']['path']}")
        await websocket.send(json.dumps(list_request))
        self.sequence_counter += 1
        self.total_tests += 1
        
        await asyncio.sleep(0.1)
        
        # 2. 测试列出不存在的目录
        error_list_request = {
            "n": "ld",
            "p": "/nonexistent/directory",
            "s": self.sequence_counter
        }
        print(f"  📂 发送列出不存在目录请求: {error_list_request['p']}")
        await websocket.send(json.dumps(error_list_request))
        self.sequence_counter += 1
        self.total_tests += 1

    async def test_get_system_info(self, websocket):
        """测试获取系统信息功能"""
        print("💻 测试获取系统信息功能...")
        
        # 1. 测试获取所有系统信息
        sysinfo_request = {
            "n": "gsi",
            "p": None,  # null payload 表示获取所有信息
            "s": self.sequence_counter
        }
        print(f"  🖥️ 发送获取系统信息请求")
        await websocket.send(json.dumps(sysinfo_request))
        self.sequence_counter += 1
        self.total_tests += 1
        
        await asyncio.sleep(0.1)
        
        # 2. 测试获取特定系统信息
        specific_sysinfo_request = {
            "n": "gsi",
            "p": ["os", "cpu"],  # 只获取操作系统和CPU信息
            "s": self.sequence_counter
        }
        print(f"  🖥️ 发送获取特定系统信息请求")
        await websocket.send(json.dumps(specific_sysinfo_request))
        self.sequence_counter += 1
        self.total_tests += 1

    async def handle_response(self, message):
        """处理收到的响应"""
        print(f"📨 收到响应: {message}")
        
        try:
            response = json.loads(message)
            sequence = response.get("s", "unknown")
            status_code = response.get("c", 0)
            
            if status_code == 200:
                print(f"✅ 请求 {sequence} 成功处理")
                result = response.get("r")
                if result:
                    self.print_result_details(result)
            else:
                print(f"❌ 请求 {sequence} 失败:")
                print(f"   错误: {response.get('e')}")
                print(f"   原因: {response.get('er')}")
                
            self.responses_received += 1
            print(f"   (已完成 {self.responses_received}/{self.total_tests} 个测试)")
            print("-" * 40)
                
        except json.JSONDecodeError:
            print(f"❌ 无效的 JSON 响应: {message}")

    def print_result_details(self, result):
        """打印响应结果的详细信息"""
        if isinstance(result, dict):
            # 读取文件响应
            if "content" in result:
                content = result["content"]
                print(f"   📄 文件内容: {repr(content[:100])}" + ("..." if len(content) > 100 else ""))
            
            # 写入文件响应
            elif "message" in result and "bytesWritten" in result:
                print(f"   ✅ {result['message']}")
                print(f"   📊 写入字节数: {result['bytesWritten']}")
            
            # 列出目录响应
            elif "files" in result:
                files = result["files"]
                print(f"   📁 目录包含 {len(files)} 个项目:")
                for i, file_info in enumerate(files[:5]):  # 只显示前5个
                    print(f"      {i+1}. {file_info['name']} ({file_info['type']}, {file_info['size']} bytes)")
                if len(files) > 5:
                    print(f"      ... 还有 {len(files) - 5} 个项目")
            
            # 系统信息响应
            elif "osName" in result:
                print(f"   🖥️ 系统信息:")
                print(f"      操作系统: {result.get('osName')} {result.get('osVersion')}")
                print(f"      CPU架构: {result.get('cpuInfo')}")
                print(f"      内存: {result.get('availableMemory', 0) // (1024**3)}GB / {result.get('totalMemory', 0) // (1024**3)}GB")
                print(f"      磁盘: {result.get('availableDisk', 0) // (1024**3)}GB / {result.get('totalDisk', 0) // (1024**3)}GB")
            
            else:
                print(f"   📋 结果: {result}")

async def main():
    server = PaaSServer()
    
    print("🚀 启动 PaaS 测试服务器")
    print("📡 监听地址: localhost:8765")
    print("⏳ 等待客户端连接...")
    print("=" * 60)
    
    async with websockets.serve(server.handle_client, "localhost", 8765):
        await asyncio.Future()  # 永远运行

if __name__ == "__main__":
    try:
        asyncio.run(main())
    except KeyboardInterrupt:
        print("\n🛑 服务器已停止")
