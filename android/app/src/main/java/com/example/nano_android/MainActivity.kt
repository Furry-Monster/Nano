package com.example.nano_android

import android.content.Intent
import android.os.Bundle
import android.os.Handler
import android.os.Looper
import android.view.MotionEvent
import android.view.SurfaceHolder
import android.view.SurfaceView
import android.view.WindowManager
import android.widget.Button
import android.widget.TextView
import androidx.appcompat.app.AppCompatActivity
import androidx.core.view.WindowCompat
import androidx.core.view.WindowInsetsCompat
import androidx.core.view.WindowInsetsControllerCompat
import com.example.nano_android.R

class MainActivity : AppCompatActivity(), SurfaceHolder.Callback {

    private lateinit var surfaceView: SurfaceView
    private var surfaceReady = false
    private lateinit var statsText: TextView
    private val statsHandler = Handler(Looper.getMainLooper())
    private var statsRunnable: Runnable? = null

    override fun onCreate(savedInstanceState: Bundle?) {
        super.onCreate(savedInstanceState)
        setContentView(R.layout.activity_main)

        window.addFlags(WindowManager.LayoutParams.FLAG_KEEP_SCREEN_ON)
        WindowCompat.setDecorFitsSystemWindows(window, false)
        WindowInsetsControllerCompat(window, window.decorView).let { controller ->
            controller.hide(WindowInsetsCompat.Type.systemBars())
            controller.systemBarsBehavior =
                WindowInsetsControllerCompat.BEHAVIOR_SHOW_TRANSIENT_BARS_BY_SWIPE
        }

        NanoRenderer.nativeInit(assets)

        val bvhPath = intent.getStringExtra(ModelSelectActivity.EXTRA_BVH_PATH)
        val meshPath = intent.getStringExtra(ModelSelectActivity.EXTRA_MESH_PATH)
        if (bvhPath != null && meshPath != null) {
            NanoRenderer.nativeSetModelPaths(bvhPath, meshPath)
        }

        surfaceView = findViewById(R.id.vulkan_surface)
        surfaceView.holder.addCallback(this)
        surfaceView.setOnTouchListener { _, event ->
            NanoRenderer.handleTouchEvent(event)
            true
        }

        statsText = findViewById(R.id.stats_text)
        findViewById<Button>(R.id.btn_lod_mode).setOnClickListener {
            NanoRenderer.nativeToggleAutoLOD()
        }
        findViewById<Button>(R.id.btn_lod_up).setOnClickListener {
            NanoRenderer.nativeLODUp()
        }
        findViewById<Button>(R.id.btn_lod_down).setOnClickListener {
            NanoRenderer.nativeLODDown()
        }
        findViewById<Button>(R.id.btn_back_select).setOnClickListener {
            startActivity(Intent(this, ModelSelectActivity::class.java))
            finish()
        }
    }

    override fun surfaceCreated(holder: SurfaceHolder) {}

    override fun surfaceChanged(holder: SurfaceHolder, format: Int, width: Int, height: Int) {
        if (!surfaceReady) {
            NanoRenderer.nativeSurfaceCreated(holder.surface, width, height)
            surfaceReady = true
            startStatsUpdate()
        }
    }

    override fun surfaceDestroyed(holder: SurfaceHolder) {
        surfaceReady = false
        stopStatsUpdate()
        NanoRenderer.nativeSurfaceDestroyed()
    }

    private fun startStatsUpdate() {
        statsRunnable?.let { statsHandler.removeCallbacks(it) }
        statsRunnable = object : Runnable {
            override fun run() {
                if (surfaceReady) {
                    statsText.text = NanoRenderer.nativeGetStats()
                }
                statsHandler.postDelayed(this, 250)
            }
        }
        statsHandler.post(statsRunnable!!)
    }

    private fun stopStatsUpdate() {
        statsRunnable?.let { statsHandler.removeCallbacks(it) }
        statsRunnable = null
    }

    override fun onTouchEvent(event: MotionEvent): Boolean {
        return super.onTouchEvent(event)
    }

    override fun onDestroy() {
        stopStatsUpdate()
        NanoRenderer.nativeDestroy()
        super.onDestroy()
    }
}
