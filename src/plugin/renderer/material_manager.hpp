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
	class SceneGraphParser;

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
	};

	class MaterialManager
	{
	public:
		MaterialManager();
		~MaterialManager();

		void Initialize();

		wr::MaterialHandle GetDefaultMaterial() noexcept;
		wr::MaterialHandle CreateMaterial(MObject& fnmesh, MObject &shading_engine, MPlug &surface_shader);
		wr::Material * GetWispMaterial(wr::MaterialHandle & material_handle);

		// Returns a pointer to an element from a vector (storing this reference might, over time, be invalid)
		SurfaceShaderShadingEngineRelation * DoesMaterialHandleExist(wr::MaterialHandle & material_handle);
		// Returns a pointer to an element from a vector (storing this reference might, over time, be invalid)
		SurfaceShaderShadingEngineRelation * DoesSurfaceShaderExist(MPlug & surface_shader);

	private:

		void ApplyMaterialToModel(wr::MaterialHandle material_handle, MObject & fnmesh);

		wmr::ScenegraphParser * m_scenegraph_parser;
		
		wr::MaterialHandle m_default_material_handle;
		std::shared_ptr<wr::MaterialPool> m_material_pool;

		// Relationship array of meshes and shading engines (shader groups)
		// Don't need a struct if mesh can be extracted from a shading engine
		std::vector<MeshShadingEngineRelation> m_mesh_shading_relations;
		// Relationship array of surface shaders and shading engines (shader groups)
		// A surface shader can be attached to multiple shading engines, so we need to keep track of these materials
		std::vector<SurfaceShaderShadingEngineRelation> m_surface_shader_shading_relations;
	};

}