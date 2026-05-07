package com.example.stm32_box.data.mqtt

import android.content.Context
import android.util.Log
import kotlinx.coroutines.flow.MutableStateFlow
import kotlinx.coroutines.flow.StateFlow
import kotlinx.coroutines.flow.asStateFlow
import org.eclipse.paho.client.mqttv3.IMqttDeliveryToken
import org.eclipse.paho.client.mqttv3.MqttCallback
import org.eclipse.paho.client.mqttv3.MqttClient
import org.eclipse.paho.client.mqttv3.MqttConnectOptions
import org.eclipse.paho.client.mqttv3.MqttMessage
import org.eclipse.paho.client.mqttv3.MqttPersistenceException
import org.eclipse.paho.client.mqttv3.persist.MemoryPersistence
import org.json.JSONObject

data class MqttConnectionState(
    val isConnected: Boolean = false,
    val isConnecting: Boolean = false,
    val errorMessage: String? = null
)

data class DeviceTelemetry(
    val objectTemp: Float? = null,
    val ambientTemp: Float? = null,
    val latitude: Double? = null,
    val longitude: Double? = null,
    val status: String? = null,
    val timestamp: Long = System.currentTimeMillis()
)

class MqttService private constructor(private val context: Context) {

    private var client: MqttClient? = null

    private val _connectionState = MutableStateFlow(MqttConnectionState())
    val connectionState: StateFlow<MqttConnectionState> = _connectionState.asStateFlow()

    private val _telemetry = MutableStateFlow(DeviceTelemetry())
    val telemetry: StateFlow<DeviceTelemetry> = _telemetry.asStateFlow()

    private val _incomingMessage = MutableStateFlow<String?>(null)
    val incomingMessage: StateFlow<String?> = _incomingMessage.asStateFlow()

    private val MAX_HISTORY_SIZE = 500
    private val _telemetryHistory = MutableStateFlow<List<DeviceTelemetry>>(emptyList())
    val telemetryHistory: StateFlow<List<DeviceTelemetry>> = _telemetryHistory.asStateFlow()

    companion object {
        private const val TAG = "MqttService"
        private const val BROKER_URL = "tcp://mqtts.heclouds.com:1883"
        private val CLIENT_ID = "Android_BOX_${System.currentTimeMillis()}"
        private const val QOS = 0

        private const val PRODUCT_ID = "y8PjkMrwSb"
        private const val DEVICE_NAME = "BOX"

        // Topic 定义
        private const val SYS_TOPIC_PREFIX = "\$sys"
        private const val SYS_TOPIC_PROP_POST = "$SYS_TOPIC_PREFIX/$PRODUCT_ID/$DEVICE_NAME/thing/property/post"
        private const val SYS_TOPIC_PROP_POST_REPLY = "$SYS_TOPIC_PREFIX/$PRODUCT_ID/$DEVICE_NAME/thing/property/post/reply"
        private const val SYS_TOPIC_PROP_SET = "$SYS_TOPIC_PREFIX/$PRODUCT_ID/$DEVICE_NAME/thing/property/set"

        // OneNET 认证信息 - 从设备详情获取
        // 用户名=产品ID, 密码=token
        private const val MQTT_USERNAME = "y8PjkMrwSb"
        private const val MQTT_PASSWORD = "version=2018-10-31&res=products%2Fy8PjkMrwSb%2Fdevices%2FBOX&et=1787378552&method=md5&sign=Uw4%2Fsd8ngftzYB1xYZzHtQ%3D%3D"

        @Volatile
        private var instance: MqttService? = null

        fun getInstance(context: Context): MqttService {
            return instance ?: synchronized(this) {
                instance ?: MqttService(context.applicationContext).also { instance = it }
            }
        }
    }

    private val mqttCallback = object : MqttCallback {
        override fun connectionLost(cause: Throwable?) {
            Log.e(TAG, "连接丢失: ${cause?.message}")
            _connectionState.value = MqttConnectionState(
                isConnected = false,
                isConnecting = false,
                errorMessage = cause?.message ?: "连接丢失"
            )
            Log.d(TAG, "连接状态更新: isConnected=false, isConnecting=false, error=${cause?.message}")
        }

        override fun messageArrived(topic: String?, message: MqttMessage?) {
            val payload = message?.payload?.let { String(it) }
            Log.d(TAG, "消息到达 - Topic: $topic, Payload: $payload")

            _incomingMessage.value = payload

            payload?.let { parseTelemetry(it) }
        }

        override fun deliveryComplete(token: IMqttDeliveryToken?) {
            Log.d(TAG, "消息投递完成")
        }
    }

