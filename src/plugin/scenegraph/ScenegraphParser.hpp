#pragma once
#include <maya\MFnMesh.h>
namespace wr
{
	class SceneGraph;
	class D3D12RenderSystem;
}

//class MFnMesh;

namespace wmr
{
	class ScenegraphParser
	{
	public:
		ScenegraphParser( wr::D3D12RenderSystem& render_system, wr::SceneGraph& scene_graph );
		~ScenegraphParser();

		void initialize();

	private:
		void meshAdded( MFnMesh& fnmesh );

		wr::SceneGraph& m_scenegraph;
		wr::D3D12RenderSystem& m_render_system;
	};
}
