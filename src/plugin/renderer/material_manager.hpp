#pragma once

#include "structs.hpp"
#include "plugin/viewport_renderer_override.hpp"
#include "d3d12/d3d12_material_pool.hpp"

// Maya API
#include <maya/MApiNamespace.h>

#include <memory>
#include <vector>

namespace wr
{
	class MaterialPool;
}

namespace wmr
{
	class ScenegraphParser;

	struct MeshShadingEngineRelation
	{
		MObject mesh; // MFnMesh
		MObject shading_engine; // kShadingEngine
	};

	struct SurfaceShaderShadingEngineRelation
	{
		wr::MaterialHandle material_handle; // Wisp material handle
		MPlug surface_shader; // kShader
		std::vector<MObject> shading_engines; // kShadingEngine that this surface shader is connected to

		std::vector<MObject>::iterator FindShadingEngine(MObject & shading_engine);
	};

	class MaterialManager
	{
	public:
		MaterialManager();
		~MaterialManager() = default;

		//! Set-up references to the renderer and create a fallback texture
		void Initialize();

		//! Deallocate used resources
		void Destroy() noexcept;

		//! Returns a pointer to an element from a vector (storing this reference might, over time, be invalid)
		SurfaceShaderShadingEngineRelation * OnCreateSurfaceShader(MPlug & surface_shader);
		void OnRemoveSurfaceShader(MPlug & surface_shader);

		//! Creates a wisp material and bind all neccessary relationships
		wr::MaterialHandle CreateMaterial(MObject& mesh, MObject &shading_engine, MPlug &surface_shader);

		//! Binds the surface shader and shading engine relationship. 
		// Checks if shading engine is already bound: (true) remove connection and set new conn. (false) Set new conn
		// This doesn't bind the relationship between the shading engine and surface shader!
		wr::MaterialHandle ConnectShaderToShadingEngine(MPlug & surface_shader, MObject & shading_engine, bool apply_material = false);

		//! Unbinds the surface shader and shading engine relationship. 
		// Tries to find the surface shader and a shader engine in the relationships of that shader. Removes the shading engine from the bound shading engines
		// This doesn't bind the relationship between the shading engine and surface shader!
		void DisconnectShaderFromShadingEngine(MPlug & surface_shader, MObject & shading_engine);
		
		//! Binds the MFnMesh and shading engine relationship. Either replaces or adds an new relationship.
		// This doesn't bind the relationship between the shading engine and surface shader!
		void ConnectMeshToShadingEngine(MObject & mesh, MObject & shading_engine, wr::MaterialHandle * material_handle = nullptr);

		//! Unbinds the shader and shading engine relationship. Removes the relationship entry if the relation was found.
		// This doesn't bind the relationship between the shading engine and surface shader!
		void DisconnectMeshFromShadingEngine(MObject & mesh, MObject & shading_engine, bool reset_material = true);

		wr::MaterialHandle GetDefaultMaterial() noexcept;
		wr::Material * GetWispMaterial(wr::MaterialHandle & material_handle);

		//! Returns a pointer to an element from a vector (storing this reference might, over time, be invalid)
		SurfaceShaderShadingEngineRelation * DoesMaterialHandleExist(wr::MaterialHandle & material_handle);
		//! Returns a pointer to an element from a vector (storing this reference might, over time, be invalid)
		SurfaceShaderShadingEngineRelation * DoesShaderEngineExist(MObject & shading_engine);
		//! Returns a pointer to an element from a vector (storing this reference might, over time, be invalid)
		SurfaceShaderShadingEngineRelation * DoesSurfaceShaderExist(MObject & surface_shader);

		wr::MaterialHandle FindWispMaterialByShadingEngine(MObject & shading_engine);
		wr::MaterialHandle FindWispMaterialBySurfaceShader(MObject & surface_shader);

	private:
		wmr::ScenegraphParser * GetSceneParser();

		void ApplyMaterialToModel(wr::MaterialHandle& material_handle, MObject & fnmesh);

		wmr::ScenegraphParser * m_scenegraph_parser;
		
		wr::MaterialHandle m_default_material_handle;
		std::shared_ptr<wr::MaterialPool> m_material_pool;
		wr::TexturePool* m_texture_pool_ptr; //! Does not need to be deleted, it's just a way to get hold of the pool

		//! Relationship array of meshes and shading engines (shader groups)
		// Don't need a struct if mesh can be extracted from a shading engine
		std::vector<MeshShadingEngineRelation> m_mesh_shading_relations;
		//! Relationship array of surface shaders and shading engines (shader groups)
		// A surface shader can be attached to multiple shading engines, so we need to keep track of these materials
		std::vector<SurfaceShaderShadingEngineRelation> m_surface_shader_shading_relations;
	};

}