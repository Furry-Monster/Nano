package com.example.nano_android

import android.app.Activity
import android.content.Intent
import android.net.Uri
import android.os.Bundle
import android.provider.OpenableColumns
import android.widget.Button
import android.widget.Toast
import androidx.activity.result.contract.ActivityResultContracts
import androidx.appcompat.app.AppCompatActivity
import java.io.File
import java.io.FileOutputStream

class ToolSelectActivity : AppCompatActivity() {

    private val pickFile = registerForActivityResult(
        ActivityResultContracts.StartActivityForResult()
    ) { result ->
        if (result.resultCode != Activity.RESULT_OK || result.data?.data == null) return@registerForActivityResult
        val uri = result.data!!.data!!
        handlePickedFile(uri)
    }

    override fun onCreate(savedInstanceState: Bundle?) {
        super.onCreate(savedInstanceState)
        setContentView(R.layout.activity_tool_select)

        NanoRenderer.nativeInit(assets)

        findViewById<Button>(R.id.btn_select_model).setOnClickListener {
            startActivity(Intent(this, ModelSelectActivity::class.java))
        }

        findViewById<Button>(R.id.btn_import_export).setOnClickListener {
            val intent = Intent(Intent.ACTION_OPEN_DOCUMENT).apply {
                addCategory(Intent.CATEGORY_OPENABLE)
                type = "*/*"
                putExtra(Intent.EXTRA_MIME_TYPES, arrayOf(
                    "model/fbx",
                    "model/gltf-binary",
                    "model/gltf+json",
                    "application/octet-stream"
                ))
            }
            pickFile.launch(intent)
        }
    }

    private fun handlePickedFile(uri: Uri) {
        val fileName = queryFileName(uri) ?: "imported_model"
        val importDir = File(getExternalFilesDir(null), "import").apply { mkdirs() }
        val modelsDir = File(getExternalFilesDir(null), "models").apply { mkdirs() }
        val tempFile = File(importDir, fileName)

        try {
            contentResolver.openInputStream(uri)?.use { input ->
                FileOutputStream(tempFile).use { output ->
                    input.copyTo(output)
                }
            }
        } catch (e: Exception) {
            Toast.makeText(this, "复制失败: ${e.message}", Toast.LENGTH_LONG).show()
            return
        }

        val outputDir = File(modelsDir, ModelPathUtils.outputDirNameFromFileName(fileName))
        outputDir.mkdirs()

        val result = NanoRenderer.nativeExportModel(tempFile.absolutePath, "$outputDir/")
        tempFile.delete()

        if (result == "OK") {
            Toast.makeText(this, "处理完成", Toast.LENGTH_SHORT).show()
            startActivity(Intent(this, ModelSelectActivity::class.java))
        } else {
            Toast.makeText(this, "处理失败: $result", Toast.LENGTH_LONG).show()
        }
    }

    private fun queryFileName(uri: Uri): String? {
        contentResolver.query(uri, null, null, null, null)?.use { cursor ->
            if (cursor.moveToFirst()) {
                val idx = cursor.getColumnIndex(OpenableColumns.DISPLAY_NAME)
                if (idx >= 0) return cursor.getString(idx)
            }
        }
        return uri.path?.substringAfterLast('/')
    }
}
