package com.example.stm32_box.data.repository

import android.content.Context
import com.example.stm32_box.data.mqtt.DeviceTelemetry
import com.example.stm32_box.data.mqtt.MqttConnectionState
import com.example.stm32_box.data.mqtt.MqttService
import kotlinx.coroutines.flow.StateFlow

class DeviceRepository(context: Context) {

    private val mqttService = MqttService.getInstance(context)

    val connectionState: StateFlow<MqttConnectionState> = mqttService.connectionState

    val telemetry: StateFlow<DeviceTelemetry> = mqttService.telemetry

    val incomingMessage: StateFlow<String?> = mqttService.incomingMessage

    val telemetryHistory: StateFlow<List<DeviceTelemetry>> = mqttService.telemetryHistory

    fun connect() {
        mqttService.connect()
    }

    fun disconnect() {
        mqttService.disconnect()
    }

    fun sendCommand(command: String) {
        mqttService.publishCommand(command)
    }

    fun queryStatus() {
        sendCommand("""{"query_status":{}}""")
    }

    fun sendBoxOpen() {
        mqttService.publishBoxControl("open")
    }

    fun sendBoxClose() {
        mqttService.publishBoxControl("close")
    }

    fun sendTimeSync(timestamp: Long) {
        mqttService.publishTimeSync(timestamp)
    }

    fun sendMedicineTime(timeStr: String) {
        mqttService.publishMedicineTime(timeStr)
    }

    fun injectMockData(data: DeviceTelemetry) {
        mqttService.injectTelemetry(data)
    }

    fun setMockConnected(connected: Boolean) {
        mqttService.setMockConnected(connected)
    }

    fun seedMockHistory(points: List<DeviceTelemetry>) {
        mqttService.seedMockHistory(points)
    }
}