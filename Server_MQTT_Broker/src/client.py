"""
OneNET HTTP API 客户端

通过 OneNET 平台 HTTP API 查询/设置设备数据，不占用 MQTT 连接。
使用产品鉴权（products/{productId}），自动生成并刷新 token。

支持四种 API 方法：
  1. query_reported()  - 查询最近上报值（GET）   从平台缓存读取，无需设备在线
  2. query_real_time() - 实时下发查询（POST）    发送命令到设备获取当前值，需设备在线且订阅了下行主题
  3. query_history()   - 查询历史记录（GET）     按时间范围查询指定属性的历史数据
  4. set_property()    - 设置设备属性（POST）    下发属性设置命令到设备，需设备在线
"""
import base64
import hashlib
import hmac
import json
import time
import sys
from urllib.parse import quote

import requests

PRODUCT_ID = "y8PjkMrwSb"
PRODUCT_ACCESS_KEY = "xQ1dzZcBAVVb2yOVK4uxeYtDS9nNAyPPISP7FXvX04c="
DEVICE_NAME = "BOX"

TOKEN_EXPIRE_SECONDS = 3600
_token_cache: dict = {}
_last_token_refresh: float = 0


def _generate_token() -> str:
    version = "2022-05-01"
    method = "sha1"
    res = f"products/{PRODUCT_ID}"
    et = str(int(time.time()) + TOKEN_EXPIRE_SECONDS)

    key = base64.b64decode(PRODUCT_ACCESS_KEY)
    org = et + "\n" + method + "\n" + res + "\n" + version
    sign_b = hmac.new(key=key, msg=org.encode(), digestmod=hashlib.sha1)
    sign = base64.b64encode(sign_b.digest()).decode()
    sign = quote(sign, safe="")
    res_encoded = quote(res, safe="")

    return f"version={version}&res={res_encoded}&et={et}&method={method}&sign={sign}"


def _get_token() -> str:
    global _last_token_refresh
    now = time.time()
    if now - _last_token_refresh > TOKEN_EXPIRE_SECONDS - 60:
        _token_cache["token"] = _generate_token()
        _last_token_refresh = now
    return _token_cache["token"]


def _headers() -> dict:
    return {
        "Authorization": _get_token(),
        "Accept": "application/json, text/plain, */*",
        "User-Agent": "OneNET-Client/1.0",
    }


def query_reported() -> dict | None:
    """查询设备最近一次上报的属性（GET，从平台缓存读取）

    Returns:
            dict | None: 成功时返回完整响应字典，结构为 {"code": 0, "data": [...], "msg": "succ", "request_id": "..."}
                          data 列表中的每个元素包含 identifier, value, time, data_type, access_mode, name 等字段
                          失败时返回 None
    """
    url = (
        f"https://open.iot.10086.cn/apidebug/thingmodel/query-device-property"
        f"?product_id={PRODUCT_ID}&device_name={DEVICE_NAME}"
    )
    try:
        resp = requests.get(url, headers=_headers(), timeout=10)
        data = resp.json()
        if data.get("code") == 0:
            return data
        print(f"[错误] API 返回异常: {data.get('msg', 'unknown')}")
        return None
    except requests.exceptions.Timeout:
        print("[错误] 请求超时")
        return None
    except requests.exceptions.RequestException as e:
        print(f"[错误] 请求失败: {e}")
        return None
    except json.JSONDecodeError:
        print(f"[错误] 响应不是有效 JSON: {resp.text[:200]}")
        return None


