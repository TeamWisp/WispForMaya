#include "material_parser.hpp"

// Maya API
#include <maya/MFnDependencyNode.h>
#include <maya/MFnMesh.h>
#include <maya/MIntArray.h>
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

// From the devkit: findTexturesPerPolygonCmd.cpp
MObject findShader(MObject& setNode)
//
//  Description:
//      Find the shading node for the given shading group set node.
//
{
	MFnDependencyNode fnNode(setNode);
	MPlug shaderPlug = fnNode.findPlug("surfaceShader", true);

	if (!shaderPlug.isNull())
	{
		MPlugArray connectedPlugs;
		bool asSrc = false;
		bool asDst = true;
		shaderPlug.connectedTo(connectedPlugs, asDst, asSrc);

		if (connectedPlugs.length() != 1)
			cerr << "Error getting shader\n";
		else
			return connectedPlugs[0].node();
	}

	return MObject::kNullObj;
}

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

		// Find the texture that is applied to this set.  First, get the
		// shading node connected to the set.  Then, if there is an input
		// attribute called "color", search upstream from it for a texture
		// file node.
		//
		MObject shaderNode = findShader(set);
		if (shaderNode == MObject::kNullObj)
			continue;

		MPlug colorPlug = MFnDependencyNode(shaderNode).findPlug("color", true, &status);
		if (status == MS::kFailure)
			continue;

		MItDependencyGraph dgIt(colorPlug, MFn::kFileTexture,
			MItDependencyGraph::kUpstream,
			MItDependencyGraph::kBreadthFirst,
			MItDependencyGraph::kNodeLevel,
			&status);

		if (status == MS::kFailure)
			continue;

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

		MGlobal::displayInfo(os.str().c_str());
	}
}
