package com.example.nano_android

import android.content.res.AssetManager
import android.view.MotionEvent
import android.view.Surface

object NanoRenderer {
    init {
        System.loadLibrary("nano_android")
    }

    external fun nativeInit(assetManager: AssetManager)
    external fun nativeSurfaceCreated(surface: Surface, width: Int, height: Int)
    external fun nativeSurfaceDestroyed()
    external fun nativeOnTouch(action: Int, pointerCount: Int, x: Float, y: Float)
    external fun nativeDestroy()

    external fun nativeToggleAutoLOD()
    external fun nativeLODUp()
    external fun nativeLODDown()
    external fun nativeGetStats(): String

    fun handleTouchEvent(event: MotionEvent): Boolean {
        val action = when (event.actionMasked) {
            MotionEvent.ACTION_DOWN -> 0
            MotionEvent.ACTION_POINTER_DOWN -> 3
            MotionEvent.ACTION_UP, MotionEvent.ACTION_CANCEL -> 1
            MotionEvent.ACTION_POINTER_UP -> 4
            MotionEvent.ACTION_MOVE -> 2
            else -> return false
        }
        nativeOnTouch(action, event.pointerCount, event.x, event.y)
        return true
    }
}
