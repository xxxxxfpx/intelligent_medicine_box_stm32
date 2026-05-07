"""
测试设置下一次吃药时间 - 通过 OneNET HTTP API 设置 NextEat

NextEat 的 data_type 为 date，以毫秒时间戳格式传输。
"""
import time
from client import set_property

TEST_TIMES = [
    "08:00",
    "12:30",
    "18:00",
    "22:00",
]


def time_to_timestamp_ms(hour, minute, second=0):
    """获取今天指定时分的 UTC+8 毫秒时间戳"""
    now = time.localtime()
    import datetime
    dt = datetime.datetime(now.tm_year, now.tm_mon, now.tm_mday, hour, minute, second)
    return int(dt.timestamp() * 1000)


print("=" * 60)
print("测试 OneNET 设置下一次吃药时间 (NextEat)")
print("=" * 60)
print()

for t in TEST_TIMES:
    parts = t.split(":")
    h, m = int(parts[0]), int(parts[1])
    ts = time_to_timestamp_ms(h, m)
    local_time = time.strftime("%H:%M:%S", time.localtime(ts / 1000))

    print(f"--- 设置 NextEat = {t} (timestamp: {ts}) ---")
    result = set_property({"NextEat": ts})
    if result and result.get("data"):
        d = result["data"]
        print(f"  消息ID: {d.get('id', '-')}")
        print(f"  响应码: {d.get('code', '-')}")
        print(f"  响应消息: {d.get('msg', '-')}")
        if d.get("code") == 200:
            print(f"  [OK] 设备已接收下次吃药时间: {t}")
        else:
            print(f"  [WARN] 设备响应异常")
    else:
        print(f"  [FAIL] 设置失败（设备可能不在线）")
    print()
