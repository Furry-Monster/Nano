package com.example.nano_android

object ModelPathUtils {
    /**
     * Derives output directory name from a file name (e.g. "model.fbx" -> "model").
     * Used when exporting imported models.
     */
    fun outputDirNameFromFileName(fileName: String): String =
        fileName.substringBeforeLast('.').ifEmpty { fileName }
}
