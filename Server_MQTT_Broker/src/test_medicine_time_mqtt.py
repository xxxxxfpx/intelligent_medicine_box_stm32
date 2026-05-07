"""
测试设置下一次吃药时间 - 通过 MQTT 直接发布（产品级鉴权）

使用产品级 access key 生成 token 连接 MQTT Broker。
"""
import json
import random
import time
import base64
import hashlib
import hmac
from urllib.parse import quote

import paho.mqtt.client as mqtt

PRODUCT_ID = "y8PjkMrwSb"
PRODUCT_ACCESS_KEY = "xQ1dzZcBAVVb2yOVK4uxeYtDS9nNAyPPISP7FXvX04c="
DEVICE_NAME = "BOX"
BROKER_HOST = "mqtts.heclouds.com"
BROKER_PORT = 1883

PUB_TOPIC = f"$sys/{PRODUCT_ID}/{DEVICE_NAME}/thing/property/set"

TEST_TIMES = [
    "08:00",
    "12:30",
    "18:00",
    "22:00",
]


def generate_token():
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


def on_connect(client, userdata, flags, rc):
    if rc == 0:
        print("[MQTT] 已连接到 OneNET Broker")
    else:
        reasons = {1: "unacceptable protocol version", 2: "identifier rejected",
                   3: "server unavailable", 4: "bad username/password", 5: "not authorized"}
        print(f"[MQTT] 连接失败，返回码 {rc}: {reasons.get(rc, 'unknown')}")


msg_id_counter = 100


def send_medicine_time(client, time_str):
    global msg_id_counter
    msg_id_counter += 1

    payload = json.dumps({
        "id": str(msg_id_counter),
        "version": "1.0",
        "params": {
            "MedicineTime": time_str,
        },
    })

    print(f"\n--- 发送 MedicineTime = {time_str} ---")
    print(f"  Topic: {PUB_TOPIC}")
    print(f"  Payload: {payload}")
    info = client.publish(PUB_TOPIC, payload, qos=0)
    info.wait_for_publish(5)
    if info.is_published():
        print(f"  [OK] 发送成功 (mid={info.mid})")
    else:
        print(f"  [FAIL] 发送失败")
    time.sleep(1)


def main():
    token = generate_token()
    client_id = f"TestApp_{random.randint(10000, 99999)}"

    print("=" * 60)
    print("测试 MedicineTime 下行 - MQTT 直接发布")
    print("=" * 60)
    print(f"Broker : {BROKER_HOST}:{BROKER_PORT}")
    print(f"Topic  : {PUB_TOPIC}")
    print(f"Client : {client_id}")
    print()

    client = mqtt.Client(client_id=client_id, protocol=mqtt.MQTTv311)
    client.on_connect = on_connect
    client.username_pw_set(PRODUCT_ID, token)

    try:
        print("正在连接 OneNET Broker（产品级鉴权）...")
        client.connect(BROKER_HOST, BROKER_PORT, keepalive=60)
        client.loop_start()
        time.sleep(2)

        if client.is_connected():
            for t in TEST_TIMES:
                send_medicine_time(client, t)

            print(f"\n{'=' * 60}")
            print(f"测试完成！共发送 {len(TEST_TIMES)} 条消息")
            print("请检查设备串口日志确认接收结果")
            print(f"{'=' * 60}")
        else:
            print("\n[FAIL] 无法连接到 Broker，请检查网络和鉴权信息")

    except Exception as e:
        print(f"\n[ERROR] {e}")
    finally:
        client.loop_stop()
        client.disconnect()


if __name__ == "__main__":
    main()
