#include "material_parser.hpp"

// Maya API
#include <maya/MFnDependencyNode.h>
#include <maya/MFnMesh.h>
#include <maya/MIntArray.h>
#include <maya/MObjectArray.h>
#include <maya/MPlug.h>
#include <maya/MPlugArray.h>

// C++ standard
#include <vector>

#include <maya/MGlobal.h>
#include <sstream>

MString GetShaderName(MObject shadingEngine)
{
	// attach a function set to the shading engine
	MFnDependencyNode fn(shadingEngine);

	// get access to the surfaceShader attribute. This will be connected to
	// lambert , phong nodes etc.
	MPlug sshader = fn.findPlug("surfaceShader");

	// will hold the connections to the surfaceShader attribute
	MPlugArray materials;

	// get the material connected to the surface shader
	sshader.connectedTo(materials, true, false);

	// if we found a material
	if (materials.length())
	{
		MFnDependencyNode fnMat(materials[0].node());
		return fnMat.name();
	}
	return "none";
}

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
					std::ostringstream os;
					
					os	<< "\t\tmaterials 1\n";
					os	<< "\t\t\t"
						<< GetShaderName(shaders[0]).asChar()
						<< std::endl;

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
						std::ostringstream os;
						os << "\t\t\t"
							<< GetShaderName(shaders[shader_index]).asChar()
							<< "\n\t\t\t"
							<< faces_by_material_index[shader_index].size()
							<< "\n\t\t\t\t";

						MGlobal::displayInfo(os.str().c_str());

						// Get all faces used by this material index
						for (unsigned int & itr : faces_by_material_index[shader_index])
						{
							std::ostringstream oss;

							// Use the material index here
							oss << itr << std::endl;

							MGlobal::displayInfo(oss.str().c_str());
						}
					}
				}
				break;
		}
	}
}
