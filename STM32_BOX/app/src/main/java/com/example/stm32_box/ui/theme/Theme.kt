package com.example.stm32_box.ui.theme

import android.app.Activity
import androidx.compose.foundation.isSystemInDarkTheme
import androidx.compose.material3.MaterialTheme
import androidx.compose.material3.darkColorScheme
import androidx.compose.material3.lightColorScheme
import androidx.compose.runtime.Composable
import androidx.compose.runtime.SideEffect
import androidx.compose.ui.graphics.toArgb
import androidx.compose.ui.platform.LocalView
import androidx.core.view.WindowCompat

private val DarkColorScheme = darkColorScheme(
    primary = Green500,
    secondary = Blue500,
    tertiary = Green200,
    error = Red500,
    background = Grey900,
    surface = Grey700,
    onPrimary = Grey100,
    onSecondary = Grey100,
    onTertiary = Grey900,
    onError = Grey100,
    onBackground = Grey100,
    onSurface = Grey100
)

private val LightColorScheme = lightColorScheme(
    primary = Green700,
    secondary = Blue700,
    tertiary = Green500,
    error = Red700,
    background = Grey100,
    surface = androidx.compose.ui.graphics.Color.White,
    onPrimary = Grey100,
    onSecondary = Grey100,
    onTertiary = Grey100,
    onError = Grey100,
    onBackground = Grey900,
    onSurface = Grey900
)

@Composable
fun STM32_BOXTheme(
    darkTheme: Boolean = isSystemInDarkTheme(),
    content: @Composable () -> Unit
) {
    val colorScheme = if (darkTheme) DarkColorScheme else LightColorScheme

    val view = LocalView.current
    if (!view.isInEditMode) {
        SideEffect {
            val window = (view.context as Activity).window
            window.statusBarColor = colorScheme.primary.toArgb()
            WindowCompat.getInsetsController(window, view).isAppearanceLightStatusBars = !darkTheme
        }
    }

    MaterialTheme(
        colorScheme = colorScheme,
        content = content
    )
}