    private fun parseTelemetry(json: String) {
        try {
            Log.d(TAG, "开始解析遥测数据 JSON: $json")
            val jsonObj = JSONObject(json)
            val params = jsonObj.optJSONObject("params") ?: run {
                Log.w(TAG, "JSON 中没有 params 字段")
                return
            }

            val current = _telemetry.value

            val newTelemetry = DeviceTelemetry(
                objectTemp = if (params.has("temperature")) params.getDouble("temperature").toFloat() else current.objectTemp,
                ambientTemp = if (params.has("ambient_temperature")) params.getDouble("ambient_temperature").toFloat() else current.ambientTemp,
                latitude = if (params.has("latitude")) params.getDouble("latitude") else current.latitude,
                longitude = if (params.has("longitude")) params.getDouble("longitude") else current.longitude,
                status = if (params.has("status")) params.getString("status") else current.status,
                timestamp = System.currentTimeMillis()
            )

            Log.d(TAG, "解析后遥测数据: 物体温度=${newTelemetry.objectTemp}°C, 环境温度=${newTelemetry.ambientTemp}°C, 经度=${newTelemetry.longitude}, 纬度=${newTelemetry.latitude}, 状态=${newTelemetry.status}")
            _telemetry.value = newTelemetry

            // 添加到历史记录
            val currentHistory = _telemetryHistory.value.toMutableList()
            currentHistory.add(newTelemetry)
            if (currentHistory.size > MAX_HISTORY_SIZE) {
                currentHistory.removeAt(0)
            }
            _telemetryHistory.value = currentHistory
        } catch (e: Exception) {
            Log.e(TAG, "解析遥测数据异常: ${e.message}")
        }
    }

    fun connect() {
        if (_connectionState.value.isConnecting) {
            Log.w(TAG, "正在连接中，忽略重复连接请求")
            return
        }

        Log.d(TAG, "开始连接 MQTT Broker: $BROKER_URL")
        _connectionState.value = MqttConnectionState(isConnecting = true)

        try {
            Log.d(TAG, "创建 MQTT Client, ClientID: $CLIENT_ID")

            // 先清理旧连接
            try {
                client?.disconnect()
            } catch (e: Exception) {
                // 忽略断开错误
            }
            client = null

            client = MqttClient(BROKER_URL, CLIENT_ID, MemoryPersistence())
            client?.setCallback(mqttCallback)
            client?.setTimeToWait(15000)  // 15秒超时

            // 使用标准 MqttConnectOptions
            val connectOptions = MqttConnectOptions().apply {
                isCleanSession = true
                connectionTimeout = 20
                keepAliveInterval = 120
                isAutomaticReconnect = false
                userName = MQTT_USERNAME
                password = MQTT_PASSWORD.toCharArray()
            }

            Log.d(TAG, "正在连接... ClientID=$CLIENT_ID, 用户名: $MQTT_USERNAME")
            Log.d(TAG, "密码前20字符: ${MQTT_PASSWORD.take(20)}...")

            // 连接并处理异常
            try {
                client?.connect(connectOptions)
                Log.d(TAG, "connect() 返回, isConnected=${client?.isConnected}")
            } catch (e: org.eclipse.paho.client.mqttv3.MqttException) {
                Log.e(TAG, "MqttException: ${e.message}, reasonCode=${e.reasonCode}")
                // 打印更详细的错误信息
                when (e.reasonCode) {
                    0 -> Log.e(TAG, "连接成功")
                    1 -> Log.e(TAG, "连接被拒绝: 错误的用户名或密码")
                    2 -> Log.e(TAG, "连接被拒绝: 服务器不可用")
                    3 -> Log.e(TAG, "连接被拒绝: 用户名或密码错误")
                    4 -> Log.e(TAG, "连接被拒绝: 授权失败")
                    5 -> Log.e(TAG, "连接被拒绝: 未授权")
                    else -> Log.e(TAG, "连接被拒绝: 未知错误码 ${e.reasonCode}")
                }
                throw e  // 重新抛出让外层处理
            }

            // 检查连接是否成功
            if (client?.isConnected == true) {
                _connectionState.value = MqttConnectionState(isConnected = true)
                Log.i(TAG, "MQTT 连接成功!")

                Log.d(TAG, "订阅 Topic: $SYS_TOPIC_PROP_POST")
                subscribe(SYS_TOPIC_PROP_POST)
                Log.d(TAG, "订阅 Topic: $SYS_TOPIC_PROP_POST_REPLY")
                subscribe(SYS_TOPIC_PROP_POST_REPLY)
            } else {
                Log.e(TAG, "MQTT 连接失败: 未连接")
                _connectionState.value = MqttConnectionState(
                    isConnected = false,
                    isConnecting = false,
                    errorMessage = "连接失败"
                )
            }

        } catch (e: org.eclipse.paho.client.mqttv3.MqttException) {
            Log.e(TAG, "MQTT 异常: ${e.message}, 错误码: ${e.reasonCode}", e)
            _connectionState.value = MqttConnectionState(
                isConnected = false,
                isConnecting = false,
                errorMessage = "${e.message} (code: ${e.reasonCode})"
            )
        } catch (e: Exception) {
            Log.e(TAG, "MQTT 连接失败: ${e.message}", e)
            _connectionState.value = MqttConnectionState(
                isConnected = false,
                isConnecting = false,
                errorMessage = e.message
            )
        }
    }

