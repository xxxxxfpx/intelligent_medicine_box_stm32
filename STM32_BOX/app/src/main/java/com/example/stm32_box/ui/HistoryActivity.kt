package com.example.stm32_box.ui

import android.os.Bundle
import androidx.activity.ComponentActivity
import androidx.activity.compose.setContent
import androidx.compose.foundation.Canvas
import androidx.compose.foundation.background
import androidx.compose.foundation.layout.Arrangement
import androidx.compose.foundation.layout.Box
import androidx.compose.foundation.layout.Column
import androidx.compose.foundation.layout.Row
import androidx.compose.foundation.layout.Spacer
import androidx.compose.foundation.layout.fillMaxSize
import androidx.compose.foundation.layout.fillMaxWidth
import androidx.compose.foundation.layout.height
import androidx.compose.foundation.layout.padding
import androidx.compose.foundation.layout.size
import androidx.compose.foundation.layout.width
import androidx.compose.foundation.rememberScrollState
import androidx.compose.foundation.shape.RoundedCornerShape
import androidx.compose.foundation.verticalScroll
import androidx.compose.material.icons.Icons
import androidx.compose.material.icons.filled.ArrowBack
import androidx.compose.material.icons.outlined.DeviceThermostat
import androidx.compose.material3.Card
import androidx.compose.material3.CardDefaults
import androidx.compose.material3.ExperimentalMaterial3Api
import androidx.compose.material3.Icon
import androidx.compose.material3.IconButton
import androidx.compose.material3.MaterialTheme
import androidx.compose.material3.Scaffold
import androidx.compose.material3.Text
import androidx.compose.material3.TopAppBar
import androidx.compose.material3.TopAppBarDefaults
import androidx.compose.runtime.Composable
import androidx.compose.runtime.collectAsState
import androidx.compose.runtime.getValue
import androidx.compose.ui.Alignment
import androidx.compose.ui.Modifier
import androidx.compose.ui.geometry.Offset
import androidx.compose.ui.graphics.Color
import androidx.compose.ui.graphics.Path
import androidx.compose.ui.graphics.StrokeCap
import androidx.compose.ui.graphics.StrokeJoin
import androidx.compose.ui.graphics.drawscope.Stroke
import androidx.compose.ui.graphics.nativeCanvas
import androidx.compose.ui.platform.LocalContext
import androidx.compose.ui.text.font.FontWeight
import androidx.compose.ui.unit.dp
import androidx.compose.ui.unit.sp
import androidx.lifecycle.viewmodel.compose.viewModel
import com.example.stm32_box.data.mqtt.DeviceTelemetry
import com.example.stm32_box.ui.theme.STM32_BOXTheme
import java.text.SimpleDateFormat
import java.util.Date
import java.util.Locale

private val objectTempColor = Color(0xFFE53935)
private val ambientTempColor = Color(0xFF1E88E5)

class HistoryActivity : ComponentActivity() {
    override fun onCreate(savedInstanceState: Bundle?) {
        super.onCreate(savedInstanceState)
        setContent {
            STM32_BOXTheme {
                val viewModel: MainViewModel = viewModel()
                val history by viewModel.telemetryHistory.collectAsState()

                HistoryScreen(
                    history = history,
                    onBack = { finish() }
                )
            }
        }
    }
}

