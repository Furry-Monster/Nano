package com.example.nano_android

import androidx.test.core.app.ApplicationProvider
import androidx.test.ext.junit.runners.AndroidJUnit4
import org.junit.Assert.assertEquals
import org.junit.Assert.assertTrue
import org.junit.Test
import org.junit.runner.RunWith
import java.io.File

@RunWith(AndroidJUnit4::class)
class ModelSelectActivityTest {

    private val context = ApplicationProvider.getApplicationContext<android.content.Context>()

    @Test
    fun listAvailableModels_returnsModelsFromAssets() {
        val models = ModelSelectActivity.listAvailableModels(context.assets)
        // Should find Mitsuba and Cian from res/ (copied to assets during build)
        assertTrue(
            "Should find at least one model from assets (Mitsuba or Cian)",
            models.isNotEmpty()
        )
        models.forEach { model ->
            assertTrue("BVH path should end with .bvh", model.bvhPath.endsWith(".bvh"))
            assertTrue("Mesh path should end with .nanomesh", model.meshPath.endsWith(".nanomesh"))
            assertTrue("Name should not be empty", model.name.isNotBlank())
            assertTrue("Asset paths should start with res/", model.bvhPath.startsWith("res/"))
        }
    }

    @Test
    fun listImportedModels_returnsModelsFromFilesystem() {
        val modelsDir = File(context.getExternalFilesDir(null), "models")
        val testModelDir = File(modelsDir, "test_model_${System.currentTimeMillis()}")
        try {
            testModelDir.mkdirs()
            File(testModelDir, "test.bvh").writeBytes(ByteArray(1))
            File(testModelDir, "test.nanomesh").writeBytes(ByteArray(1))

            val models = ModelSelectActivity.listImportedModels(context)
            val ourModel = models.find { it.name == testModelDir.name }
            assertEquals("Should find our test model", testModelDir.name, ourModel?.name)
            assertEquals(
                File(testModelDir, "test.bvh").absolutePath,
                ourModel?.bvhPath
            )
            assertEquals(
                File(testModelDir, "test.nanomesh").absolutePath,
                ourModel?.meshPath
            )
        } finally {
            testModelDir.deleteRecursively()
        }
    }

    @Test
    fun listImportedModels_ignoresIncompleteModelDirs() {
        val modelsDir = File(context.getExternalFilesDir(null), "models")
        val incompleteDir = File(modelsDir, "incomplete_${System.currentTimeMillis()}")
        try {
            incompleteDir.mkdirs()
            File(incompleteDir, "only_bvh.bvh").writeBytes(ByteArray(1))
            // No .nanomesh file

            val modelsBefore = ModelSelectActivity.listImportedModels(context)
            val found = modelsBefore.any { it.name == incompleteDir.name }
            assertTrue("Incomplete dir (bvh only) should not be listed", !found)
        } finally {
            incompleteDir.deleteRecursively()
        }
    }
}
