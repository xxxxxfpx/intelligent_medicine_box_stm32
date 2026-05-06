"""
实时设备属性查询测试

测试 query-device-property-detail API，带超时和有限次重试。
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


def make_token() -> str:
    version = "2022-05-01"
    method = "sha1"
    res = f"products/{PRODUCT_ID}"
    et = str(int(time.time()) + 3600)
    key = base64.b64decode(PRODUCT_ACCESS_KEY)
    org = et + "\n" + method + "\n" + res + "\n" + version
    sign_b = hmac.new(key=key, msg=org.encode(), digestmod=hashlib.sha1)
    sign = base64.b64encode(sign_b.digest()).decode()
    sign = quote(sign, safe="")
    res_encoded = quote(res, safe="")
    return f"version={version}&res={res_encoded}&et={et}&method={method}&sign={sign}"


def main():
    headers = {
        "Authorization": make_token(),
        "Content-Type": "application/json",
        "Accept": "application/json",
    }

    body = {
        "product_id": PRODUCT_ID,
        "device_name": DEVICE_NAME,
        "params": [
            "temperature",
            "ambient_temperature",
            "pillbox_status",
            "latitude",
            "longitude",
        ],
    }

    max_retries = 5
    timeout = 10

    print("=" * 60)
    print("OneNET 实时设备属性查询测试")
    print("=" * 60)
    print(f"产品ID: {PRODUCT_ID}")
    print(f"设备名: {DEVICE_NAME}")
    print(f"查询属性: {body['params']}")
    print(f"超时时间: {timeout}秒")
    print(f"最大重试: {max_retries}次")
    print()

    for i in range(max_retries):
        print(f"--- 第 {i+1}/{max_retries} 次尝试 ---")

        try:
            resp = requests.post(
                "https://iot-api.heclouds.com/thingmodel/query-device-property-detail",
                headers=headers,
                json=body,
                timeout=timeout,
            )
            data = resp.json()
            print(json.dumps(data, ensure_ascii=False, indent=2))

            if data.get("code") == 0:
                print("\n✓ 实时查询成功！")
                sys.exit(0)
            else:
                msg = data.get("msg", "unknown")
                print(f"设备返回: {msg}")
        except requests.exceptions.Timeout:
            print(f"✗ 请求超时（超过{timeout}秒）")
        except requests.exceptions.RequestException as e:
            print(f"✗ 请求异常: {e}")
        except json.JSONDecodeError:
            print(f"✗ 响应不是有效 JSON: {resp.text[:200]}")

        if i < max_retries - 1:
            wait = 3
            print(f"等待{wait}秒后重试...")
            time.sleep(wait)

    print(f"\n✗ 重试 {max_retries} 次后仍未成功，设备可能不在线或API异常")
    sys.exit(1)


if __name__ == "__main__":
    main()