def query_real_time(params: list[str] | None = None) -> dict | None:
    """实时下发命令到设备，获取当前属性值（POST，需设备在线）

    Args:
        params: 要查询的功能点标识列表，为 None 时查询全部预定义属性
                (temperature, ambient_temperature, pillbox_status, BatteryStatus,
                 CellNumber, PillRemain, TakePillState, latitude, longitude)

    Returns:
        dict | None: 成功时返回完整响应字典，结构为 {"code": 0, "data": {"属性名": 值, ...}, "msg": "succ", "request_id": "..."}
                     失败时返回 None
    """
    url = "https://iot-api.heclouds.com/thingmodel/query-device-property-detail"

    if params is None:
        params = [
            "temperature",
            "ambient_temperature",
            "pillbox_status",
            "BatteryStatus",
            "CellNumber",
            "PillRemain",
            "TakePillState",
            "latitude",
            "longitude",
        ]

    body = {
        "product_id": PRODUCT_ID,
        "device_name": DEVICE_NAME,
        "params": params,
    }

    headers = _headers()
    headers["Content-Type"] = "application/json"

    try:
        print(f"[下发命令] 查询属性: {params}")
        resp = requests.post(url, headers=headers, json=body, timeout=15)
        data = resp.json()
        if data.get("code") == 0:
            return data
        print(f"[错误] API 返回异常: {data.get('msg', 'unknown')}")
        return None
    except requests.exceptions.Timeout:
        print("[错误] 请求超时（设备可能不在线）")
        return None
    except requests.exceptions.RequestException as e:
        print(f"[错误] 请求失败: {e}")
        return None
    except json.JSONDecodeError:
        print(f"[错误] 响应不是有效 JSON: {resp.text[:200]}")
        return None


def query_history(
    identifier: str,
    start_time: int | None = None,
    end_time: int | None = None,
    sort: int = 2,
    offset: int = 0,
    limit: int = 10,
) -> dict | None:
    """查询设备属性历史记录（GET）

    Args:
        identifier: 属性功能点标识，如 "temperature"、"ambient_temperature"
        start_time: 查询起始时间，毫秒时间戳，默认为24小时前
        end_time: 查询结束时间，毫秒时间戳，默认为当前时间
        sort: 排序方式，1-正序（旧到新），2-倒序（新到旧）
        offset: 请求起始位置，用于分页
        limit: 每次请求记录数，范围 [1, 100]

    Returns:
        dict | None: 成功时返回完整响应字典，结构为 {"code": 0, "data": {"list": [...]}, "msg": "succ", "request_id": "..."}
                      data.list 中的每个元素包含 value（属性值）和 time（毫秒时间戳）
                      失败时返回 None
    """
    if end_time is None:
        end_time = int(time.time() * 1000)
    if start_time is None:
        start_time = end_time - 24 * 60 * 60 * 1000

    url = "https://iot-api.heclouds.com/thingmodel/query-device-property-history"
    params = {
        "product_id": PRODUCT_ID,
        "device_name": DEVICE_NAME,
        "identifier": identifier,
        "start_time": str(start_time),
        "end_time": str(end_time),
        "sort": str(sort),
        "offset": str(offset),
        "limit": str(limit),
    }

    try:
        resp = requests.get(url, headers=_headers(), params=params, timeout=10)
        data = resp.json()
        if data.get("code") == 0:
            return data
        print(f"[错误] API 返回异常: {data.get('msg', 'unknown')}")
        return None
    except requests.exceptions.Timeout:
        print("[错误] 请求超时")
        return None
    except requests.exceptions.RequestException as e:
        print(f"[错误] 请求失败: {e}")
        return None
    except json.JSONDecodeError:
        print(f"[错误] 响应不是有效 JSON: {resp.text[:200]}")
        return None


def set_property(params: dict) -> dict | None:
    """设置设备属性（POST，需设备在线且订阅了下行主题）

    下发物模型属性设置命令到设备，设备收到命令后执行并返回设置结果。

    Args:
        params: 要设置的属性键值对，如 {"switch": true, "temperature": 30.2}
                属性值必须符合物模型定义的数据类型（bool、int32、int64、float、double、string、enum、struct 等）

    Returns:
        dict | None: 成功时返回完整响应字典，结构为 {"code": 0, "data": {"id": "155", "code": 200, "msg": "success"}, "msg": "succ", "request_id": "..."}
                      data.id 为设备端回复消息id，data.code 为设备端响应码，data.msg 为设备端响应消息
                      失败时返回 None
    """
    url = "https://iot-api.heclouds.com/thingmodel/set-device-property"
    body = {
        "product_id": PRODUCT_ID,
        "device_name": DEVICE_NAME,
        "params": params,
    }

    headers = _headers()
    headers["Content-Type"] = "application/json"

    try:
        print(f"[下发命令] 设置属性: {json.dumps(params, ensure_ascii=False)}")
        resp = requests.post(url, headers=headers, json=body, timeout=15)
        data = resp.json()
        if data.get("code") == 0:
            return data
        print(f"[错误] API 返回异常: {data.get('msg', 'unknown')}")
        return None
    except requests.exceptions.Timeout:
        print("[错误] 请求超时（设备可能不在线）")
        return None
    except requests.exceptions.RequestException as e:
        print(f"[错误] 请求失败: {e}")
        return None
    except json.JSONDecodeError:
        print(f"[错误] 响应不是有效 JSON: {resp.text[:200]}")
        return None


