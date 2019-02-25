#pragma once
#include <maya/MFnMesh.h>
namespace wr
{
	class SceneGraph;
	class D3D12RenderSystem;
	class TexturePool;
	class MaterialPool;
}

//class MFnMesh;

namespace wmr
{
	class ViewportRendererOverride;
	class ScenegraphParser
	{
	public:
		ScenegraphParser();
		~ScenegraphParser();

		void initialize( std::shared_ptr<wr::TexturePool> texture_pool, std::shared_ptr<wr::MaterialPool> material_pool);

	private:
		void meshAdded( MFnMesh& fnmesh );
		static void addedCallback( MObject &node, void *clientData );
		wr::SceneGraph& m_scenegraph;
		wr::D3D12RenderSystem& m_render_system;
		ViewportRendererOverride* m_viewport_override = nullptr;



	};
}
