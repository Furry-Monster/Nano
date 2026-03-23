package com.example.nano_android

import org.junit.Assert.assertEquals
import org.junit.Test

class ModelSelectActivityExtrasTest {

    @Test
    fun `EXTRA_BVH_PATH constant matches expected key`() {
        assertEquals("bvh_path", ModelSelectActivity.EXTRA_BVH_PATH)
    }

    @Test
    fun `EXTRA_MESH_PATH constant matches expected key`() {
        assertEquals("mesh_path", ModelSelectActivity.EXTRA_MESH_PATH)
    }

    @Test
    fun `intent extras keys are used consistently for MainActivity`() {
        // Ensure we can build intent extras without typos
        val bvhPath = "res/Mitsuba/mitsuba.bvh"
        val meshPath = "res/Mitsuba/mitsuba.nanomesh"
        val extras = mapOf(
            ModelSelectActivity.EXTRA_BVH_PATH to bvhPath,
            ModelSelectActivity.EXTRA_MESH_PATH to meshPath
        )
        assertEquals(bvhPath, extras[ModelSelectActivity.EXTRA_BVH_PATH])
        assertEquals(meshPath, extras[ModelSelectActivity.EXTRA_MESH_PATH])
    }
}