def main():
    mode = "reported"
    interval = 5

    if len(sys.argv) > 1:
        arg = sys.argv[1]
        if arg in ("realtime", "live", "rt"):
            mode = "realtime"
        elif arg in ("reported", "cache"):
            mode = "reported"
        elif arg in ("history", "hist"):
            mode = "history"
        elif arg in ("set", "write"):
            mode = "set"

    if len(sys.argv) > 2:
        try:
            interval = int(sys.argv[2])
        except ValueError:
            pass

    mode_name = {
        "realtime": "实时下发（需设备在线）",
        "reported": "最近上报值（缓存）",
        "history": "历史记录查询",
        "set": "设置设备属性",
    }.get(mode, "最近上报值（缓存）")

    print("=" * 60)
    print(f"OneNET HTTP API - {mode_name}")
    print("=" * 60)
    print(f"产品ID: {PRODUCT_ID}")
    print(f"设备名: {DEVICE_NAME}")

    if mode == "history":
        identifier = sys.argv[2] if len(sys.argv) > 2 else "temperature"
        limit = int(sys.argv[3]) if len(sys.argv) > 3 else 10
        print(f"属性标识: {identifier}")
        print(f"记录条数: {limit}")
        print()
        result = query_history(identifier=identifier, limit=limit)
        records = []
        if result and result.get("data"):
            data = result["data"]
            if isinstance(data, dict) and "list" in data:
                records = data["list"]
            elif isinstance(data, list):
                records = data
        if records:
            print(f"共 {len(records)} 条记录:")
            for r in records:
                if isinstance(r, dict):
                    t = time.strftime("%Y-%m-%d %H:%M:%S", time.localtime(int(r["time"]) / 1000))
                    print(f"  [{t}] {identifier} = {r.get('value', '-')}")
                else:
                    print(f"  {r}")
        else:
            print("没有找到记录")
        return

    if mode == "set":
        if len(sys.argv) > 2:
            try:
                set_params = json.loads(sys.argv[2])
            except json.JSONDecodeError:
                print("[错误] 属性参数必须是有效的 JSON 对象，如 '{\"CellNumber\": 7}'")
                return
        else:
            print("用法: python client.py set '{\"属性名\": 值, ...}'")
            print("示例: python client.py set '{\"CellNumber\": 7}'")
            return

        print(f"产品ID: {PRODUCT_ID}")
        print(f"设备名: {DEVICE_NAME}")
        print(f"设置内容: {json.dumps(set_params, ensure_ascii=False)}")
        print()

        result = set_property(params=set_params)
        if result and result.get("data"):
            dev_data = result["data"]
            print("设备回复:")
            print(f"  消息ID: {dev_data.get('id', '-')}")
            print(f"  响应码: {dev_data.get('code', '-')}")
            print(f"  响应消息: {dev_data.get('msg', '-')}")
        else:
            print("设置失败，请检查设备是否在线且订阅了下行主题")
        return

    print(f"查询间隔: {interval}秒")
    print()

    try:
        while True:
            if mode == "realtime":
                result = query_real_time()
            else:
                result = query_reported()

            if result and result.get("data"):
                data = result["data"]
                ts = time.strftime("%H:%M:%S")
                if mode == "reported":
                    properties = data
                    print(f"[{ts}] 设备属性（缓存）:")
                    for prop in properties:
                        if "value" in prop:
                            v_time = ""
                            if "time" in prop:
                                v_time = time.strftime(
                                    " %H:%M:%S",
                                    time.localtime(prop["time"] / 1000),
                                )
                            print(f"  {prop['identifier']}: {prop['value']}{v_time}")
                else:
                    print(f"[{ts}] 设备属性（实时）:")
                    for k, v in data.items():
                        print(f"  {k}: {v}")

            time.sleep(interval)

    except KeyboardInterrupt:
        print("\n\n正在退出...")


if __name__ == "__main__":
    main()
