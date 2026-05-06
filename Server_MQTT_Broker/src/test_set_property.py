"""
测试 set_property API - 设置设备属性
"""
from client import set_property

print("=" * 60)
print("测试 OneNET set-device-property API")
print("=" * 60)

print("\n--- 测试1: 设置 CellNumber=7 ---")
result = set_property({"CellNumber": 7})
if result and result.get("data"):
    d = result["data"]
    print(f"消息ID: {d.get('id', '-')}")
    print(f"响应码: {d.get('code', '-')}")
    print(f"响应消息: {d.get('msg', '-')}")
else:
    print("设置失败（设备可能不在线或未订阅下行主题）")

print("\n--- 测试2: 设置 PillRemain=3 ---")
result = set_property({"PillRemain": 3})
if result and result.get("data"):
    d = result["data"]
    print(f"消息ID: {d.get('id', '-')}")
    print(f"响应码: {d.get('code', '-')}")
    print(f"响应消息: {d.get('msg', '-')}")
else:
    print("设置失败（设备可能不在线或未订阅下行主题）")
