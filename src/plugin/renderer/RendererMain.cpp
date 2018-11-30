#include "RendererMain.hpp"

// TODO: Remove this once debugging is over
#include <maya/MGlobal.h>
#include <maya/MStreamUtils.h>

wmr::wri::RendererMain::RendererMain()
{
}

wmr::wri::RendererMain::~RendererMain()
{
}

void wmr::wri::RendererMain::StartWispRenderer()
{
}

void wmr::wri::RendererMain::UpdateWispRenderer()
{
	MStreamUtils::stdOutStream() << "Wisp renderer update!" << "\n";
}

void wmr::wri::RendererMain::StopWispRenderer()
{
}
