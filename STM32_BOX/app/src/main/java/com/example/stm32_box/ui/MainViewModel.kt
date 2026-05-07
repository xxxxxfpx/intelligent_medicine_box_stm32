package com.example.stm32_box.ui

import android.app.Application
import androidx.lifecycle.AndroidViewModel
import androidx.lifecycle.viewModelScope
import com.example.stm32_box.data.mqtt.DeviceTelemetry
import com.example.stm32_box.data.mqtt.MqttConnectionState
import com.example.stm32_box.data.repository.DeviceRepository
import kotlinx.coroutines.flow.MutableStateFlow
import kotlinx.coroutines.flow.StateFlow
import kotlinx.coroutines.flow.asStateFlow
import kotlinx.coroutines.launch

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

    init {
        viewModelScope.launch {
            repository.incomingMessage.collect { msg ->
                _lastMessage.value = msg
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
     * 生成并注入模拟数据，用于离线调试 UI
     */
    fun loadMockData() {
        val now = System.currentTimeMillis()
        val mockData = DeviceTelemetry(
            objectTemp = 24.5f + (Math.random() * 3).toFloat(),
            ambientTemp = 23.0f + (Math.random() * 2).toFloat(),
            latitude = 31.2300 + Math.random() * 0.01,
            longitude = 121.4700 + Math.random() * 0.01,
            status = "正常运行",
            timestamp = now
        )
        repository.injectMockData(mockData)
        _lastMessage.value = """{"id":"${now}","params":{"temperature":${String.format("%.1f", mockData.objectTemp)},"ambient_temperature":${String.format("%.1f", mockData.ambientTemp)},"latitude":${mockData.latitude},"longitude":${mockData.longitude},"status":"正常运行"}}"""
    }
}
