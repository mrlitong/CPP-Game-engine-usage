#include "GameProcess.h"
#include "Object.h"
#include "Engine.h"
#include "World.h"
#include "App.h"
#include "ToolsCamera.h"
#include "ControlsApp.h"
#include "MathLib.h"



using namespace MathLib;

CGameProcess::CGameProcess(void)
{

}

CGameProcess::~CGameProcess(void)
{

}
int CGameProcess::Init()
{
	g_Engine.pFileSystem->CacheFilesFormExt("char");
	g_Engine.pFileSystem->CacheFilesFormExt("node");
	g_Engine.pFileSystem->CacheFilesFormExt("smesh");
	g_Engine.pFileSystem->CacheFilesFormExt("sanim");

	g_Engine.pWorld->LoadWorld("data/scene/terrain/test/test.world");
	//g_Engine.pWorld->LoadWorld("data/scene/terrain/cj/cj.world"); 
	g_Engine.pControls->SetKeyPressFunc(KeyPress);
	g_Engine.pControls->SetKeyReleaseFunc(KeyRelease);

	m_pRole = new CFPSRoleLocal();
	m_pRole->Init(10001, "data/role/hero/FpsRole/fps.char");		//加载角色资源

	m_pRole->SetActorPosition(vec3(0, 0, 0));	//设置角色初始位置。以门处作为原点，三维坐标系vec3是向量
	m_pSkillSystem = new CSkillSystem(this);
	m_pCameraBase = new CCameraBase();
	m_pCameraBase->SetEnabled(1);



}