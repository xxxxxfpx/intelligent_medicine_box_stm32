package com.example.stm32_box.ui

import android.os.Bundle
import androidx.activity.ComponentActivity
import androidx.activity.compose.setContent
import androidx.compose.foundation.Canvas
import androidx.compose.foundation.background
import androidx.compose.foundation.horizontalScroll
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
import androidx.compose.material.icons.automirrored.filled.ArrowBack
import androidx.compose.material.icons.filled.DeviceThermostat
import androidx.compose.material.icons.filled.MyLocation
import androidx.compose.material3.Card
import androidx.compose.material3.CardDefaults
import androidx.compose.material3.ExperimentalMaterial3Api
import androidx.compose.material3.FilterChip
import androidx.compose.material3.FilterChipDefaults
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
import androidx.compose.runtime.mutableStateOf
import androidx.compose.runtime.remember
import androidx.compose.runtime.setValue
import androidx.compose.ui.Alignment
import androidx.compose.ui.Modifier
import androidx.compose.ui.geometry.Offset
import androidx.compose.ui.graphics.Color
import androidx.compose.ui.graphics.Path
import androidx.compose.ui.graphics.StrokeCap
import androidx.compose.ui.graphics.StrokeJoin
import androidx.compose.ui.graphics.drawscope.Stroke
import androidx.compose.ui.graphics.nativeCanvas
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
private val latitudeColor = Color(0xFF43A047)
private val longitudeColor = Color(0xFFFB8C00)

data class TimeSpan(val label: String, val millis: Long)

