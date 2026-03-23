package com.example.nano_android

import android.content.Intent
import android.content.res.AssetManager
import android.os.Bundle
import android.view.View
import android.widget.TextView
import androidx.appcompat.app.AppCompatActivity
import androidx.recyclerview.widget.LinearLayoutManager
import androidx.recyclerview.widget.RecyclerView
import com.example.nano_android.databinding.ActivityModelSelectBinding
import java.io.File

class ModelSelectActivity : AppCompatActivity() {

    private lateinit var binding: ActivityModelSelectBinding

    override fun onCreate(savedInstanceState: Bundle?) {
        super.onCreate(savedInstanceState)
        binding = ActivityModelSelectBinding.inflate(layoutInflater)
        setContentView(binding.root)

        val models = listAvailableModels(assets) + listImportedModels(this)
        if (models.isEmpty()) {
            binding.emptyHint.visibility = View.VISIBLE
            binding.modelList.visibility = View.GONE
        } else {
            binding.emptyHint.visibility = View.GONE
            binding.modelList.layoutManager = LinearLayoutManager(this)
            binding.modelList.adapter = ModelAdapter(models) { model ->
                val intent = Intent(this, MainActivity::class.java)
                    .putExtra(EXTRA_BVH_PATH, model.bvhPath)
                    .putExtra(EXTRA_MESH_PATH, model.meshPath)
                startActivity(intent)
                finish()
            }
        }
    }

    companion object {
        const val EXTRA_BVH_PATH = "bvh_path"
        const val EXTRA_MESH_PATH = "mesh_path"

        fun listAvailableModels(assets: AssetManager): List<ModelInfo> {
            val models = mutableListOf<ModelInfo>()
            val resDirs = assets.list("res") ?: return models

            for (dir in resDirs) {
                val files = assets.list("res/$dir") ?: continue
                val bvhFiles = files.filter { it.endsWith(".bvh") }
                for (bvhFile in bvhFiles) {
                    val baseName = bvhFile.removeSuffix(".bvh")
                    val meshFile = "$baseName.nanomesh"
                    if (meshFile in files) {
                        val displayName = dir + if (baseName != dir) " / $baseName" else ""
                        models.add(
                            ModelInfo(
                                name = displayName.ifEmpty { dir },
                                bvhPath = "res/$dir/$bvhFile",
                                meshPath = "res/$dir/$meshFile"
                            )
                        )
                    }
                }
            }
            return models
        }

        fun listImportedModels(context: android.content.Context): List<ModelInfo> {
            val models = mutableListOf<ModelInfo>()
            val modelsDir = File(context.getExternalFilesDir(null), "models")
            if (!modelsDir.exists()) return models
            modelsDir.listFiles()?.filter { it.isDirectory }?.forEach { dir ->
                dir.listFiles()?.let { files ->
                    val bvh = files.find { it.name.endsWith(".bvh") }
                    val mesh = files.find { it.name.endsWith(".nanomesh") }
                    if (bvh != null && mesh != null) {
                        models.add(
                            ModelInfo(
                                name = dir.name,
                                bvhPath = bvh.absolutePath,
                                meshPath = mesh.absolutePath
                            )
                        )
                    }
                }
            }
            return models
        }
    }
}

private class ModelAdapter(
    private val models: List<ModelInfo>,
    private val onSelect: (ModelInfo) -> Unit
) : RecyclerView.Adapter<ModelAdapter.ViewHolder>() {

    class ViewHolder(view: View) : RecyclerView.ViewHolder(view) {
        val name: TextView = view.findViewById(R.id.model_name)
        val path: TextView = view.findViewById(R.id.model_path)
    }

    override fun onCreateViewHolder(parent: android.view.ViewGroup, viewType: Int): ViewHolder {
        val view = android.view.LayoutInflater.from(parent.context)
            .inflate(R.layout.item_model, parent, false)
        return ViewHolder(view)
    }

    override fun onBindViewHolder(holder: ViewHolder, position: Int) {
        val model = models[position]
        holder.name.text = model.name
        holder.path.text = model.bvhPath
        holder.itemView.setOnClickListener { onSelect(model) }
    }

    override fun getItemCount() = models.size
}
