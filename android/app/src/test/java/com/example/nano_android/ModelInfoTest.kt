package com.example.nano_android

import org.junit.Assert.assertEquals
import org.junit.Assert.assertNotEquals
import org.junit.Test

class ModelInfoTest {

    @Test
    fun `ModelInfo holds name and paths correctly`() {
        val model = ModelInfo(
            name = "Mitsuba",
            bvhPath = "res/Mitsuba/mitsuba.bvh",
            meshPath = "res/Mitsuba/mitsuba.nanomesh"
        )
        assertEquals("Mitsuba", model.name)
        assertEquals("res/Mitsuba/mitsuba.bvh", model.bvhPath)
        assertEquals("res/Mitsuba/mitsuba.nanomesh", model.meshPath)
    }

    @Test
    fun `ModelInfo equals by value`() {
        val a = ModelInfo("A", "path1.bvh", "path1.nanomesh")
        val b = ModelInfo("A", "path1.bvh", "path1.nanomesh")
        val c = ModelInfo("B", "path2.bvh", "path2.nanomesh")
        assertEquals(a, b)
        assertNotEquals(a, c)
    }

    @Test
    fun `ModelInfo supports destructuring`() {
        val (name, bvhPath, meshPath) = ModelInfo("Test", "a.bvh", "a.nanomesh")
        assertEquals("Test", name)
        assertEquals("a.bvh", bvhPath)
        assertEquals("a.nanomesh", meshPath)
    }

    @Test
    fun `ModelInfo copy works`() {
        val original = ModelInfo("Orig", "x.bvh", "x.nanomesh")
        val copy = original.copy(name = "Copied")
        assertEquals("Copied", copy.name)
        assertEquals("x.bvh", copy.bvhPath)
    }
}
