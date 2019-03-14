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
#include <maya/MFnLambertShader.h>
#include <maya/MDagPath.h>
#include <maya/MFnSet.h>
#include <maya/MItMeshPolygon.h>
#include <maya/MItDependencyGraph.h>

// C++ standard
#include <vector>

#include <maya/MGlobal.h>
#include <sstream>

// https://nccastaff.bournemouth.ac.uk/jmacey/RobTheBloke/www/research/maya/mfnmesh.htm
void wmr::MaterialParser::Parse(const MFnMesh& mesh)
{
	MStatus status;
	MDagPath path;
	mesh.getPath(path);

	// If it is instanced, determine which instance the path refers to
	std::uint32_t instance_count = 0;
	if (path.isInstanced())
		instance_count = path.instanceNumber();

	MObjectArray sets;
	MObjectArray comps;

	if (!mesh.getConnectedSetsAndMembers(instance_count, sets, comps, true))
		return;

	for (std::uint32_t index = 0; index < sets.length(); ++index)
	{
		MObject set = sets[index];
		MObject comp = comps[index];

		MFnSet fn_set(set, &status);
		if (status == MStatus::kFailure)
			continue;

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

		dgIt.disablePruningOnFilter();

		// If no texture file node was found, just continue.
		//
		if (dgIt.isDone())
			continue;

		// Print out the texture node name and texture file that it references.
		//
		MObject textureNode = dgIt.currentItem();
		MPlug filenamePlug = MFnDependencyNode(textureNode).findPlug("fileTextureName", true);
		MString textureName;
		std::ostringstream os;
		filenamePlug.getValue(textureName);
		os << "Set: " << fn_set.name() << endl;
		os << "Texture Node Name: " << MFnDependencyNode(textureNode).name() << endl;
		os << "Texture File Name: " << textureName.asChar() << endl;

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
