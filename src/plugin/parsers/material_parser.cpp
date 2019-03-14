#include "material_parser.hpp"

// Maya API
#include <maya/MFnDependencyNode.h>
#include <maya/MFnLambertShader.h>
#include <maya/MFnMesh.h>
#include <maya/MFnSet.h>
#include <maya/MIntArray.h>
#include <maya/MItDependencyGraph.h>
#include <maya/MObjectArray.h>
#include <maya/MPlug.h>
#include <maya/MPlugArray.h>

// C++ standard
#include <vector>

#include <maya/MGlobal.h>
#include <sstream>

// https://nccastaff.bournemouth.ac.uk/jmacey/RobTheBloke/www/research/maya/mfnmesh.htm
void wmr::MaterialParser::Parse(const MFnMesh& mesh)
{
	// Number of instances of this mesh
	std::uint32_t instance_count = mesh.parentCount();

	for (auto instance_index = 0; instance_index < instance_count; ++instance_index)
	{
		// Attach a function set to the instance
		MFnDependencyNode mesh_fn(mesh.parent(instance_index));

		// References to the shaders used on the meshes
		MObjectArray shaders;

		// Indices to the materials in the object array
		MIntArray material_indices;

		// Get all attached shaders for this instance
		mesh.getConnectedShaders(instance_index, shaders, material_indices);

		switch (shaders.length())
		{
			// No shaders applied to this mesh instance
			case 0:
				{}
				break;

				// All faces use the same material
			case 1:
				{
					// Output hack
					std::ostringstream os;

					MFnDependencyNode fn_node(shaders[0]);
					MPlug shader_plug = fn_node.findPlug("surfaceShader", true);

					// Is this a valid plug?
					if (!shader_plug.isNull())
					{
						MPlugArray connected_plugs;
						shader_plug.connectedTo(connected_plugs, true, false);

						// Could not find a valid connected plug
						if (connected_plugs.length() != 1)
							return;

						os << connected_plugs[0].node().apiTypeStr();

						// Found a lambert shader
						if (connected_plugs[0].node().apiType() == MFn::kLambert)
						{
							auto color_plug = MFnDependencyNode(connected_plugs[0].node()).findPlug("color");

							MItDependencyGraph dependency_graph_iterator(
								color_plug,
								MFn::kFileTexture,
								MItDependencyGraph::kUpstream,
								MItDependencyGraph::kBreadthFirst,
								MItDependencyGraph::kNodeLevel);

							dependency_graph_iterator.disablePruningOnFilter();

							auto texture_node = dependency_graph_iterator.currentItem();
							auto file_name_plug = MFnDependencyNode(texture_node).findPlug("fileTextureName", true);

							MString texture_path;
							file_name_plug.getValue(texture_path);
							os << texture_path.asChar();
						}
					}

					MGlobal::displayInfo(os.str().c_str());
				}
				break;

				// Two or more materials are used
			default:
				{
					// Get the number of materials by doing:
					// auto num_of_mats = shaders.length()

					// Holds sorted face indices based on their applied material
					std::vector<std::vector<std::uint32_t>> faces_by_material_index;

					// Make sure the container has the same size as the number of shaders
					faces_by_material_index.resize(shaders.length());

					for (auto material_index = 0; material_index < material_indices.length(); ++material_index)
					{
						faces_by_material_index[material_indices[material_index]].push_back(material_index);
					}

					for (auto shader_index = 0; shader_index < shaders.length(); ++shader_index)
					{
						// Get all faces used by this material index
						for (unsigned int & itr : faces_by_material_index[shader_index])
						{
							// Use faces by material ID here
						}
					}
				}
				break;
		}
	}
}