@OptIn(ExperimentalMaterial3Api::class)
@Composable
fun HistoryScreen(
    history: List<DeviceTelemetry>,
    onBack: () -> Unit
) {
    Scaffold(
        topBar = {
            TopAppBar(
                title = { Text("历史数据", fontWeight = FontWeight.Bold) },
                navigationIcon = {
                    IconButton(onClick = onBack) {
                        Icon(Icons.Default.ArrowBack, contentDescription = "返回")
                    }
                },
                colors = TopAppBarDefaults.topAppBarColors(
                    containerColor = MaterialTheme.colorScheme.primary,
                    titleContentColor = MaterialTheme.colorScheme.onPrimary,
                    navigationIconContentColor = MaterialTheme.colorScheme.onPrimary
                )
            )
        }
    ) { padding ->
        Column(
            modifier = Modifier
                .fillMaxSize()
                .background(MaterialTheme.colorScheme.background)
                .padding(padding)
                .padding(16.dp)
                .verticalScroll(rememberScrollState()),
            verticalArrangement = Arrangement.spacedBy(16.dp)
        ) {
            if (history.isEmpty()) {
                Box(
                    modifier = Modifier
                        .fillMaxWidth()
                        .height(300.dp),
                    contentAlignment = Alignment.Center
                ) {
                    Column(horizontalAlignment = Alignment.CenterHorizontally) {
                        Icon(
                            Icons.Outlined.DeviceThermostat,
                            contentDescription = null,
                            tint = MaterialTheme.colorScheme.onSurface.copy(alpha = 0.3f),
                            modifier = Modifier.size(48.dp)
                        )
                        Spacer(Modifier.height(8.dp))
                        Text(
                            "暂无历史数据",
                            style = MaterialTheme.typography.bodyLarge,
                            color = MaterialTheme.colorScheme.onSurface.copy(alpha = 0.4f)
                        )
                        Text(
                            "等待设备上报数据...",
                            style = MaterialTheme.typography.bodySmall,
                            color = MaterialTheme.colorScheme.onSurface.copy(alpha = 0.3f)
                        )
                    }
                }
            } else {
                // 折线图卡片
                Card(
                    modifier = Modifier.fillMaxWidth(),
                    shape = RoundedCornerShape(16.dp),
                    elevation = CardDefaults.cardElevation(defaultElevation = 2.dp)
                ) {
                    Column(modifier = Modifier.padding(16.dp)) {
                        Text(
                            "温度变化趋势",
                            style = MaterialTheme.typography.titleMedium,
                            fontWeight = FontWeight.Bold
                        )
                        Spacer(Modifier.height(16.dp))

                        TelemetryLineChart(
                            history = history,
                            modifier = Modifier
                                .fillMaxWidth()
                                .height(280.dp)
                        )

                        Spacer(Modifier.height(12.dp))

                        // 图例
                        Row(
                            modifier = Modifier.fillMaxWidth(),
                            horizontalArrangement = Arrangement.Center,
                            verticalAlignment = Alignment.CenterVertically
                        ) {
                            LegendItem(color = objectTempColor, label = "物体温度")
                            Spacer(Modifier.width(24.dp))
                            LegendItem(color = ambientTempColor, label = "环境温度")
                        }
                    }
                }

                // 数据统计卡片
                val latest = history.last()
                Card(
                    modifier = Modifier.fillMaxWidth(),
                    shape = RoundedCornerShape(16.dp),
                    elevation = CardDefaults.cardElevation(defaultElevation = 2.dp)
                ) {
                    Column(modifier = Modifier.padding(16.dp)) {
                        Text(
                            "数据统计",
                            style = MaterialTheme.typography.titleMedium,
                            fontWeight = FontWeight.Bold
                        )
                        Spacer(Modifier.height(12.dp))

                        val objTemps = history.mapNotNull { it.objectTemp }
                        val ambTemps = history.mapNotNull { it.ambientTemp }

                        StatRow(
                            label = "物体温度范围",
                            value = "${"%.1f".format(objTemps.minOrNull() ?: 0f)}°C ~ ${"%.1f".format(objTemps.maxOrNull() ?: 0f)}°C",
                            color = objectTempColor
                        )
                        StatRow(
                            label = "环境温度范围",
                            value = "${"%.1f".format(ambTemps.minOrNull() ?: 0f)}°C ~ ${"%.1f".format(ambTemps.maxOrNull() ?: 0f)}°C",
                            color = ambientTempColor
                        )
                        StatRow(
                            label = "数据点数",
                            value = "${history.size}",
                            color = MaterialTheme.colorScheme.onSurface
                        )
                        StatRow(
                            label = "当前物体温度",
                            value = latest.objectTemp?.let { "%.1f°C".format(it) } ?: "--",
                            color = objectTempColor
                        )
                        StatRow(
                            label = "当前环境温度",
                            value = latest.ambientTemp?.let { "%.1f°C".format(it) } ?: "--",
                            color = ambientTempColor
                        )
                    }
                }
            }
        }
    }
}

@Composable
fun LegendItem(color: Color, label: String) {
    Row(verticalAlignment = Alignment.CenterVertically) {
        Box(
            modifier = Modifier
                .size(10.dp)
                .background(color, RoundedCornerShape(2.dp))
        )
        Spacer(Modifier.width(6.dp))
        Text(label, style = MaterialTheme.typography.labelMedium)
    }
}

@Composable
fun StatRow(label: String, value: String, color: Color) {
    Row(
        modifier = Modifier
            .fillMaxWidth()
            .padding(vertical = 4.dp),
        horizontalArrangement = Arrangement.SpaceBetween
    ) {
        Text(
            text = label,
            style = MaterialTheme.typography.bodyMedium,
            color = MaterialTheme.colorScheme.onSurface.copy(alpha = 0.6f)
        )
        Text(
            text = value,
            style = MaterialTheme.typography.bodyMedium,
            fontWeight = FontWeight.SemiBold,
            color = color
        )
    }
}

