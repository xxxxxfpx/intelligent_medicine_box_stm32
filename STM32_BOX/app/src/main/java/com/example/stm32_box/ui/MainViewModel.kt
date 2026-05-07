package com.example.stm32_box.ui

import android.app.Application
import androidx.lifecycle.AndroidViewModel
import androidx.lifecycle.viewModelScope
import com.example.stm32_box.data.mqtt.DeviceTelemetry
import com.example.stm32_box.data.mqtt.MqttConnectionState
import com.example.stm32_box.data.repository.DeviceRepository
import kotlinx.coroutines.Job
import kotlinx.coroutines.delay
import kotlinx.coroutines.flow.MutableStateFlow
import kotlinx.coroutines.flow.StateFlow
import kotlinx.coroutines.flow.asStateFlow
import kotlinx.coroutines.launch
import kotlin.math.sin

data class UiState(
    val connectionState: MqttConnectionState = MqttConnectionState(),
    val telemetry: DeviceTelemetry = DeviceTelemetry(),
    val lastMessage: String? = null
)

class MainViewModel(application: Application) : AndroidViewModel(application) {

    private val repository = DeviceRepository(application)

    val connectionState: StateFlow<MqttConnectionState> = repository.connectionState

    val telemetry: StateFlow<DeviceTelemetry> = repository.telemetry

    val telemetryHistory: StateFlow<List<DeviceTelemetry>> = repository.telemetryHistory

    private val _lastMessage = MutableStateFlow<String?>(null)
    val lastMessage: StateFlow<String?> = _lastMessage.asStateFlow()

    private var mockDataJob: Job? = null

    private val fakeMessages = listOf(
        """{"id":"msg_001","params":{"temperature":25.3,"ambient_temperature":23.1,"humidity":58,"status":"正常运行"}}""",
        """{"id":"msg_002","params":{"temperature":25.8,"ambient_temperature":23.4,"humidity":56,"status":"正常运行"}}""",
        """{"id":"msg_003","params":{"temperature":26.1,"ambient_temperature":23.2,"humidity":59,"status":"取药中","action":"open_door"}}""",
        """{"id":"msg_004","params":{"temperature":24.9,"ambient_temperature":22.8,"humidity":60,"status":"正常运行"}}""",
        """{"id":"msg_005","params":{"temperature":25.6,"ambient_temperature":23.5,"humidity":55,"status":"药箱门已关闭","action":"close_door"}}""",
        """{"id":"msg_006","params":{"temperature":26.3,"ambient_temperature":23.7,"humidity":57,"status":"药量充足","remain":85}}""",
        """{"id":"msg_007","params":{"temperature":25.1,"ambient_temperature":23.0,"humidity":61,"status":"下次用药: 14:30","next_eat":"14:30"}}""",
        """{"id":"msg_008","params":{"temperature":24.7,"ambient_temperature":22.6,"humidity":62,"status":"正常运行"}}""",
    )

    init {
        viewModelScope.launch {
            repository.incomingMessage.collect { msg ->
                if (msg != null) _lastMessage.value = msg
            }
        }
    }

    fun connect() {
        repository.connect()
    }

    fun disconnect() {
        repository.disconnect()
    }

    fun queryStatus() {
        repository.queryStatus()
    }

    fun sendBoxOpen() {
        repository.sendBoxOpen()
    }

    fun sendBoxClose() {
        repository.sendBoxClose()
    }

    fun sendTimeSync(timestamp: Long) {
        repository.sendTimeSync(timestamp)
    }

    fun sendMedicineTime(timeStr: String) {
        repository.sendMedicineTime(timeStr)
    }