private val timeSpans = listOf(
    TimeSpan("1分钟", 60_000L),
    TimeSpan("5分钟", 300_000L),
    TimeSpan("15分钟", 900_000L),
    TimeSpan("全部", Long.MAX_VALUE),
)

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
    var selectedSpanIdx by remember { mutableStateOf(3) } // default: 全部
    val selectedSpan = timeSpans[selectedSpanIdx]

    val now = System.currentTimeMillis()
    val filtered = history.filter { now - it.timestamp <= selectedSpan.millis }

    Scaffold(
        topBar = {
            TopAppBar(
                title = { Text("历史数据", fontWeight = FontWeight.Bold) },
                navigationIcon = {
                    IconButton(onClick = onBack) {
                        Icon(Icons.AutoMirrored.Filled.ArrowBack, contentDescription = "返回")
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
            // 时间跨度选择器
            Row(
                modifier = Modifier
                    .fillMaxWidth()
                    .horizontalScroll(rememberScrollState()),
                horizontalArrangement = Arrangement.spacedBy(8.dp)
            ) {
                timeSpans.forEachIndexed { idx, span ->
                    FilterChip(
                        selected = idx == selectedSpanIdx,
                        onClick = { selectedSpanIdx = idx },
                        label = { Text(span.label, fontSize = 13.sp) },
                        colors = FilterChipDefaults.filterChipColors(
                            selectedContainerColor = MaterialTheme.colorScheme.primary,
                            selectedLabelColor = MaterialTheme.colorScheme.onPrimary
                        )
                    )
                }
            }

            if (filtered.isEmpty()) {
                Box(
                    modifier = Modifier
                        .fillMaxWidth()
                        .height(300.dp),
                    contentAlignment = Alignment.Center
                ) {
                    Column(horizontalAlignment = Alignment.CenterHorizontally) {
                        Icon(
                            Icons.Default.DeviceThermostat,
                            contentDescription = null,
                            tint = MaterialTheme.colorScheme.onSurface.copy(alpha = 0.3f),
                            modifier = Modifier.size(48.dp)
                        )
                        Spacer(Modifier.height(8.dp))
                        Text(
                            "暂无数据",
                            style = MaterialTheme.typography.bodyLarge,
                            color = MaterialTheme.colorScheme.onSurface.copy(alpha = 0.4f)
                        )
                    }
                }
            } else {
                // 温度折线图
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
                        Text(
                            "${filtered.size} 个数据点 · ${timeSpans[selectedSpanIdx].label}",
                            style = MaterialTheme.typography.bodySmall,
                            color = MaterialTheme.colorScheme.onSurface.copy(alpha = 0.4f)
                        )
                        Spacer(Modifier.height(12.dp))

                        TelemetryLineChart(
                            data = filtered,
                            getValue = { it.objectTemp },
                            getValue2 = { it.ambientTemp },
                            color1 = objectTempColor,
                            color2 = ambientTempColor,
                            unit = "°C",
                            modifier = Modifier
                                .fillMaxWidth()
                                .height(240.dp)
                        )

                        Spacer(Modifier.height(8.dp))
                        Row(
                            modifier = Modifier.fillMaxWidth(),
                            horizontalArrangement = Arrangement.Center,
                            verticalAlignment = Alignment.CenterVertically
                        ) {
                            LegendDot(objectTempColor, "物体温度")
                            Spacer(Modifier.width(20.dp))
                            LegendDot(ambientTempColor, "环境温度")
                        }
                    }
                }

                // GPS 折线图
                val hasGps = filtered.any { it.latitude != null && it.longitude != null }
                if (hasGps) {
                    Card(
                        modifier = Modifier.fillMaxWidth(),
                        shape = RoundedCornerShape(16.dp),
                        elevation = CardDefaults.cardElevation(defaultElevation = 2.dp)
                    ) {
                        Column(modifier = Modifier.padding(16.dp)) {
                            Row(verticalAlignment = Alignment.CenterVertically) {
                                Icon(
                                    Icons.Default.MyLocation,
                                    contentDescription = null,
                                    tint = MaterialTheme.colorScheme.primary,
                                    modifier = Modifier.size(20.dp)
                                )
                                Spacer(Modifier.width(8.dp))
                                Text(
                                    "GPS 坐标变化",
                                    style = MaterialTheme.typography.titleMedium,
                                    fontWeight = FontWeight.Bold
                                )
                            }
                            Spacer(Modifier.height(12.dp))

                            TelemetryLineChart(
                                data = filtered,
                                getValue = { it.latitude?.toFloat() },
                                getValue2 = { it.longitude?.toFloat() },
                                color1 = latitudeColor,
                                color2 = longitudeColor,
                                unit = "",
                                modifier = Modifier
                                    .fillMaxWidth()
                                    .height(240.dp)
                            )

                            Spacer(Modifier.height(8.dp))
                            Row(
                                modifier = Modifier.fillMaxWidth(),
                                horizontalArrangement = Arrangement.Center,
                                verticalAlignment = Alignment.CenterVertically
                            ) {
                                LegendDot(latitudeColor, "纬度")
                                Spacer(Modifier.width(20.dp))
                                LegendDot(longitudeColor, "经度")
                            }
                        }
                    }
                }

                // 数据统计
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

                        val objTemps = filtered.mapNotNull { it.objectTemp }
                        val ambTemps = filtered.mapNotNull { it.ambientTemp }
                        val lats = filtered.mapNotNull { it.latitude }
                        val lngs = filtered.mapNotNull { it.longitude }

                        StatRow("数据点数", "${filtered.size}", MaterialTheme.colorScheme.onSurface)
                        StatRow(
                            "物体温度范围",
                            "${"%.1f".format(objTemps.minOrNull() ?: 0f)} ~ ${"%.1f".format(objTemps.maxOrNull() ?: 0f)}°C",
                            objectTempColor
                        )
                        StatRow(
                            "环境温度范围",
                            "${"%.1f".format(ambTemps.minOrNull() ?: 0f)} ~ ${"%.1f".format(ambTemps.maxOrNull() ?: 0f)}°C",
                            ambientTempColor
                        )
                        if (lats.isNotEmpty() && lngs.isNotEmpty()) {
                            StatRow(
                                "纬度范围",
                                "${"%.4f".format(lats.minOrNull() ?: 0.0)} ~ ${"%.4f".format(lats.maxOrNull() ?: 0.0)}",
                                latitudeColor
                            )
                            StatRow(
                                "经度范围",
                                "${"%.4f".format(lngs.minOrNull() ?: 0.0)} ~ ${"%.4f".format(lngs.maxOrNull() ?: 0.0)}",
                                longitudeColor
                            )
                        }
                        val latest = filtered.last()
                        StatRow(
                            "当前物体温度",
                            latest.objectTemp?.let { "%.1f°C".format(it) } ?: "--",
                            objectTempColor
                        )
                        StatRow(
                            "当前环境温度",
                            latest.ambientTemp?.let { "%.1f°C".format(it) } ?: "--",
                            ambientTempColor
                        )
                    }
                }
            }
        }
    }
}

