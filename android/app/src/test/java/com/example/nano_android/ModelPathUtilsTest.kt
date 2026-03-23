package com.example.nano_android

import org.junit.Assert.assertEquals
import org.junit.Test

class ModelPathUtilsTest {

    @Test
    fun `outputDirNameFromFileName extracts base name from standard extension`() {
        assertEquals("model", ModelPathUtils.outputDirNameFromFileName("model.fbx"))
        assertEquals("scene", ModelPathUtils.outputDirNameFromFileName("scene.gltf"))
        assertEquals("asset", ModelPathUtils.outputDirNameFromFileName("asset.glb"))
    }

    @Test
    fun `outputDirNameFromFileName handles multiple dots`() {
        assertEquals("my.model", ModelPathUtils.outputDirNameFromFileName("my.model.gltf"))
    }

    @Test
    fun `outputDirNameFromFileName returns full name when no extension`() {
        assertEquals("noextension", ModelPathUtils.outputDirNameFromFileName("noextension"))
    }

    @Test
    fun `outputDirNameFromFileName handles empty string`() {
        assertEquals("", ModelPathUtils.outputDirNameFromFileName(""))
    }

    @Test
    fun `outputDirNameFromFileName handles dot prefix`() {
        assertEquals(".hidden", ModelPathUtils.outputDirNameFromFileName(".hidden"))
    }
}