    /**
     * 启动持续模拟数据流
     *
     * 温度模拟策略（三段式，更贴近真实物理变化）:
     *   1. 正弦波动 — 模拟昼夜/空调周期 (±1.5°C / ±1.0°C)
     *   2. 分段偏移 — 每 5~10 秒整体偏移一个随机值（模拟开门/关窗/人员走动）
     *   3. 逐点噪声 — 每个数据点在上述基础上再叠加随机抖动
     */
    fun startMockDataStream() {
        stopMockDataStream()

        repository.setMockConnected(true)

        mockDataJob = viewModelScope.launch {
            val now = System.currentTimeMillis()
            val seedCount = 1200

            // ---- 播种 1200 条历史 ----
            val seedPoints = mutableListOf<DeviceTelemetry>()
            var segOffset = 0.0f
            var segTimer = 0
            var segDuration = 5 + (Math.random() * 6).toInt()

            for (i in 0 until seedCount) {
                segTimer++
                if (segTimer >= segDuration) {
                    segOffset = nextSegmentOffset(segOffset)
                    segTimer = 0
                    segDuration = 5 + (Math.random() * 6).toInt()
                }
                seedPoints.add(
                    generateMockPoint(i.toDouble(), now - (seedCount - i) * 1000L, segOffset)
                )
            }
            repository.seedMockHistory(seedPoints)

            // ---- 持续生成 ----
            var phase = seedCount.toDouble()
            var liveSegOffset = segOffset
            var liveSegTimer = 0
            var liveSegDuration = 5 + (Math.random() * 6).toInt()
            var msgIndex = 0
            var msgCountdown = 3

            while (true) {
                delay(1000)

                liveSegTimer++
                if (liveSegTimer >= liveSegDuration) {
                    liveSegOffset = nextSegmentOffset(liveSegOffset)
                    liveSegTimer = 0
                    liveSegDuration = 5 + (Math.random() * 6).toInt()
                }

                val data = generateMockPoint(phase, System.currentTimeMillis(), liveSegOffset)
                repository.injectMockData(data)

                msgCountdown--
                if (msgCountdown <= 0) {
                    _lastMessage.value = fakeMessages[msgIndex % fakeMessages.size]
                    msgIndex++
                    msgCountdown = 3
                }
                phase += 1.0
            }
        }
    }

    /** 计算下一个分段偏移量：在上一次基础上随机变化 -1.0 ~ +1.0，钳位 ±3.0 */
    private fun nextSegmentOffset(current: Float): Float {
        return (current + ((Math.random() - 0.5) * 2.0).toFloat()).coerceIn(-3.0f, 3.0f)
    }

    fun stopMockDataStream() {
        mockDataJob?.cancel()
        mockDataJob = null
    }

    /**
     * 生成单个模拟数据点
     *
     * @param phase         相位（用于正弦波）
     * @param timestamp     时间戳
     * @param segmentOffset 当前 5~10s 时间段的整体偏移量
     */
    private fun generateMockPoint(phase: Double, timestamp: Long, segmentOffset: Float = 0f): DeviceTelemetry {
        // 1. 正弦波动
        val waveObj = (sin(phase * 0.12) * 1.5).toFloat()
        val waveAmb = (sin(phase * 0.08 + 2.0) * 0.3).toFloat()

        // 2. 逐点随机噪声
        val objNoise = ((Math.random() - 0.5) * 0.8).toFloat()
        val ambNoise = ((Math.random() - 0.5) * 0.15).toFloat()

        // GPS 漂移
        val waveLat = (sin(phase * 0.02) * 0.002)
        val waveLng = (sin(phase * 0.025 + 1.0) * 0.002)

        val statusIndex = ((phase / 30).toInt()) % 4
        val status = if (statusIndex == 1) "取药中" else "正常运行"

        return DeviceTelemetry(
            objectTemp = 25.5f + waveObj + segmentOffset + objNoise,
            ambientTemp = 23.0f + waveAmb + (segmentOffset * 0.1f) + ambNoise,
            latitude = 28.544 + waveLat + (Math.random() - 0.5) * 0.0005,
            longitude = 112.378 + waveLng + (Math.random() - 0.5) * 0.0005,
            status = status,
            timestamp = timestamp
        )
    }

    fun loadMockData() {
        startMockDataStream()
    }
}