@Composable
fun LegendDot(color: Color, label: String) {
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
    data: List<DeviceTelemetry>,
    getValue: (DeviceTelemetry) -> Float?,
    getValue2: (DeviceTelemetry) -> Float?,
    color1: Color,
    color2: Color,
    unit: String,
    modifier: Modifier = Modifier
) {
    if (data.isEmpty()) return

    val displayData = if (data.size > 150) data.takeLast(150) else data

    val vals1 = displayData.mapNotNull(getValue)
    val vals2 = displayData.mapNotNull(getValue2)
    val allVals = vals1 + vals2

    if (allVals.isEmpty()) return

    val minVal = (allVals.minOrNull() ?: 0f) - (allVals.maxOrNull()?.minus(allVals.minOrNull() ?: 0f)?.times(0.1f) ?: 1f)
    val maxVal = (allVals.maxOrNull() ?: 30f) + (allVals.maxOrNull()?.minus(allVals.minOrNull() ?: 0f)?.times(0.1f) ?: 1f)
    val range = if (maxVal - minVal < 0.01f) 1f else maxVal - minVal

    val timeFormat = SimpleDateFormat("HH:mm:ss", Locale.getDefault())

    Canvas(modifier = modifier) {
        val chartLeft = 60.dp.toPx()
        val chartBottom = size.height - 28.dp.toPx()
        val chartWidth = size.width - chartLeft - 12.dp.toPx()
        val chartHeight = chartBottom - 12.dp.toPx()

        val gridColor = Color(0xFFE0E0E0)

        val paint = android.graphics.Paint().apply {
            color = android.graphics.Color.parseColor("#9E9E9E")
            textSize = 10.dp.toPx()
            isAntiAlias = true
        }

        // 水平网格线
        val gridLines = 4
        for (i in 0..gridLines) {
            val y = chartBottom - (chartHeight * i / gridLines)
            drawLine(gridColor, Offset(chartLeft, y), Offset(size.width - 12.dp.toPx(), y), strokeWidth = 1f)

            val v = minVal + range * i / gridLines
            val label = if (unit == "°C") "%.1f$unit".format(v) else "%.4f".format(v.toDouble())
            drawContext.canvas.nativeCanvas.drawText(label, 4.dp.toPx(), y + 4.dp.toPx(), paint)
        }

        // 时间标签
        val timeLabels = 4
        for (i in 0..timeLabels) {
            val index = (displayData.size - 1) * i / timeLabels
            if (index < displayData.size) {
                val x = chartLeft + chartWidth * i / timeLabels
                val t = timeFormat.format(Date(displayData[index].timestamp))
                drawContext.canvas.nativeCanvas.drawText(t, x - 20.dp.toPx(), chartBottom + 18.dp.toPx(), paint)
            }
        }

        if (displayData.size < 2) return@Canvas

        // 绘制第一条线
        val path1 = Path()
        var first = true
        for (i in displayData.indices) {
            val v = getValue(displayData[i]) ?: continue
            val x = chartLeft + (chartWidth * i / (displayData.size - 1))
            val y = chartBottom - ((v - minVal) / range * chartHeight)
            if (first) { path1.moveTo(x, y); first = false }
            else { path1.lineTo(x, y) }
        }
        drawPath(path1, color1, style = Stroke(2.5.dp.toPx(), cap = StrokeCap.Round, join = StrokeJoin.Round))
        for (i in displayData.indices) {
            val v = getValue(displayData[i]) ?: continue
            val x = chartLeft + (chartWidth * i / (displayData.size - 1))
            val y = chartBottom - ((v - minVal) / range * chartHeight)
            drawCircle(color1, 3.dp.toPx(), Offset(x, y))
        }

        // 绘制第二条线
        val path2 = Path()
        first = true
        for (i in displayData.indices) {
            val v = getValue2(displayData[i]) ?: continue
            val x = chartLeft + (chartWidth * i / (displayData.size - 1))
            val y = chartBottom - ((v - minVal) / range * chartHeight)
            if (first) { path2.moveTo(x, y); first = false }
            else { path2.lineTo(x, y) }
        }
        drawPath(path2, color2, style = Stroke(2.5.dp.toPx(), cap = StrokeCap.Round, join = StrokeJoin.Round))
        for (i in displayData.indices) {
            val v = getValue2(displayData[i]) ?: continue
            val x = chartLeft + (chartWidth * i / (displayData.size - 1))
            val y = chartBottom - ((v - minVal) / range * chartHeight)
            drawCircle(color2, 3.dp.toPx(), Offset(x, y))
        }
    }
}