@Composable
fun TelemetryLineChart(
    history: List<DeviceTelemetry>,
    modifier: Modifier = Modifier
) {
    if (history.isEmpty()) return

    val displayData = if (history.size > 100) history.takeLast(100) else history

    val objTemps = displayData.mapNotNull { it.objectTemp }
    val ambTemps = displayData.mapNotNull { it.ambientTemp }
    val allTemps = objTemps + ambTemps

    if (allTemps.isEmpty()) return

    val minTemp = (allTemps.minOrNull() ?: 0f) - 1f
    val maxTemp = (allTemps.maxOrNull() ?: 30f) + 1f
    val tempRange = maxTemp - minTemp

    val timeFormat = SimpleDateFormat("HH:mm:ss", Locale.getDefault())

    Canvas(modifier = modifier) {
        val chartLeft = 52.dp.toPx()
        val chartBottom = size.height - 28.dp.toPx()
        val chartWidth = size.width - chartLeft - 12.dp.toPx()
        val chartHeight = chartBottom - 12.dp.toPx()

        val gridColor = Color(0xFFE0E0E0)
        val textColor = Color(0xFF9E9E9E)

        // 绘制水平网格线
        val gridLines = 4
        val paint = android.graphics.Paint().apply {
            color = android.graphics.Color.parseColor("#9E9E9E")
            textSize = 10.dp.toPx()
            isAntiAlias = true
        }

        for (i in 0..gridLines) {
            val y = chartBottom - (chartHeight * i / gridLines)
            drawLine(gridColor, Offset(chartLeft, y), Offset(size.width - 12.dp.toPx(), y), strokeWidth = 1f)

            val tempValue = minTemp + tempRange * i / gridLines
            drawContext.canvas.nativeCanvas.drawText(
                "%.1f".format(tempValue),
                4.dp.toPx(),
                y + 4.dp.toPx(),
                paint
            )
        }

        // 绘制时间标签
        val timeLabels = 4
        for (i in 0..timeLabels) {
            val index = (displayData.size - 1) * i / timeLabels
            if (index < displayData.size) {
                val x = chartLeft + chartWidth * i / timeLabels
                val telemetry = displayData[index]
                val timeStr = timeFormat.format(Date(telemetry.timestamp))
                drawContext.canvas.nativeCanvas.drawText(
                    timeStr,
                    x - 20.dp.toPx(),
                    chartBottom + 18.dp.toPx(),
                    paint
                )
            }
        }

        if (displayData.size < 2) return@Canvas

        // 绘制物体温度线
        val objPath = Path()
        var firstPoint = true
        for (i in displayData.indices) {
            val temp = displayData[i].objectTemp ?: continue
            val x = chartLeft + (chartWidth * i / (displayData.size - 1))
            val y = chartBottom - ((temp - minTemp) / tempRange * chartHeight)

            if (firstPoint) {
                objPath.moveTo(x, y)
                firstPoint = false
            } else {
                objPath.lineTo(x, y)
            }
        }

        drawPath(
            path = objPath,
            color = objectTempColor,
            style = Stroke(width = 2.5.dp.toPx(), cap = StrokeCap.Round, join = StrokeJoin.Round)
        )

        // 绘制物体温度数据点
        for (i in displayData.indices) {
            val temp = displayData[i].objectTemp ?: continue
            val x = chartLeft + (chartWidth * i / (displayData.size - 1))
            val y = chartBottom - ((temp - minTemp) / tempRange * chartHeight)
            drawCircle(objectTempColor, 3.dp.toPx(), Offset(x, y))
        }

        // 绘制环境温度线
        val ambPath = Path()
        firstPoint = true
        for (i in displayData.indices) {
            val temp = displayData[i].ambientTemp ?: continue
            val x = chartLeft + (chartWidth * i / (displayData.size - 1))
            val y = chartBottom - ((temp - minTemp) / tempRange * chartHeight)

            if (firstPoint) {
                ambPath.moveTo(x, y)
                firstPoint = false
            } else {
                ambPath.lineTo(x, y)
            }
        }

        drawPath(
            path = ambPath,
            color = ambientTempColor,
            style = Stroke(width = 2.5.dp.toPx(), cap = StrokeCap.Round, join = StrokeJoin.Round)
        )

        // 绘制环境温度数据点
        for (i in displayData.indices) {
            val temp = displayData[i].ambientTemp ?: continue
            val x = chartLeft + (chartWidth * i / (displayData.size - 1))
            val y = chartBottom - ((temp - minTemp) / tempRange * chartHeight)
            drawCircle(ambientTempColor, 3.dp.toPx(), Offset(x, y))
        }
    }
}