    fun disconnect() {
        Log.d(TAG, "用户请求断开 MQTT 连接")
        try {
            client?.disconnect()
            _connectionState.value = MqttConnectionState(isConnected = false)
            Log.i(TAG, "MQTT 连接已断开")
        } catch (e: Exception) {
            Log.e(TAG, "断开连接异常: ${e.message}")
        }
    }

    fun subscribe(topic: String, qos: Int = QOS) {
        try {
            client?.subscribe(topic, qos)
            Log.i(TAG, "订阅成功: $topic (QOS=$qos)")
        } catch (e: Exception) {
            Log.e(TAG, "订阅失败: ${e.message}")
        }
    }

    fun publish(topic: String, payload: String, qos: Int = QOS) {
        try {
            val message = MqttMessage(payload.toByteArray()).apply {
                this.qos = qos
            }
            client?.publish(topic, message)
            Log.i(TAG, "发布成功 - Topic: $topic, Payload: $payload")
        } catch (e: Exception) {
            Log.e(TAG, "发布失败: ${e.message}")
        }
    }

    fun publishCommand(command: String) {
        val topic = SYS_TOPIC_PROP_SET
        val payload = """{"id":"cmd_${System.currentTimeMillis()}","version":"1.0","params":$command}"""
        Log.d(TAG, "发送命令 - Topic: $topic, Payload: $payload")
        publish(topic, payload)
    }

    fun publishBoxControl(action: String) {
        val command = """{"box_control":{"action":"$action"}}"""
        Log.d(TAG, "发送药箱控制命令: $action")
        publishCommand(command)
    }

    fun publishTimeSync(timestamp: Long) {
        val command = """{"time_sync":{"timestamp":$timestamp}}"""
        Log.d(TAG, "发送时间同步命令: $timestamp")
        publishCommand(command)
    }

    fun publishMedicineTime(timeStr: String) {
        val command = """{"MedicineTime":"$timeStr"}"""
        Log.d(TAG, "发送用药时间: $timeStr")
        publishCommand(command)
    }

    /**
     * 注入模拟遥测数据，用于 UI 调试展示
     */
    fun injectTelemetry(data: DeviceTelemetry) {
        _telemetry.value = data
        _incomingMessage.value = "模拟数据已更新"

        val currentHistory = _telemetryHistory.value.toMutableList()
        currentHistory.add(data)
        if (currentHistory.size > MAX_HISTORY_SIZE) {
            currentHistory.removeAt(0)
        }
        _telemetryHistory.value = currentHistory

        Log.d(TAG, "注入模拟遥测数据: $data")
    }
}